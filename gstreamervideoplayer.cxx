#include "gstreamervideoplayer.hpp"
#include <unistd.h>
#include <iomanip>
#include <QApplication>
#include "settings.h"

gboolean timeout_callback_player(gpointer dataptr)
{
    PlayerProgramData* data = (PlayerProgramData*) dataptr;
    Q_ASSERT(data->source);
    Q_ASSERT(data->loop);
    Q_ASSERT(data->worker);
    data->worker->handleCommands(data);

    return TRUE;
}

GstFlowReturn on_new_video_sample_from_sink_player(GstElement* elt, PlayerProgramData* data)
{
    GstSample* sample;
    GstBuffer* buffer;
    GstMapInfo info;

    sample = gst_app_sink_pull_sample(GST_APP_SINK(elt));
    buffer = gst_sample_get_buffer(sample);
    gst_buffer_map(buffer, &info, GST_MAP_READ);

    std::vector<unsigned char> outputVector(info.size);
    memcpy(outputVector.data(), info.data, info.size);
    data->worker->sendVideoSample(outputVector);
    gst_buffer_unmap(buffer, &info);
    gst_sample_unref(sample);
    std::cerr << "FRAME!" << std::endl;

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
        g_main_loop_quit(data->loop);
        break;
    case GST_MESSAGE_STATE_CHANGED:
        g_print("State changed\n");
        break;
    default:
        break;
    }
    return TRUE;
}

static gboolean gst_my_filter_sink_event_player(GstPad* pad, GstObject* parent, GstEvent* event)
{
    std::cout << "PLAYER EVENT:" << GST_EVENT_TYPE_NAME(event) << std::endl;

    switch (GST_EVENT_TYPE(event)) {
    case GST_EVENT_EOS:
        std::cerr << "PLAYER EOS EVENT!" << std::endl;
        break;
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

void GstreamerVideoPlayer::run()
{
    mainLoop();
}

void GstreamerVideoPlayer::handleCommands(PlayerProgramData* data)
{
    _mutex.lock();
    for (int i = 0; i < 50 && !_commands.empty(); ++i) {
        PlayerCommand* command = _commands.front();

        command->handleCommand(data);

        delete command;
        _commands.pop();
    }
    _mutex.unlock();
}


GstreamerVideoPlayer::GstreamerVideoPlayer(QObject* parent)
    : QThread(parent)
    , _timeoutId(0)
    , _currentFileName("video")
{
}

void GstreamerVideoPlayer::sendVideoSample(std::vector<unsigned char>& frame)
{
    emit frameReady(frame);
}

void GstreamerVideoPlayer::mainLoop()
{
    PlayerProgramData* data = NULL;
    gchar* string = NULL;
    GstBus* bus = NULL;
    GstElement* myplayervideosink = NULL;

    std::cout << "Initializing..." << std::endl;
    gst_init(0, 0);

    data = g_new0(PlayerProgramData, 1);
    data->worker = this;

    data->loop = g_main_loop_new(NULL, FALSE);
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

    QString launchString("filesrc location=" + currentFilePath + "/" + _currentFileName + QString::number(id)
                         + ".ts ! tsdemux ! queue ! capsfilter caps=\"video/x-h264\" ! h264parse ! decodebin ! videoconvert ! videoscale ! "
                           "video/x-raw,format=RGB,width=1280,height=720 ! appsink name=myplayervideosink sync=true");


    // string = g_strdup_printf
    // good ("filesrc location=/workspace/gst-qt/samples/test.avi ! avidemux name=d ! queue ! xvimagesink d. ! audioconvert ! audioresample ! appsink caps=\"%s\" name=myaudiosink", filename, audio_caps);
    // ("filesrc location=/workspace/gst-qt/samples/bunny.mkv ! matroskademux ! h264parse ! avdec_h264 ! videorate ! videoconvert ! videoscale ! video/x-raw,format=RGB,width=1280,height=720 ! appsink name=myvideosink sync=true");
    //(,  // filesrc location=file%d.ts use-mmap=false ! tsdemux
    // id);
    std::cout << "Pipeline string: \n" << launchString.toStdString().c_str() << std::endl;  // filesrc location=file%d.ts ! tsparse ! tsdemux
    data->source = gst_parse_launch(launchString.toStdString().c_str(), NULL);
    g_free(string);
    std::cout << "Created pipeline..." << std::endl;

    if (data->source == NULL) {
        g_print("Bad source\n");
        return;
    }

    bus = gst_element_get_bus(data->source);
    gst_bus_add_watch(bus, (GstBusFunc) on_source_message_player, data);
    gst_object_unref(bus);

    myplayervideosink = gst_bin_get_by_name(GST_BIN(data->source), "myplayervideosink");
    g_object_set(G_OBJECT(myplayervideosink), "emit-signals", TRUE, "sync", TRUE, NULL);
    g_signal_connect(myplayervideosink, "new-sample", G_CALLBACK(on_new_video_sample_from_sink_player), data);
    gst_object_unref(myplayervideosink);

    std::cout << "Player video sink ready..." << std::endl;

    GstPad* appsinkPad = gst_element_get_static_pad(GST_ELEMENT(myplayervideosink), "sink");
    gst_pad_set_event_function(appsinkPad, gst_my_filter_sink_event_player);

    gst_element_set_state(data->source, GST_STATE_PAUSED);

    _timeoutId = g_timeout_add(50, timeout_callback_player, data);
    std::cout << "Starting main loop..." << std::endl;
    g_main_loop_run(data->loop);
    std::cout << "Main loop finished..." << std::endl;

    gst_element_set_state(data->source, GST_STATE_NULL);

    gst_object_unref(data->source);
    g_main_loop_unref(data->loop);
    g_free(data);

    std::cout << "GStreamer thread finished..." << std::endl;
    emit finished();
}

void GstreamerVideoPlayer::stopWorker()
{
    std::cout << "Stop player" << std::endl;
    PlayerStopCommand* command = new PlayerStopCommand();
    _mutex.lock();
    _commands.push(command);
    _mutex.unlock();
}

void GstreamerVideoPlayer::playStream()
{
    std::cout << "Play player" << std::endl;
    PlayerPlayCommand* command = new PlayerPlayCommand();
    _mutex.lock();
    _commands.push(command);
    _mutex.unlock();
}

void GstreamerVideoPlayer::showFrameAt(GstClockTime time)
{
    PlayerSeekCommand* command = new PlayerSeekCommand(time);
    _mutex.lock();
    _commands.push(command);
    _mutex.unlock();
}

void GstreamerVideoPlayer::stopHandlerTimeout()
{
    if (_timeoutId) {
        g_source_remove(_timeoutId);
        _timeoutId = 0;
    }
}
