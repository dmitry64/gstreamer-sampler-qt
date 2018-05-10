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

#include "commands.h"
#include "waveanalyzer.hpp"
#include "waveanalyzerthread.hpp"

class GstreamerThreadWorker;

class ProgramData
{
public:
    GMainLoop* loop;
    GstElement* source;
    GstElement* audiosink;
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
    std::mutex _mutex;
    std::queue<WorkerCommand*> _commands;

    WaveAnalyzerThread _waveThread;
    CameraType _cameraType;

    void run();
    void mainLoop();

public:
    void handleCommands(ProgramData* data);
    void addSampleAndTimestamp(const std::vector<signed short>& samples, GstClockTime time, GstClockTime duration);
    void sendSignalBuffers();
    void stopWorker();

public:
    explicit GstreamerThreadWorker(QObject* parent = nullptr);

    void sendAudioSample(std::vector<signed short>& samples);
    void sendVideoSample(std::vector<unsigned char>& frame);
    void setCameraType(const CameraType& cameraType);

signals:
    void sampleReady(std::vector<signed short> samples);
    void frameReady(std::vector<unsigned char> frame);
    void sampleCutReady(std::vector<signed short> samples);
    void coordReady(unsigned int coord, GstClockTime time, int cameraIndex);
    void finished();
};

#endif  // GSTREAMERTHREADWORKER_H
