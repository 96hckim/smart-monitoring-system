#ifndef SERIALMANAGER_H
#define SERIALMANAGER_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QStringList>

class SerialManager : public QObject {
    Q_OBJECT
public:
    explicit SerialManager(QObject* parent = nullptr);
    ~SerialManager();

    // 1. 사용 가능한 COM 포트 목록 조회
    QStringList availablePorts() const;

    // 2. 시리얼 포트 연결 및 해제
    bool connectPort(const QString& portName, qint32 baudRate = QSerialPort::Baud115200);
    void disconnectPort();
    bool isConnected() const;

    // 3. STM32로 데이터 전송 (명령어 '1', '0' 등)
    bool sendCommand(const QString& cmd);
    bool sendChar(char cmd);

signals:
    // UI로 보낼 이벤트 시그널들
    void dataReceived(int adcValue); // 가스 ADC 수치 파싱 성공 시
    void rawLineReceived(const QString& line); // 원본 문자열 수신 시 (로그용)
    void statusMessage(const QString& msg, bool isError); // 연결 상태 메시지
    void connectionStateChanged(bool isConnected); // 연결/해제 상태 변경 시

private slots:
    void onReadyRead(); // 수신 데이터 처리
    void onErrorOccurred(QSerialPort::SerialPortError error); // 에러 처리

private:
    QSerialPort* m_serialPort;
};

#endif // SERIALMANAGER_H