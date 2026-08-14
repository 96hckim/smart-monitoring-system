#include "mainwindow.h"
#include "logger.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_streamServer(new TcpStreamServer(this))
    , m_serialManager(new SerialManager(this))
    , m_chartManager(new ChartManager(this))
    , m_currentThreshold(3000)
    , m_saveDirPath("./logs")
{
    ui->setupUi(this);

    // ----------------------------------------------------
    // 1. UI 초기 세팅 (IP, 클라이언트 수, BaudRate, 포트, 저장경로)
    // ----------------------------------------------------
    if (ui->lblServerIp) {
        ui->lblServerIp->setText(QString("IP: %1").arg(getLocalIPAddress()));
    }

    if (ui->lblClientCount) {
        ui->lblClientCount->setText("접속 상태: 클라이언트 0 명 접속 중");
    }

    if (ui->cbBaudRate) {
        ui->cbBaudRate->clear();
        ui->cbBaudRate->addItems({ "9600", "19200", "38400", "57600", "115200" });
        ui->cbBaudRate->setCurrentText("115200");
    }

    if (ui->cbPortList) {
        QStringList ports = m_serialManager->availablePorts();
        if (!ports.isEmpty()) {
            ui->cbPortList->clear();
            ui->cbPortList->addItems(ports);
        }
    }

    // CSV 저장 경로 UI 초기화
    if (ui->leLogPath) {
        ui->leLogPath->setText(m_saveDirPath);
    }

    // ----------------------------------------------------
    // 2. 시그널 - 슬롯 연결 (TCP 서버 모듈)
    // ----------------------------------------------------
    connect(m_streamServer, &TcpStreamServer::frameReceived, this, &MainWindow::onFrameReceived);
    connect(m_streamServer, &TcpStreamServer::logMessage, this, &MainWindow::onLogMessage);
    connect(m_streamServer, &TcpStreamServer::clientCountChanged, this, &MainWindow::onClientCountChanged);
    connect(m_streamServer, &TcpStreamServer::valveCommandReceived, this, &MainWindow::onValveCommandFromClient);

    // ----------------------------------------------------
    // 3. 시그널 - 슬롯 연결 (시리얼 통신 모듈)
    // ----------------------------------------------------
    connect(m_serialManager, &SerialManager::dataReceived, this, &MainWindow::onGasDataReceived);
    connect(m_serialManager, &SerialManager::statusMessage, this, &MainWindow::onSerialStatusMessage);
    connect(m_serialManager, &SerialManager::connectionStateChanged, this, &MainWindow::onSerialConnectionChanged);

    // ----------------------------------------------------
    // 4. 차트 & 임계값 스핀박스 초기화
    // ----------------------------------------------------
    if (ui->chartView) {
        m_chartManager->initChart(ui->chartView);
        m_chartManager->setThreshold(m_currentThreshold);
    }

    if (ui->sbThreshold) {
        ui->sbThreshold->setValue(m_currentThreshold);
        // SpinBox에서 Enter 키 입력 시 [적용] 버튼 클릭 동작 연동
        connect(ui->sbThreshold, &QSpinBox::editingFinished, this, &MainWindow::on_btnApplyThreshold_clicked);
    }

    updateStatusBadge(false); // 초기 상태 배지: 정상(초록색)
}

MainWindow::~MainWindow()
{
    delete ui;
}

// ----------------------------------------------------
// ⭐ 내 PC의 첫 번째 유효 IPv4 주소 조회
// ----------------------------------------------------
QString MainWindow::getLocalIPAddress()
{
    const QList<QHostAddress> addresses = QNetworkInterface::allAddresses();
    for (const QHostAddress& address : addresses) {
        if (address != QHostAddress::LocalHost && address.toIPv4Address()) {
            return address.toString();
        }
    }
    return "127.0.0.1";
}

// ----------------------------------------------------
// [서버 시작 / 중지] 토글 버튼
// ----------------------------------------------------
void MainWindow::on_btnStartServer_clicked()
{
    if (ui->btnStartServer->text() == "서버 시작") {
        quint16 port = ui->lePort->text().toUShort();
        if (port == 0)
            port = 8080;

        if (m_streamServer->startServer(port)) {
            ui->btnStartServer->setText("서버 중지");
            if (ui->lePort)
                ui->lePort->setEnabled(false);
        }
    } else {
        m_streamServer->stopServer();
        ui->btnStartServer->setText("서버 시작");
        if (ui->lePort)
            ui->lePort->setEnabled(true);
        onClientCountChanged(0);
    }
}

