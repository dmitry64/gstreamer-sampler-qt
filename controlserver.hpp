#ifndef CONTROLSERVER_HPP
#define CONTROLSERVER_HPP
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include "videocontrolprotocol.hpp"

class ControlServer : public QObject
{
    Q_OBJECT
public:
    explicit ControlServer(QObject* parent = nullptr);

private:
    void parseMessage();
    void onMessageStartReg(const QString &name);
    void onMessageStopReg();
    void onMessageViewMode();
    void onMessageRealtimeMode();
    void onMessageShowCoord(unsigned int coord);
public slots:
    void slotNewConnection();
    void slotServerRead();
    void slotClientDisconnected();
signals:
    void clientConnected();
    void clientDisconnected();
    void doStartRegistration(QString name);
    void doStopRegistration();
    void doViewMode();
    void doRealtimeMode();
    void doSetCoord(unsigned int coord);


private:
    QTcpServer* mTcpServer;
    QTcpSocket* mTcpSocket;
    QByteArray _currentArray;
};

#endif  // CONTROLSERVER_HPP
