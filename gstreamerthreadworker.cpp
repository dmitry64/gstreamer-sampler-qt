#include "gstreamerthreadworker.h"
#include <unistd.h>
#include <iomanip>
#include <QApplication>
#include "settings.h"

GstFlowReturn on_worker_new_audio_sample_from_sink(GstElement* elt, ProgramData* data)
{
    GstSample* sample;
    GstBuffer* buffer;
    GstMapInfo info;

    sample = gst_app_sink_pull_sample(GST_APP_SINK(elt));
    buffer = gst_sample_get_buffer(sample);
    gst_buffer_map(buffer, &info, GST_MAP_READ);
    QSharedPointer<std::vector<signed short>> outputVector(new std::vector<signed short>(info.size / 2));

    memcpy(outputVector->data(), info.data, info.size);

    data->worker->addSampleAndTimestamp(outputVector, buffer->pts, buffer->duration);
    data->worker->sendSignalBuffers();
    data->worker->sendAudioSample(outputVector);

    gst_sample_unref(sample);
    return GST_FLOW_OK;
}

GstFlowReturn on_worker_new_video_sample_from_sink(GstElement* elt, ProgramData* data)
{
    GstSample* sample;
    GstBuffer* buffer;
    GstMapInfo info;

    sample = gst_app_sink_pull_sample(GST_APP_SINK(elt));
    buffer = gst_sample_get_buffer(sample);
    gst_buffer_map(buffer, &info, GST_MAP_READ);

    QSharedPointer<std::vector<unsigned char>> outputVector(new std::vector<unsigned char>(info.size));
    memcpy(outputVector->data(), info.data, info.size);
    data->worker->sendVideoSample(outputVector);

    gst_buffer_unmap(buffer, &info);
    gst_sample_unref(sample);

    return GST_FLOW_OK;
}

static gboolean on_worker_source_message(GstBus* bus, GstMessage* message, ProgramData* data)
{
    switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_EOS:
        g_print("The source got dry\n");
        break;
    case GST_MESSAGE_ERROR:
        g_print("Received error\n");
        // g_main_loop_quit(data->loop);
        break;
    default:
        break;
    }
    return TRUE;
}

static gboolean gst_worker_my_filter_sink_event_worker(GstPad* pad, GstObject* parent, GstEvent* event)
{
    if (event->type != GST_EVENT_TAG) {
        std::cout << "WORKER EVENT:" << GST_EVENT_TYPE_NAME(event) << std::endl;
    }
    switch (GST_EVENT_TYPE(event)) {
    case GST_EVENT_EOS:
        std::cerr << "WORKER EOS EVENT!" << std::endl;
        break;
    }
    gboolean ret = gst_pad_event_default(pad, parent, event);
    return ret;
}

void GstreamerThreadWorker::setCameraType(const CameraType& cameraType)
{
    _cameraType = cameraType;
}

void GstreamerThreadWorker::run()
{
    mainLoop();
    exec();
    std::cout << "Worker Main loop finished..." << std::endl;

    gst_element_set_state(_data->source, GST_STATE_NULL);
    gst_object_unref(_data->source);
    g_free(_data);
    std::cout << "Worker GStreamer thread finished..." << std::endl;
    emit finished();
}

void GstreamerThreadWorker::addSampleAndTimestamp(QSharedPointer<std::vector<signed short>> samples, GstClockTime time, GstClockTime duration)
{
    _waveThread.addSampleAndTimestamp(samples, time, duration);
}

void GstreamerThreadWorker::sendSignalBuffers()
{
    QSharedPointer<std::vector<signed short>> samples;
    std::vector<signed short> samplesData;
    while (_waveThread.getNextBuffer(samplesData)) {
        samples = QSharedPointer<std::vector<signed short>>(new std::vector<signed short>(samplesData));
        emit sampleCutReady(samples);
    }
    unsigned int coord;
    GstClockTime time;
    while (_waveThread.getNextCoord(coord, time)) {
        emit coordReady(coord, time, static_cast<int>(_cameraType));
    }
}

GstreamerThreadWorker::GstreamerThreadWorker(QObject* parent)
    : QThread(parent)
    , _timeoutId(0)
    , _currentFileName("video")
{
}

void GstreamerThreadWorker::sendAudioSample(QSharedPointer<std::vector<signed short>> samples)
{
    emit sampleReady(samples);
}

void GstreamerThreadWorker::sendVideoSample(QSharedPointer<std::vector<unsigned char>> frame)
{
    emit frameReady(frame);
}

