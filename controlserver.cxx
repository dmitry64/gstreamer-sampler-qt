#include "controlserver.hpp"
#include <QDebug>
#include <QCoreApplication>
#include <iostream>

ControlServer::ControlServer(QObject* parent)
    : QObject(parent)
{
    mTcpServer = new QTcpServer(this);

    connect(mTcpServer, &QTcpServer::newConnection, this, &ControlServer::slotNewConnection);

    if (!mTcpServer->listen(QHostAddress::Any, 49005)) {
        qDebug() << "server is not started";
    }
    else {
        qDebug() << "server is started";
    }
}

void ControlServer::parseMessage()
{
    if (_currentArray.size() > 1) {
        unsigned char messageId = _currentArray.at(0);
        unsigned char messageSize = _currentArray.at(1);

        std::cout << "RAW SIZE:" << static_cast<unsigned int>(messageSize) << " ID:" << std::hex << static_cast<unsigned int>(messageId) << std::dec << std::endl;
        if ((_currentArray.size()) >= (messageSize + VIDEO_PROTOCOL::HEADER_SIZE)) {
            switch (messageId) {
            case VIDEO_PROTOCOL::MESSAGE_TYPE_START_REG: {
                std::cout << "=================================== START REG: " << static_cast<unsigned int>(messageSize) << std::endl;
                QByteArray stringArray(messageSize, 0x00);

                for (int i = 0; i < messageSize; ++i) {
                    stringArray[i] = _currentArray.at(i + VIDEO_PROTOCOL::HEADER_SIZE);
                }

                onMessageStartReg(QString(stringArray));
            } break;
            case VIDEO_PROTOCOL::MESSAGE_TYPE_STOP_REG: {
                std::cout << "=================================== STOP REG! " << std::endl;
                onMessageStopReg();
            } break;
            case VIDEO_PROTOCOL::MESSAGE_TYPE_VIEW_MODE: {
                std::cout << "=================================== VIEW MODE! " << std::endl;
                onMessageViewMode();
            } break;
            case VIDEO_PROTOCOL::MESSAGE_TYPE_REALTIME_MODE: {
                std::cout << "=================================== REALTIME MODE! " << std::endl;
                onMessageRealtimeMode();
            } break;
            case VIDEO_PROTOCOL::MESSAGE_TYPE_SHOW_COORD: {
                unsigned int coord = 0;
                unsigned int b1 = _currentArray.at(2) & 0x000000ff;
                unsigned int b2 = _currentArray.at(3) & 0x000000ff;
                unsigned int b3 = _currentArray.at(4) & 0x000000ff;
                unsigned int b4 = _currentArray.at(5) & 0x000000ff;

                coord = b4;
                coord = coord << 8;
                coord |= b3;
                coord = coord << 8;
                coord |= b2;
                coord = coord << 8;
                coord |= b1;

                std::cout << "=================================== SHOW: " << coord << std::endl;
                onMessageShowCoord(coord);
            } break;
            }
            _currentArray.remove(0, messageSize + VIDEO_PROTOCOL::HEADER_SIZE);
        }
    }
}

void ControlServer::onMessageStartReg(const QString& name)
{
    std::cout << "MESSAGE START REG" << std::endl;

    emit doStartRegistration(name);
}

void ControlServer::onMessageStopReg()
{
    std::cout << "MESSAGE STOP REG" << std::endl;
    emit doStopRegistration();
}

void ControlServer::onMessageViewMode()
{
    std::cout << "MESSAGE VIEW" << std::endl;
    emit doViewMode();
}

void ControlServer::onMessageRealtimeMode()
{
    std::cout << "MESSAGE REALTIME" << std::endl;
    emit doRealtimeMode();
}

void ControlServer::onMessageShowCoord(unsigned int coord)
{
    std::cout << "MESSAGE COORD: " << coord << std::endl;
    emit doSetCoord(coord);
}

void ControlServer::slotNewConnection()
{
    mTcpSocket = mTcpServer->nextPendingConnection();
    connect(mTcpSocket, &QTcpSocket::readyRead, this, &ControlServer::slotServerRead);
    connect(mTcpSocket, &QTcpSocket::disconnected, this, &ControlServer::slotClientDisconnected);
    emit clientConnected();
}

void ControlServer::slotServerRead()
{
    while (mTcpSocket->bytesAvailable() > 0) {
        const QByteArray& array = mTcpSocket->readAll();
        _currentArray.append(array);
        parseMessage();
    }
}

void ControlServer::slotClientDisconnected()
{
    mTcpSocket->close();
    emit clientDisconnected();
}
