#ifndef TCPSTREAMSERVER_H
#define TCPSTREAMSERVER_H

#include <QByteArray>
#include <QObject>
#include <QPixmap>
#include <QTcpServer>
#include <QTcpSocket>

class TcpStreamServer : public QObject {
    Q_OBJECT
public:
    explicit TcpStreamServer(QObject* parent = nullptr);
    ~TcpStreamServer();

    bool startServer(quint16 port);
    void stopServer();

    // ⭐ 안드로이드 클라이언트로 가스 수치를 쏴주는 중계 함수 (추후 연동용)
    void sendGasDataToClient(int adcValue, int threshold);

signals:
    void frameReceived(const QPixmap& pixmap);
    void logMessage(const QString& message);
    void clientCountChanged(int count);
    void valveCommandReceived(char cmd);

private slots:
    void onNewConnection();
    void onReadyRead();
    void onClientDisconnected();

private:
    QTcpServer* m_server;
    QTcpSocket* m_clientSocket;
    QByteArray m_buffer;
    qint32 m_imageSize;
};

#endif // TCPSTREAMSERVER_H