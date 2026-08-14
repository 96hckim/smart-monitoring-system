#include "mainwindow.h"
#include "logger.h"
#include "ui_mainwindow.h"
#include <QNetworkInterface> // ⭐ 내 PC IP 조회를 위해 추가

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_streamServer(new TcpStreamServer(this))
    , m_serialManager(new SerialManager(this))
    , m_chartManager(new ChartManager(this))
{
    ui->setupUi(this);

    // ----------------------------------------------------
    // 1. ⭐ UI 초기 설정 (IP, 접속 상태, BaudRate)
    // ----------------------------------------------------
    // 내 PC IPv4 주소 자동 조회 및 라벨 출력
    if (ui->lblServerIp) {
        ui->lblServerIp->setText(QString("IP: %1").arg(getLocalIPAddress()));
    }

    // 클라이언트 접속 상태 초기화
    if (ui->lblClientCount) {
        ui->lblClientCount->setText("접속 상태: 클라이언트 0 명 접속 중");
    }

    // 시리얼 BaudRate 콤보박스 목록 설정 (기본값 115200)
    if (ui->cbBaudRate) {
        ui->cbBaudRate->clear();
        ui->cbBaudRate->addItems({ "9600", "19200", "38400", "57600", "115200" });
        ui->cbBaudRate->setCurrentText("115200");
    }

    // 시리얼 포트 목록 채우기
    if (ui->cbPortList) {
        ui->cbPortList->clear();
        ui->cbPortList->addItems(m_serialManager->availablePorts());
    }

    // ----------------------------------------------------
    // 2. 시그널 - 슬롯 연결 (TCP 서버)
    // ----------------------------------------------------
    connect(m_streamServer, &TcpStreamServer::frameReceived, this, &MainWindow::onFrameReceived);
    connect(m_streamServer, &TcpStreamServer::logMessage, this, &MainWindow::onLogMessage);
    connect(m_streamServer, &TcpStreamServer::clientCountChanged, this, &MainWindow::onClientCountChanged);

    // ----------------------------------------------------
    // 3. 시그널 - 슬롯 연결 (시리얼 통신)
    // ----------------------------------------------------
    connect(m_serialManager, &SerialManager::dataReceived, this, &MainWindow::onGasDataReceived);
    connect(m_serialManager, &SerialManager::statusMessage, this, &MainWindow::onSerialStatusMessage);
    connect(m_serialManager, &SerialManager::connectionStateChanged, this, &MainWindow::onSerialConnectionChanged);

    // ----------------------------------------------------
    // 4. 차트 및 상태 배지 초기화
    // ----------------------------------------------------
    if (ui->chartView) {
        m_chartManager->initChart(ui->chartView);
    }

    updateStatusBadge(false); // 초기 상태: 정상(초록색)
}

MainWindow::~MainWindow()
{
    delete ui;
}

// ----------------------------------------------------
// ⭐ 내 PC의 IPv4 주소를 가져오는 헬퍼 함수
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
// ⭐ [서버 시작 / 중지] 토글 버튼 (isListening 에러 수정 위치)
// ----------------------------------------------------
void MainWindow::on_btnStartServer_clicked()
{
    // 버튼 텍스트가 "서버 시작"이면 구동, 아니면 중지
    if (ui->btnStartServer->text() == "서버 시작") {
        quint16 port = ui->lePort->text().toUShort();
        if (port == 0)
            port = 8080; // 기본 포트 8080

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

// ⭐ 클라이언트 접속자 수 변경 슬롯
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
// ⭐ [시리얼 연결 / 해제] 버튼 (BaudRate 연동)
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

// [밸브 차단] 버튼 클릭
void MainWindow::on_btnValveClose_clicked()
{
    if (m_serialManager->sendChar('1')) {
        onLogMessage(Logger::format(LogCategory::Serial, LogLevel::Tx, "STM32로 밸브 차단 명령 ('1') 전송"));
    }
}

// [밸브 복구] 버튼 클릭
void MainWindow::on_btnValveOpen_clicked()
{
    if (m_serialManager->sendChar('0')) {
        onLogMessage(Logger::format(LogCategory::Serial, LogLevel::Tx, "STM32로 밸브 복구 명령 ('0') 전송"));
    }
}

// ----------------------------------------------------
// ⭐ 가스 수치 데이터 수신 및 배지/자동 차단 제어
// ----------------------------------------------------
void MainWindow::onGasDataReceived(int adcValue)
{
    // 1. 라벨 수치 표출
    if (ui->lblGasVal) {
        ui->lblGasVal->setText(QString::number(adcValue));
    }

    // 2. 실시간 차트에 점 추가
    if (m_chartManager) {
        m_chartManager->addGasData(adcValue);
    }

    // 3. 임계값 비교 및 상태 배지 / 자동 차단 처리
    int threshold = ui->sbThreshold ? ui->sbThreshold->value() : 3000;
    bool isDanger = (adcValue >= threshold);

    // 상태 배지 업데이트 (정상 <-> 위험)
    updateStatusBadge(isDanger);

    // 자동 차단 옵션 체크 시 위험 상황 처리
    if (ui->chkAutoClose && ui->chkAutoClose->isChecked() && isDanger) {
        if (m_serialManager->sendChar('1')) {
            onLogMessage(Logger::format(LogCategory::Serial, LogLevel::Warn,
                QString("⚠️ [위험] 가스 수치 초과(%1 >= %2)! 밸브를 자동으로 차단합니다.").arg(adcValue).arg(threshold)));
        }
    }
}

// ⭐ 상태 배지(lblStatusBadge) 색상 및 문구 변경 헬퍼
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

// UI에서 임계값(SpinBox) 숫자를 변경하면 차트 가로선 위치도 같이 이동
void MainWindow::on_sbThreshold_valueChanged(int value)
{
    if (m_chartManager) {
        m_chartManager->setThreshold(value);
    }
}