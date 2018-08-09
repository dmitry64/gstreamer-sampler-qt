#ifndef GSTREAMERTHREADWORKER_H
#define GSTREAMERTHREADWORKER_H

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

#include "waveanalyzer.hpp"
#include "waveanalyzerthread.hpp"

class GstreamerThreadWorker;

class ProgramData
{
public:
    // GMainLoop* loop;
    GstElement* source;
    // GstElement* audiosink;
    GstreamerThreadWorker* worker;
};

class GstreamerThreadWorker : public QThread
{
    Q_OBJECT

public:
    enum CameraType
    {
        eCameraLeft = 0,
        eCameraRight = 1
    };

private:
    ProgramData* _data;
    std::mutex _mutex;
    WaveAnalyzerThread _waveThread;
    CameraType _cameraType;
    guint _timeoutId;
    QString _currentFileName;
    QString _currentPath;
    void run();
    void mainLoop();

public:
    void addSampleAndTimestamp(QSharedPointer<std::vector<signed short>> samples, GstClockTime time, GstClockTime duration);
    void sendSignalBuffers();
    void stopWorker();
    void setRegistrationFileName(const QString& path, const QString& name);

public:
    explicit GstreamerThreadWorker(QObject* parent = nullptr);

    void sendAudioSample(QSharedPointer<std::vector<signed short>> samples);
    void sendVideoSample(QSharedPointer<std::vector<unsigned char>> frame);
    void setCameraType(const CameraType& cameraType);

signals:
    void sampleReady(QSharedPointer<std::vector<signed short>> samples);
    void frameReady(QSharedPointer<std::vector<unsigned char>> frame);
    void sampleCutReady(QSharedPointer<std::vector<signed short>> samples);
    void coordReady(unsigned int coord, GstClockTime time, int cameraIndex);
    void finished();
};

#endif  // GSTREAMERTHREADWORKER_H
