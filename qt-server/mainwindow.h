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

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    // --- [1. TCP 서버 & 영상 수신 슬롯] ---
    void on_btnStartServer_clicked();
    void onFrameReceived(const QPixmap& pixmap);
    void onClientCountChanged(int count);
    void onLogMessage(const QString& message);
    void onValveCommandFromClient(char cmd); // 안드로이드 원격 제어 수신

    // --- [2. 시리얼 통신 슬롯] ---
    void on_btnSerialConnect_clicked();
    void on_btnValveClose_clicked();
    void on_btnValveOpen_clicked();
    void onGasDataReceived(int adcValue);
    void onSerialStatusMessage(const QString& msg, bool isError);
    void onSerialConnectionChanged(bool isConnected);

    // --- [3. 임계값 설정 슬롯] ---
    void on_btnApplyThreshold_clicked();

    // --- [4. CSV 파일 저장 및 경로 관리 슬롯] ---
    void on_btnSelectPath_clicked(); // [경로 선택] 버튼 (QFileDialog)
    void on_btnOpenFolder_clicked(); // [폴더 열기] 버튼 (탐색기)

private:
    QString getLocalIPAddress();
    void updateStatusBadge(bool isDanger);
    void logGasDataToCsv(int adcValue, int threshold, bool isDanger);

private:
    Ui::MainWindow* ui;
    TcpStreamServer* m_streamServer; // TCP 스트리밍 서버 모듈
    SerialManager* m_serialManager; // 시리얼 통신 전담 모듈
    ChartManager* m_chartManager; // 차트 매니저

    int m_currentThreshold = 3000; // 현재 적용된 위험 임계값
    QString m_saveDirPath = "./logs"; // 기본 로그 저장 경로
};

#endif // MAINWINDOW_H