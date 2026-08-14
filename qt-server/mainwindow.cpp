#include "mainwindow.h"
#include "logger.h"
#include "ui_mainwindow.h"
#include <QNetworkInterface>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_streamServer(new TcpStreamServer(this))
    , m_serialManager(new SerialManager(this))
    , m_chartManager(new ChartManager(this))
    , m_currentThreshold(3000)
{
    ui->setupUi(this);

    // ----------------------------------------------------
    // 1. UI 초기 설정 (IP, 접속 상태, BaudRate, 포트)
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

    // ----------------------------------------------------
    // 2. 시그널 - 슬롯 연결 (TCP 서버)
    // ----------------------------------------------------
    connect(m_streamServer, &TcpStreamServer::frameReceived, this, &MainWindow::onFrameReceived);
    connect(m_streamServer, &TcpStreamServer::logMessage, this, &MainWindow::onLogMessage);
    connect(m_streamServer, &TcpStreamServer::clientCountChanged, this, &MainWindow::onClientCountChanged);
    connect(m_streamServer, &TcpStreamServer::valveCommandReceived, this, &MainWindow::onValveCommandFromClient);

    // ----------------------------------------------------
    // 3. 시그널 - 슬롯 연결 (시리얼 통신)
    // ----------------------------------------------------
    connect(m_serialManager, &SerialManager::dataReceived, this, &MainWindow::onGasDataReceived);
    connect(m_serialManager, &SerialManager::statusMessage, this, &MainWindow::onSerialStatusMessage);
    connect(m_serialManager, &SerialManager::connectionStateChanged, this, &MainWindow::onSerialConnectionChanged);

    // ----------------------------------------------------
    // 4. 차트 및 스핀박스/배지 초기화 (중복 호출 제거)
    // ----------------------------------------------------
    if (ui->chartView) {
        m_chartManager->initChart(ui->chartView);
        m_chartManager->setThreshold(m_currentThreshold);
    }

    if (ui->sbThreshold) {
        ui->sbThreshold->setValue(m_currentThreshold);
        // 키보드로 입력 후 Enter 키를 눌렀을 때도 적용 버튼 누른 것과 동일하게 연동
        connect(ui->sbThreshold, &QSpinBox::editingFinished, this, &MainWindow::on_btnApplyThreshold_clicked);
    }

    updateStatusBadge(false); // 초기 상태: 정상(초록색)
}

MainWindow::~MainWindow()
{
    delete ui;
}

// ----------------------------------------------------
// ⭐ 내 PC의 IPv4 주소를 가져오는 헬퍼 함수 (핫스팟 IP 우선)
// ----------------------------------------------------
QString MainWindow::getLocalIPAddress()
{
    QString defaultIp = "";
    const QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();

    for (const QNetworkInterface& netInterface : interfaces) {
        QNetworkInterface::InterfaceFlags flags = netInterface.flags();
        if ((flags & QNetworkInterface::IsUp) && !(flags & QNetworkInterface::IsLoopBack)) {
            for (const QNetworkAddressEntry& entry : netInterface.addressEntries()) {
                QHostAddress ip = entry.ip();
                if (ip.protocol() == QAbstractSocket::IPv4Protocol) {
                    QString ipStr = ip.toString();
                    // 1순위: 모바일 핫스팟 대역 감지 시 우선 반환
                    if (ipStr.startsWith("192.168.137.")) {
                        return ipStr;
                    }
                    if (defaultIp.isEmpty()) {
                        defaultIp = ipStr;
                    }
                }
            }
        }
    }
    return defaultIp.isEmpty() ? "127.0.0.1" : defaultIp;
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

// ⭐ 안드로이드 앱에서 밸브 차단/복구 버튼을 눌렀을 때 실행
void MainWindow::onValveCommandFromClient(char cmd)
{
    if (cmd == '1') {
        onLogMessage(Logger::format(LogCategory::TCP, LogLevel::Info, "📱 [안드로이드 원격 제어] 밸브 차단 요청 수신"));
        on_btnValveClose_clicked(); // 기존 밸브 차단 함수 실행 (STM32로 '1' 전달)
    } else if (cmd == '0') {
        onLogMessage(Logger::format(LogCategory::TCP, LogLevel::Info, "📱 [안드로이드 원격 제어] 밸브 복구 요청 수신"));
        on_btnValveOpen_clicked(); // 기존 밸브 복구 함수 실행 (STM32로 '0' 전달)
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

void MainWindow::on_btnValveClose_clicked()
{
    if (m_serialManager->sendChar('1')) {
        onLogMessage(Logger::format(LogCategory::Serial, LogLevel::Tx, "STM32로 밸브 차단 명령 ('1') 전송"));
    }
}

void MainWindow::on_btnValveOpen_clicked()
{
    if (m_serialManager->sendChar('0')) {
        onLogMessage(Logger::format(LogCategory::Serial, LogLevel::Tx, "STM32로 밸브 복구 명령 ('0') 전송"));
    }
}

// ----------------------------------------------------
// 가스 수치 데이터 수신 처리
// ----------------------------------------------------
void MainWindow::onGasDataReceived(int adcValue)
{
    // 1. UI 라벨 수치 표출
    if (ui->lblGasVal) {
        ui->lblGasVal->setText(QString::number(adcValue));
    }

    // 2. 실시간 차트에 데이터 점 추가
    if (m_chartManager) {
        m_chartManager->addGasData(adcValue);
    }

    // 3. ⭐ [추가] 연결된 안드로이드 클라이언트로 가스 수치 + 현재 임계값 실시간 중계!
    if (m_streamServer) {
        m_streamServer->sendGasDataToClient(adcValue, m_currentThreshold);
    }

    // 4. 확정된 m_currentThreshold와 수치 비교
    bool isDanger = (adcValue >= m_currentThreshold);

    // 5. 상태 배지 업데이트 (정상 <-> 위험)
    updateStatusBadge(isDanger);

    // 6. 자동 차단 체크 시 위험 상황 동작
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
// [적용] 버튼 클릭 시 동작하는 슬롯
// ----------------------------------------------------
void MainWindow::on_btnApplyThreshold_clicked()
{
    if (!ui->sbThreshold)
        return;

    int inputVal = ui->sbThreshold->value();

    // 기존 적용값과 같으면 중복 실행 방지
    if (m_currentThreshold == inputVal)
        return;

    // 1. 실제 내부 위험 임계값 갱신
    m_currentThreshold = inputVal;

    // 2. 차트 가로선 위치 반영
    if (m_chartManager) {
        m_chartManager->setThreshold(m_currentThreshold);
    }

    // 3. 변경 결과 로그 출력
    onLogMessage(Logger::format(LogCategory::Serial, LogLevel::Info,
        QString("가스 위험 임계값이 %1 ADC로 변경 적용되었습니다.").arg(m_currentThreshold)));
}

// ⭐ 상태 배지 UI 갱신 헬퍼
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