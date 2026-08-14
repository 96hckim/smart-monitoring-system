#include "tcpstreamserver.h"
#include "logger.h"
#include <QDataStream>

TcpStreamServer::TcpStreamServer(QObject* parent)
    : QObject(parent)
    , m_server(new QTcpServer(this))
    , m_clientSocket(nullptr)
    , m_imageSize(0)
{
    connect(m_server, &QTcpServer::newConnection, this, &TcpStreamServer::onNewConnection);
}

TcpStreamServer::~TcpStreamServer()
{
    stopServer();
}

bool TcpStreamServer::startServer(quint16 port)
{
    if (m_server->listen(QHostAddress::AnyIPv4, port)) {
        emit logMessage(Logger::format(LogCategory::TCP, LogLevel::Info,
            QString("서버가 포트 %1에서 시작되었습니다.").arg(port)));
        return true;
    } else {
        emit logMessage(Logger::format(LogCategory::TCP, LogLevel::Error,
            QString("서버 시작 실패: %1").arg(m_server->errorString())));
        return false;
    }
}

void TcpStreamServer::stopServer()
{
    if (m_clientSocket) {
        m_clientSocket->abort();
        m_clientSocket->deleteLater();
        m_clientSocket = nullptr;

        emit clientCountChanged(0);
    }
    if (m_server->isListening()) {
        m_server->close();
    }
}

void TcpStreamServer::onNewConnection()
{
    if (m_clientSocket) {
        m_clientSocket->disconnect();
        m_clientSocket->abort();
        m_clientSocket->deleteLater();
        m_clientSocket = nullptr;
    }

    m_clientSocket = m_server->nextPendingConnection();

    connect(m_clientSocket, &QTcpSocket::readyRead, this, &TcpStreamServer::onReadyRead);
    connect(m_clientSocket, &QTcpSocket::disconnected, this, &TcpStreamServer::onClientDisconnected);

    m_buffer.clear();
    m_imageSize = 0;

    QString clientIp = m_clientSocket->peerAddress().toString();
    emit logMessage(Logger::format(LogCategory::TCP, LogLevel::Info,
        QString("새 클라이언트가 연결되었습니다. (%1)").arg(clientIp)));

    emit clientCountChanged(1);
}

void TcpStreamServer::onReadyRead()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket || socket != m_clientSocket)
        return;

    m_buffer.append(socket->readAll());

    while (true) {
        if (m_imageSize == 0) {
            if (m_buffer.size() < static_cast<int>(sizeof(qint32))) {
                return;
            }

            QDataStream stream(m_buffer.left(sizeof(qint32)));
            stream.setByteOrder(QDataStream::BigEndian);
            stream >> m_imageSize;

            m_buffer.remove(0, sizeof(qint32));

            if (m_imageSize <= 0 || m_imageSize > 10 * 1024 * 1024) {
                emit logMessage(Logger::format(LogCategory::TCP, LogLevel::Warn,
                    QString("손상된 패킷 감지 (%1 바이트). 버퍼를 비웁니다.").arg(m_imageSize)));
                m_buffer.clear();
                m_imageSize = 0;
                return;
            }
        }

        if (m_buffer.size() < m_imageSize) {
            return;
        }

        QByteArray jpegData = m_buffer.left(m_imageSize);
        m_buffer.remove(0, m_imageSize);

        QPixmap pixmap;
        if (pixmap.loadFromData(jpegData, "JPEG")) {
            emit frameReceived(pixmap);
        }

        m_imageSize = 0;

        if (m_buffer.size() < static_cast<int>(sizeof(qint32))) {
            break;
        }
    }
}

void TcpStreamServer::onClientDisconnected()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());

    emit logMessage(Logger::format(LogCategory::TCP, LogLevel::Info, "클라이언트 연결이 해제되었습니다."));

    if (socket && socket == m_clientSocket) {
        m_clientSocket = nullptr;
        m_buffer.clear();
        m_imageSize = 0;

        emit clientCountChanged(0);
    }

    if (socket) {
        socket->deleteLater();
    }
}