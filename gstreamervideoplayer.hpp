#ifndef GSTREAMERVIDEOPLAYER_HPP
#define GSTREAMERVIDEOPLAYER_HPP

#include <QObject>


#include <gst/gst.h>

#include <string.h>

#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include <iostream>
#include <vector>
#include <queue>
#include <mutex>
#include <QThread>
#include <QSharedPointer>

#include "commands.h"

class GstreamerVideoPlayer;

class PlayerProgramData
{
public:
    GMainLoop* loop;
    GstElement* source;
    GstreamerVideoPlayer* worker;
};


class GstreamerVideoPlayer : public QThread
{
    Q_OBJECT
public:
    enum CameraType
    {
        eCameraLeft = 0,
        eCameraRight = 1
    };

private:
    std::mutex _mutex;
    std::queue<PlayerCommand*> _commands;
    CameraType _cameraType;
    guint _timeoutId;
    QString _currentFileName;
    QString _currentPath;

    void run();
    void mainLoop();

public:
    void handleCommands(PlayerProgramData* data);
    void stopWorker();
    void playStream();
    void showFrameAt(GstClockTime time);
    void stopHandlerTimeout();

public:
    explicit GstreamerVideoPlayer(QObject* parent = nullptr);

    void sendVideoSample(QSharedPointer<std::vector<unsigned char>> frame);
    void setCameraType(const CameraType& cameraType);
    void setRegistrationFileName(const QString& path, const QString& name);

    void sendLoadingFrame(bool status);
signals:
    void frameReady(QSharedPointer<std::vector<unsigned char>> frame);
    void statusChanged(bool status);
    void loadingFrame(bool status);
    void finished();
};

#endif  // GSTREAMERVIDEOPLAYER_HPP