void MainWindow::onClientCountChanged(int count)
{
    if (ui->lblClientCount) {
        ui->lblClientCount->setText(QString("접속 상태: 클라이언트 %1 명 접속 중").arg(count));
    }
}

void MainWindow::onFrameReceived(const QPixmap& pixmap)
{
    if (ui->lblCameraPreview) {
        ui->lblCameraPreview->setPixmap(
            pixmap.scaled(ui->lblCameraPreview->size(),
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation));
    }
}

void MainWindow::onLogMessage(const QString& message)
{
    if (ui->teLog) {
        ui->teLog->append(message);
    }
}

// ----------------------------------------------------
// [시리얼 연결 / 해제] 버튼
// ----------------------------------------------------
void MainWindow::on_btnSerialConnect_clicked()
{
    if (m_serialManager->isConnected()) {
        m_serialManager->disconnectPort();
    } else {
        QString selectedPort = ui->cbPortList ? ui->cbPortList->currentText() : "";
        qint32 baudRate = ui->cbBaudRate ? ui->cbBaudRate->currentText().toInt() : 115200;

        if (!selectedPort.isEmpty()) {
            m_serialManager->connectPort(selectedPort, baudRate);
        }
    }
}

// [밸브 차단] 버튼 ('1' 송신)
void MainWindow::on_btnValveClose_clicked()
{
    if (m_serialManager->sendChar('1')) {
        onLogMessage(Logger::format(LogCategory::Serial, LogLevel::Tx, "STM32로 밸브 차단 명령 ('1') 전송"));
    }
}

// [밸브 복구] 버튼 ('0' 송신)
void MainWindow::on_btnValveOpen_clicked()
{
    if (m_serialManager->sendChar('0')) {
        onLogMessage(Logger::format(LogCategory::Serial, LogLevel::Tx, "STM32로 밸브 복구 명령 ('0') 전송"));
    }
}

// ----------------------------------------------------
// ⭐ 가스 수치 실시간 수신 처리 핵심 로직
// ----------------------------------------------------
void MainWindow::onGasDataReceived(int adcValue)
{
    // 1. UI 라벨 표출
    if (ui->lblGasVal) {
        ui->lblGasVal->setText(QString::number(adcValue));
    }

    // 2. 실시간 차트에 데이터 점 추가
    if (m_chartManager) {
        m_chartManager->addGasData(adcValue);
    }

    // 3. 안드로이드 클라이언트로 가스 수치 + 현재 임계값 실시간 중계 (GAS:adc:thresh)
    if (m_streamServer) {
        m_streamServer->sendGasDataToClient(adcValue, m_currentThreshold);
    }

    // 4. 위험 판단
    bool isDanger = (adcValue >= m_currentThreshold);

    // 5. CSV 파일로 실시간 누적 저장 (체크박스 활성화 시)
    logGasDataToCsv(adcValue, m_currentThreshold, isDanger);

    // 6. UI 상태 배지 업데이트
    updateStatusBadge(isDanger);

    // 7. 자동 차단 체크 시 위험 상황 제어
    if (ui->chkAutoClose && ui->chkAutoClose->isChecked() && isDanger) {
        if (m_serialManager->sendChar('1')) {
            onLogMessage(Logger::format(LogCategory::Serial, LogLevel::Warn,
                QString("⚠️ [위험] 가스 수치 초과(%1 >= %2)! 밸브를 자동으로 차단합니다.")
                    .arg(adcValue)
                    .arg(m_currentThreshold)));
        }
    }
}

// ----------------------------------------------------
// ⭐ 안드로이드 원격 제어 명령 수신 슬롯
// ----------------------------------------------------
void MainWindow::onValveCommandFromClient(char cmd)
{
    if (cmd == '1') {
        onLogMessage(Logger::format(LogCategory::TCP, LogLevel::Info, "📱 [안드로이드 원격 제어] 밸브 차단 요청 수신"));
        on_btnValveClose_clicked();
    } else if (cmd == '0') {
        onLogMessage(Logger::format(LogCategory::TCP, LogLevel::Info, "📱 [안드로이드 원격 제어] 밸브 복구 요청 수신"));
        on_btnValveOpen_clicked();
    }
}

