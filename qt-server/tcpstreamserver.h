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

signals:
    // ⭐ MainWindow에 전달할 시그널 정의
    void frameReceived(const QPixmap& pixmap); // 완성된 영상 프레임 전달
    void logMessage(const QString& message); // 로그 메시지 전달
    void clientCountChanged(int count);

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