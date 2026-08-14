#include "serialmanager.h"
#include "logger.h"
#include <QDebug>

SerialManager::SerialManager(QObject* parent)
    : QObject(parent)
    , m_serialPort(new QSerialPort(this))
{
    connect(m_serialPort, &QSerialPort::readyRead, this, &SerialManager::onReadyRead);
    connect(m_serialPort, &QSerialPort::errorOccurred, this, &SerialManager::onErrorOccurred);
}

SerialManager::~SerialManager()
{
    disconnectPort();
}

QStringList SerialManager::availablePorts() const
{
    QStringList ports;
    const auto portInfos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo& info : portInfos) {
        ports.append(info.portName());
    }
    return ports;
}

bool SerialManager::connectPort(const QString& portName, qint32 baudRate)
{
    if (m_serialPort->isOpen()) {
        m_serialPort->close();
    }

    m_serialPort->setPortName(portName);
    m_serialPort->setBaudRate(baudRate);
    m_serialPort->setDataBits(QSerialPort::Data8);
    m_serialPort->setParity(QSerialPort::NoParity);
    m_serialPort->setStopBits(QSerialPort::OneStop);
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);

    if (m_serialPort->open(QIODevice::ReadWrite)) {
        m_serialPort->clear();

        emit statusMessage(Logger::format(LogCategory::Serial, LogLevel::Info,
                               QString("포트 연결 성공: %1 (%2 bps)").arg(portName).arg(baudRate)),
            false);
        emit connectionStateChanged(true);
        return true;
    } else {
        emit statusMessage(Logger::format(LogCategory::Serial, LogLevel::Error,
                               QString("포트 연결 실패: %1 (%2)").arg(portName).arg(m_serialPort->errorString())),
            true);
        emit connectionStateChanged(false);
        return false;
    }
}

void SerialManager::disconnectPort()
{
    if (m_serialPort->isOpen()) {
        m_serialPort->close();
        emit statusMessage(Logger::format(LogCategory::Serial, LogLevel::Info, "시리얼 포트 연결이 해제되었습니다."), false);
        emit connectionStateChanged(false);
    }
}

bool SerialManager::isConnected() const
{
    return m_serialPort->isOpen();
}

bool SerialManager::sendCommand(const QString& cmd)
{
    if (!m_serialPort->isOpen())
        return false;

    qint64 bytesWritten = m_serialPort->write(cmd.toUtf8());
    return bytesWritten != -1;
}

bool SerialManager::sendChar(char cmd)
{
    if (!m_serialPort->isOpen())
        return false;

    qint64 bytesWritten = m_serialPort->write(&cmd, 1);
    return bytesWritten != -1;
}

void SerialManager::onReadyRead()
{
    while (m_serialPort->canReadLine()) {
        QByteArray lineBytes = m_serialPort->readLine();
        QString line = QString::fromUtf8(lineBytes).trimmed();

        if (line.isEmpty())
            continue;

        emit rawLineReceived(line);

        bool isNumber = false;
        int adcVal = line.toInt(&isNumber);

        if (isNumber && adcVal >= 0 && adcVal <= 4095) {
            emit dataReceived(adcVal);
        }
    }
}

void SerialManager::onErrorOccurred(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError) {
        emit statusMessage(Logger::format(LogCategory::Serial, LogLevel::Error, "장치가 강제로 연결 해제되었습니다."), true);
        disconnectPort();
    }
}