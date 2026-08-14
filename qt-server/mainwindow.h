#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "chartmanager.h"
#include "serialmanager.h"
#include "tcpstreamserver.h"
#include <QMainWindow>
#include <QNetworkInterface>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    // --- [1. TCP 스트리밍 관련 슬롯] ---
    void on_btnStartServer_clicked();
    void onFrameReceived(const QPixmap& pixmap);
    void onClientCountChanged(int count);
    void onLogMessage(const QString& message);
    void onValveCommandFromClient(char cmd);

    // --- [2. 시리얼 통신 관련 슬롯] ---
    void on_btnSerialConnect_clicked(); // [시리얼 연결] 버튼
    void on_btnValveClose_clicked(); // [밸브 차단] 버튼 ('1' 송신)
    void on_btnValveOpen_clicked(); // [밸브 복구] 버튼 ('0' 송신)

    void onGasDataReceived(int adcValue); // 가스 ADC 수치 수신 슬롯
    void onSerialStatusMessage(const QString& msg, bool isError); // 시리얼 상태 로그
    void onSerialConnectionChanged(bool isConnected); // 연결/해제 UI 토글

    // --- [3. 임계값 적용 슬롯] ---
    void on_btnApplyThreshold_clicked(); // [적용] 버튼 클릭 슬롯

private:
    QString getLocalIPAddress();
    void updateStatusBadge(bool isDanger);

private:
    Ui::MainWindow* ui;
    TcpStreamServer* m_streamServer; // 스트리밍 서버 모듈
    SerialManager* m_serialManager; // 시리얼 통신 전담 모듈
    ChartManager* m_chartManager; // 차트 매니저

    int m_currentThreshold = 3000; // 현재 실제 적용 중인 위험 임계값
};

#endif // MAINWINDOW_H