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
private:
    std::mutex _mutex;
    std::queue<Command*> _commands;
    WaveAnalyzer _analyzer;
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
signals:
    void sampleReady(std::vector<signed short> samples);
    void frameReady(std::vector<unsigned char> frame);
    void sampleCutReady(std::vector<signed short> samples);
    void finished();

public slots:

    void seekPipeline(int pos);
};

#endif  // GSTREAMERTHREADWORKER_H
