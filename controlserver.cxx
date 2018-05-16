#include "controlserver.hpp"
#include <QDebug>
#include <QCoreApplication>
#include <iostream>

ControlServer::ControlServer(QObject* parent)
    : QObject(parent)
{
    mTcpServer = new QTcpServer(this);

    connect(mTcpServer, &QTcpServer::newConnection, this, &ControlServer::slotNewConnection);

    if (!mTcpServer->listen(QHostAddress::Any, 6000)) {
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

        std::cout << "RAW SIZE:" << std::hex << static_cast<unsigned int>(messageSize) << " ID:" << static_cast<unsigned int>(messageId) << std::endl;
        if ((_currentArray.size()) >= (messageSize + VIDEO_PROTOCOL::HEADER_SIZE)) {
            switch (messageId) {
            case VIDEO_PROTOCOL::MESSAGE_TYPE_START_REG:
                onMessageStartReg();
                break;
            case VIDEO_PROTOCOL::MESSAGE_TYPE_STOP_REG:
                onMessageStopReg();
                break;
            case VIDEO_PROTOCOL::MESSAGE_TYPE_VIEW_MODE:
                onMessageViewMode();
                break;
            case VIDEO_PROTOCOL::MESSAGE_TYPE_REALTIME_MODE:
                onMessageRealtimeMode();
                break;
            case VIDEO_PROTOCOL::MESSAGE_TYPE_SHOW_COORD:
                unsigned int coord;
                unsigned int b1 = _currentArray.at(2);
                unsigned int b2 = _currentArray.at(3);
                unsigned int b3 = _currentArray.at(4);
                unsigned int b4 = _currentArray.at(5);
                coord = b1 | b2 << 8 | b3 << 16 | b4 << 24;

                onMessageShowCoord(coord);
                break;
            }
            _currentArray.remove(0, messageSize + VIDEO_PROTOCOL::HEADER_SIZE);
        }
    }
}

void ControlServer::onMessageStartReg()
{
    std::cout << "MESSAGE START REG" << std::endl;
}

void ControlServer::onMessageStopReg()
{
    std::cout << "MESSAGE STOP REG" << std::endl;
}

void ControlServer::onMessageViewMode()
{
    std::cout << "MESSAGE VIEW" << std::endl;
}

void ControlServer::onMessageRealtimeMode()
{
    std::cout << "MESSAGE REALTIME" << std::endl;
}

void ControlServer::onMessageShowCoord(unsigned int coord)
{
    std::cout << "MESSAGE COORD: " << coord << std::endl;
}

void ControlServer::slotNewConnection()
{
    mTcpSocket = mTcpServer->nextPendingConnection();

    mTcpSocket->write("INIT");

    connect(mTcpSocket, &QTcpSocket::readyRead, this, &ControlServer::slotServerRead);
    connect(mTcpSocket, &QTcpSocket::disconnected, this, &ControlServer::slotClientDisconnected);
}

void ControlServer::slotServerRead()
{
    while (mTcpSocket->bytesAvailable() > 0) {
        QByteArray array = mTcpSocket->readAll();
        _currentArray.append(array);
        parseMessage();

        mTcpSocket->write("OK");
    }
}

void ControlServer::slotClientDisconnected()
{
    mTcpSocket->close();
}