void GstreamerThreadWorker::mainLoop()
{
    GstBus* bus = NULL;
    GstElement* myaudiosink = NULL;
    GstElement* myworkervideosink = NULL;

    std::cout << "Initializing worker..." << std::endl;
    gst_init(0, 0);

    _data = g_new0(ProgramData, 1);
    _data->worker = this;

    int id = static_cast<int>(_cameraType);

    QString cameraAddress;
    QString currentFilePath = restoreDefaultVideoFolder();
    std::cout << "Current file path: " << currentFilePath.toStdString() << std::endl;
    if (_cameraType == eCameraLeft) {
        cameraAddress = restoreLeftCameraAddress();
    }
    else {
        cameraAddress = restoreRightCameraAddress();
    }
    QString finalPath = currentFilePath + "/" + _currentPath + _currentFileName + QString::number(id);
    QString launchString = "rtspsrc debug=true udp-buffer-size=500000000 timeout=50000000 latency=4000 buffer-mode=0 location=" + cameraAddress
                           + " sync=false name=demux "
                             "demux. ! rtph264depay ! queue name=videoqueue ! tee name=t ! queue name=filequeue ! mpegtsmux ! queue ! filesink location="
                           + finalPath
                           + ".ts buffer-mode=unbuffered t. ! queue ! "
                             "decodebin ! videoconvert ! "
                             "videoscale ! video/x-raw,format=RGB,width=1280,height=720 ! appsink name=myworkervideosink "
                             "caps=\"video/x-raw,format=RGB,width=1280,height=720\" sync=false "
                             "demux. ! queue name=audioqueue ! decodebin !"
                             "audioconvert ! audioresample ! audio/x-raw,format=S16LE,channels=1,rate=8000,layout=interleaved ! appsink "
                             "caps=\"audio/x-raw,format=S16LE,channels=1,rate=8000,layout=interleaved\" name=myaudiosink sync=false";

    // string = g_strdup_printf
    // good ("filesrc location=/workspace/gst-qt/samples/test.avi ! avidemux name=d ! queue ! xvimagesink d. ! audioconvert ! audioresample ! appsink caps=\"%s\" name=myaudiosink", filename, audio_caps);
    // ("filesrc location=/workspace/gst-qt/samples/bunny.mkv ! matroskademux ! h264parse ! avdec_h264 ! videorate ! videoconvert ! videoscale ! video/x-raw,format=RGB16,width=640,height=480 ! appsink name=myworkervideosink sync=true");
    //    ();
    std::cerr << "Pipeline string: \n" << launchString.toStdString() << std::endl;
    _data->source = gst_parse_launch(launchString.toStdString().c_str(), NULL);

    std::cerr << "Created worker pipeline..." << std::endl;

    if (_data->source == NULL) {
        g_print("Bad source\n");
        return;
    }

    bus = gst_element_get_bus(_data->source);
    gst_bus_add_watch(bus, (GstBusFunc) on_worker_source_message, _data);
    gst_object_unref(bus);

    myaudiosink = gst_bin_get_by_name(GST_BIN(_data->source), "myaudiosink");
    g_object_set(G_OBJECT(myaudiosink), "emit-signals", TRUE, "sync", FALSE, NULL);
    g_signal_connect(myaudiosink, "new-sample", G_CALLBACK(on_worker_new_audio_sample_from_sink), _data);
    gst_object_unref(myaudiosink);

    std::cout << "Worker audio sink ready..." << std::endl;

    myworkervideosink = gst_bin_get_by_name(GST_BIN(_data->source), "myworkervideosink");
    g_object_set(G_OBJECT(myworkervideosink), "emit-signals", TRUE, "sync", FALSE, NULL);
    g_signal_connect(myworkervideosink, "new-sample", G_CALLBACK(on_worker_new_video_sample_from_sink), _data);
    gst_object_unref(myworkervideosink);

    std::cout << "Worker video sink ready..." << std::endl;

    GstPad* appsinkPad = gst_element_get_static_pad(GST_ELEMENT(myworkervideosink), "sink");
    gst_pad_set_event_function(appsinkPad, gst_worker_my_filter_sink_event_worker);

    gst_element_set_state(_data->source, GST_STATE_PLAYING);

    _waveThread.startAnalysys(finalPath);

    std::cout << "Starting worker main loop..." << std::endl;
}

void GstreamerThreadWorker::stopWorker()
{
    std::cout << "Stop worker" << std::endl;
    QThread::exit(0);
}

void GstreamerThreadWorker::setRegistrationFileName(const QString& path, const QString& name)
{
    _currentPath = path;
    _currentFileName = name;
}