// ----------------------------------------------------
// [임계값 적용] 버튼 클릭 슬롯
// ----------------------------------------------------
void MainWindow::on_btnApplyThreshold_clicked()
{
    if (!ui->sbThreshold)
        return;

    int inputVal = ui->sbThreshold->value();
    if (m_currentThreshold == inputVal)
        return;

    m_currentThreshold = inputVal;

    if (m_chartManager) {
        m_chartManager->setThreshold(m_currentThreshold);
    }

    onLogMessage(Logger::format(LogCategory::Serial, LogLevel::Info,
        QString("가스 위험 임계값이 %1 ADC로 변경 적용되었습니다.").arg(m_currentThreshold)));
}

// ----------------------------------------------------
// ⭐ [경로 선택] 버튼 클릭 슬롯 (QFileDialog 연동)
// ----------------------------------------------------
void MainWindow::on_btnSelectPath_clicked()
{
    QString selectedDir = QFileDialog::getExistingDirectory(
        this,
        "로그 파일 저장 폴더 선택",
        m_saveDirPath,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!selectedDir.isEmpty()) {
        m_saveDirPath = selectedDir;

        if (ui->leLogPath) {
            ui->leLogPath->setText(m_saveDirPath);
        }

        onLogMessage(Logger::format(LogCategory::System, LogLevel::Info,
            QString("CSV 저장 경로가 변경되었습니다: %1").arg(m_saveDirPath)));
    }
}

// ----------------------------------------------------
// ⭐ [폴더 열기] 버튼 클릭 슬롯 (윈도우 탐색기 열기)
// ----------------------------------------------------
void MainWindow::on_btnOpenFolder_clicked()
{
    // 폴더가 없으면 우선 생성
    QDir dir(m_saveDirPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // 윈도우 파일 탐색기로 해당 경로 열기
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_saveDirPath));
}

// ----------------------------------------------------
// ⭐ 가스 수치를 일자별 CSV 파일에 저장하는 함수
// ----------------------------------------------------
void MainWindow::logGasDataToCsv(int adcValue, int threshold, bool isDanger)
{
    // CSV 저장 체크박스가 꺼져있으면 기록 안 함
    if (ui->chkEnableLogging && !ui->chkEnableLogging->isChecked()) {
        return;
    }

    // 1. 지정된 저장 폴더 존재 확인 및 생성
    QDir dir(m_saveDirPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // 2. 일자별 파일명 (예: ./logs/gas_log_2026-08-14.csv)
    QString dateStr = QDate::currentDate().toString("yyyy-MM-dd");
    QString filePath = QString("%1/gas_log_%2.csv").arg(m_saveDirPath, dateStr);

    QFile file(filePath);
    bool isNewFile = !file.exists();

    // 3. Append(덧붙이기) 모드로 열어 데이터 기록
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);

        // 새 파일인 경우 최상단 컬럼 헤더 추가
        if (isNewFile) {
            out << "Timestamp,ADC_Value,Threshold,Status\n";
        }

        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
        QString status = isDanger ? "DANGER" : "NORMAL";

        // 데이터 한 줄 덧붙이기
        out << timestamp << "," << adcValue << "," << threshold << "," << status << "\n";
        file.close();
    }
}

// ----------------------------------------------------
// UI 상태 배지 업데이트 헬퍼
// ----------------------------------------------------
void MainWindow::updateStatusBadge(bool isDanger)
{
    if (!ui->lblStatusBadge)
        return;

    if (isDanger) {
        ui->lblStatusBadge->setText("위 험");
        ui->lblStatusBadge->setStyleSheet("background-color: #FF2D55; color: white; font-weight: bold; border-radius: 4px;");
    } else {
        ui->lblStatusBadge->setText("정 상");
        ui->lblStatusBadge->setStyleSheet("background-color: #34C759; color: white; font-weight: bold; border-radius: 4px;");
    }
}

void MainWindow::onSerialStatusMessage(const QString& msg, bool isError)
{
    Q_UNUSED(isError);
    onLogMessage(msg);
}

void MainWindow::onSerialConnectionChanged(bool isConnected)
{
    if (ui->btnSerialConnect) {
        ui->btnSerialConnect->setText(isConnected ? "시리얼 해제" : "시리얼 연결");
    }
    if (ui->cbPortList) {
        ui->cbPortList->setEnabled(!isConnected);
    }
    if (ui->cbBaudRate) {
        ui->cbBaudRate->setEnabled(!isConnected);
    }
}