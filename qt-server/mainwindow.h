#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "chartmanager.h"
#include "serialmanager.h"
#include "tcpstreamserver.h"

#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QMainWindow>
#include <QNetworkInterface>
#include <QTextStream>
#include <QUrl>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

/**
 * @brief 스마트 모니터링 시스템의 메인 GUI 컨트롤러
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    // TCP 스트리밍 & 원격 제어 슬롯
    void on_btnStartServer_clicked();
    void onFrameReceived(const QPixmap& pixmap);
    void onClientCountChanged(int count);
    void onLogMessage(const QString& message);
    void onValveCommandFromClient(char cmd);

    // 시리얼 통신 & 밸브 제어 슬롯
    void on_btnSerialConnect_clicked();
    void on_btnValveClose_clicked();
    void on_btnValveOpen_clicked();
    void onGasDataReceived(int adcValue);
    void onSerialStatusMessage(const QString& msg, bool isError);
    void onSerialConnectionChanged(bool isConnected);

    // 임계값 및 파일 로깅 슬롯
    void on_btnApplyThreshold_clicked();
    void on_btnSelectPath_clicked();
    void on_btnOpenFolder_clicked();

private:
    QString getLocalIPAddress();
    void updateStatusBadge(bool isDanger);
    void logGasDataToCsv(int adcValue, int threshold, bool isDanger);

private:
    Ui::MainWindow* ui;
    TcpStreamServer* m_streamServer;
    SerialManager* m_serialManager;
    ChartManager* m_chartManager;

    int m_currentThreshold = 3000;
    QString m_saveDirPath = "./logs";
    bool m_isValveClosed = false; // 밸브 중복 제어 방지 래치 플래그
};

#endif // MAINWINDOW_H