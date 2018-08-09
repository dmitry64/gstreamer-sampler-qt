#include "gstreamervideoplayer.hpp"
#include <unistd.h>
#include <iomanip>
#include <QApplication>
#include "settings.h"

GstFlowReturn on_new_video_sample_from_sink_player(GstElement* elt, PlayerProgramData* data)
{
    std::cerr << "FRAME!" << std::endl;
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
    // gst_element_set_state(data->source, GST_STATE_PAUSED);
    // gst_element_set_state(data->source, GST_STATE_PAUSED);
    return GST_FLOW_FLUSHING;
}

static gboolean on_source_message_player(GstBus* bus, GstMessage* message, PlayerProgramData* data)
{
    switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_EOS:
        std::cout << "The source got dry" << std::endl;
        break;
    case GST_MESSAGE_ERROR:
        g_print("Received error\n");
        break;
    case GST_MESSAGE_STATE_CHANGED:
        std::cout << "================ STATE CHANGED!!!" << std::endl;
        break;
    case GST_MESSAGE_ASYNC_DONE:
        std::cout << "================ ASYNC DONE!" << std::endl;
        break;
    default:
        break;
    }
    return TRUE;
}

static gboolean gst_my_filter_sink_event_player(GstPad* pad, GstObject* parent, GstEvent* event)
{
    if (event->type != GST_EVENT_TAG) {
        std::cout << "PLAYER EVENT:" << GST_EVENT_TYPE_NAME(event) << std::endl;
    }
    switch (GST_EVENT_TYPE(event)) {
    case GST_EVENT_EOS: {
        std::cerr << "PLAYER EOS EVENT!" << std::endl;
        GstElement* element = reinterpret_cast<GstElement*>(pad);


    } break;
    case GST_EVENT_SEEK:
        std::cerr << "PLAYER SEEK EVENT!" << std::endl;
        break;
    case GST_EVENT_SEGMENT:
        std::cerr << "PLAYER SEGMENT EVENT!" << std::endl;
        break;
    case GST_EVENT_FLUSH_START:
        std::cerr << "PLAYER FLUSH BEGIN..." << std::endl;
        break;
    case GST_EVENT_FLUSH_STOP:
        std::cerr << "PLAYER FLUSH END!" << std::endl;
        break;
    default:

        break;
    }
    gboolean ret = gst_pad_event_default(pad, parent, event);
    return ret;
}

void GstreamerVideoPlayer::setCameraType(const CameraType& cameraType)
{
    _cameraType = cameraType;
}

void GstreamerVideoPlayer::setRegistrationFileName(const QString& path, const QString& name)
{
    _currentPath = path;
    _currentFileName = name;
}

void GstreamerVideoPlayer::sendLoadingFrame(bool status)
{
    emit loadingFrame(status);
}

gint64 GstreamerVideoPlayer::getTotalDuration()
{
    gint64 len;
    gst_element_query_duration(_data->source, GST_FORMAT_TIME, &len);

    return len;
}

void GstreamerVideoPlayer::run()
{
    mainLoop();
    exec();

    std::cout << "Main loop finished..." << std::endl;

    gst_element_set_state(_data->source, GST_STATE_NULL);

    gst_object_unref(_data->source);
    g_free(_data);

    std::cout << "GStreamer thread finished..." << std::endl;
    emit finished();
}

GstreamerVideoPlayer::GstreamerVideoPlayer(QObject* parent)
    : QThread(parent)
    , _timeoutId(0)
    , _currentFileName("video")
{
}

void GstreamerVideoPlayer::sendVideoSample(QSharedPointer<std::vector<unsigned char>> frame)
{
    emit frameReady(frame);
}

void GstreamerVideoPlayer::mainLoop()
{
    GstBus* bus = NULL;
    GstElement* myplayervideosink = NULL;

    std::cout << "Initializing..." << std::endl;
    gst_init(0, 0);

    _data = g_new0(PlayerProgramData, 1);
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

    QString launchString("filesrc name=inputfile location=" + currentFilePath + "/" + _currentPath + _currentFileName + QString::number(id)
                         + ".ts ! tsdemux ! queue ! h264parse ! decodebin ! videoconvert ! videoscale ! videorate ! "
                           "video/x-raw,format=RGB,width=1280,height=720 ! appsink name=myplayervideosink");

    /* QString launchString("filesrc location=/workspace/gst-qt/samples/bunny.mkv ! matroskademux ! queue ! h264parse ! avdec_h264 ! videoconvert ! videoscale ! videorate ! "
                          "video/x-raw,format=RGB,width=1280,height=720 ! appsink name=myplayervideosink");*/

    // string = g_strdup_printf
    // good ("filesrc location=/workspace/gst-qt/samples/test.avi ! avidemux name=d ! queue ! xvimagesink d. ! audioconvert ! audioresample ! appsink caps=\"%s\" name=myaudiosink", filename, audio_caps);
    // ("filesrc location=/workspace/gst-qt/samples/bunny.mkv ! matroskademux ! h264parse ! avdec_h264 ! videorate ! videoconvert ! videoscale ! video/x-raw,format=RGB,width=1280,height=720 ! appsink name=myvideosink sync=true");
    //(,  // filesrc location=file%d.ts use-mmap=false ! tsdemux
    // id);
    std::cout << "Pipeline string: \n" << launchString.toStdString().c_str() << std::endl;  // filesrc location=file%d.ts ! tsparse ! tsdemux
    _data->source = gst_parse_launch(launchString.toStdString().c_str(), NULL);
    std::cout << "Created pipeline..." << std::endl;

    if (_data->source == NULL) {
        g_print("Bad source\n");
        return;
    }

    bus = gst_element_get_bus(_data->source);
    gst_bus_add_watch(bus, (GstBusFunc) on_source_message_player, _data);
    gst_object_unref(bus);

    myplayervideosink = gst_bin_get_by_name(GST_BIN(_data->source), "myplayervideosink");
    g_object_set(G_OBJECT(myplayervideosink), "emit-signals", TRUE, "sync", FALSE, NULL);
    g_signal_connect(myplayervideosink, "new-sample", G_CALLBACK(on_new_video_sample_from_sink_player), _data);
    gst_object_unref(myplayervideosink);

    std::cout << "Player video sink ready..." << std::endl;

    GstPad* appsinkPad = gst_element_get_static_pad(GST_ELEMENT(myplayervideosink), "sink");
    gst_pad_set_event_function(appsinkPad, gst_my_filter_sink_event_player);

    gst_element_set_state(_data->source, GST_STATE_PAUSED);
    // gst_element_set_state(_data->source, GST_STATE_PLAYING);

    std::cout << "Starting main loop..." << std::endl;
}

void GstreamerVideoPlayer::stopWorker()
{
    std::cout << "Stop player" << std::endl;
    QThread::exit(0);
}

void GstreamerVideoPlayer::showFrameAt(GstClockTime time)
{
    std::cout << "POS:" << time / 1000.0f / 1000.0f / 1000.0f << "s" << std::endl;
    sync();
    gint64 len;
    gst_element_query_duration(_data->source, GST_FORMAT_TIME, &len);
    std::cout << "DURA:" << len << std::endl;
    if (len <= time) {
        std::cerr << "=================== NOT ENOUGH DATA!!!!!" << std::endl;
        return;
    }
    //
    gst_element_set_state(_data->source, GST_STATE_PLAYING);
    std::cout << "SEEK:" << time << std::endl;

    GstEvent* event = gst_event_new_seek(1.0, GST_FORMAT_TIME, static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_SKIP), GST_SEEK_TYPE_SET, time, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);

    gst_element_send_event(_data->source, event);
}
