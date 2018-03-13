#ifndef GSTREAMERTHREADWORKER_H
#define GSTREAMERTHREADWORKER_H

#include <QObject>


#include <gst/gst.h>

#include <string.h>

#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include <iostream>
#include <vector>


class GstreamerThreadWorker : public QObject
{
    Q_OBJECT
public:
    typedef struct
    {
        GMainLoop* loop;
        GstElement* source;
        GstElement* audiosink;
        GstreamerThreadWorker* worker;
    } ProgramData;

public:
    explicit GstreamerThreadWorker(QObject* parent = nullptr);

    void sendAudioSample(std::vector<signed short>& samples);
    void sendVideoSample(std::vector<unsigned char>& frame);
signals:
    void sampleReady(std::vector<signed short> samples);
    void frameReady(std::vector<unsigned char> frame);

public slots:
    void mainLoop();
};

#endif  // GSTREAMERTHREADWORKER_H
