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

// ⭐ 연결된 안드로이드 클라이언트로 가스 수치 데이터를 쏴주는 로직
void TcpStreamServer::sendGasDataToClient(int adcValue, int threshold)
{
    if (m_clientSocket && m_clientSocket->isOpen()) {
        // 포맷: GAS:수치:임계값\n
        QString dataStr = QString("GAS:%1:%2\n").arg(adcValue).arg(threshold);
        m_clientSocket->write(dataStr.toUtf8());
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

    while (!m_buffer.isEmpty()) {

        // =========================================================
        // [1단계] 아직 헤더를 안 읽은 새 패킷 시작점일 때 (m_imageSize == 0)
        // =========================================================
        if (m_imageSize == 0) {

            // A. 텍스트 명령어 처리 ('1', '0' 등 첫 바이트가 0x00이 아닌 경우)
            if (static_cast<unsigned char>(m_buffer[0]) != 0x00) {
                int newlineIdx = m_buffer.indexOf('\n');

                if (newlineIdx != -1) {
                    QByteArray lineBytes = m_buffer.left(newlineIdx).trimmed();
                    m_buffer.remove(0, newlineIdx + 1);

                    QString cmd = QString::fromUtf8(lineBytes);
                    if (cmd == "1" || cmd == "0") {
                        emit valveCommandReceived(cmd.at(0).toLatin1());
                    }
                } else if (m_buffer.size() == 1 && (m_buffer[0] == '1' || m_buffer[0] == '0')) {
                    char cmdChar = m_buffer[0];
                    m_buffer.remove(0, 1);
                    emit valveCommandReceived(cmdChar);
                } else {
                    // 개행문자(\n)가 다 올 때까지 대기
                    break;
                }
                continue; // 다음 루프로 이동
            }

            // B. 비디오 프레임 헤더 읽기 (첫 바이트가 0x00인 바이너리 길이)
            if (m_buffer.size() < static_cast<int>(sizeof(qint32))) {
                break; // 4바이트 헤더가 다 쌓일 때까지 대기
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

        // =========================================================
        // [2단계] 비디오 프레임 바디(JPEG) 수신 (m_imageSize > 0)
        // =========================================================
        if (m_buffer.size() < m_imageSize) {
            break; // 전체 이미지 데이터가 다 올 때까지 대기
        }

        QByteArray jpegData = m_buffer.left(m_imageSize);
        m_buffer.remove(0, m_imageSize);

        QPixmap pixmap;
        if (pixmap.loadFromData(jpegData, "JPEG")) {
            emit frameReceived(pixmap);
        }

        m_imageSize = 0; // 프레임 완료 후 초기화
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
