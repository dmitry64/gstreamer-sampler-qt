#include "gstreamervideoplayer.hpp"
#include <unistd.h>
#include <iomanip>
#include <QApplication>

static void seek_to_time_player(GstElement* pipeline, gint64 time_nanoseconds)
{
    if (!gst_element_seek(pipeline, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET, time_nanoseconds, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE)) {
        g_print("Seek failed!\n");
    }
}

gboolean timeout_callback_player(gpointer dataptr)
{
    PlayerProgramData* data = (PlayerProgramData*) dataptr;

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

    float second = (buffer->pts / 1000.0f / 1000.0f / 1000.0f);


    gst_buffer_unmap(buffer, &info);

    gst_sample_unref(sample);

    // gst_element_set_state(data->source, GST_STATE_READY);

    return GST_FLOW_OK;
}

static gboolean on_source_message_player(GstBus* bus, GstMessage* message, PlayerProgramData* data)
{
    GstElement* source;

    switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_EOS:
        g_print("The source got dry\n");

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
    gboolean ret;
    std::cout << "PLAYER EVENT:" << GST_EVENT_TYPE_NAME(event) << std::endl;

    switch (GST_EVENT_TYPE(event)) {
    default:
        /* just call the default handler */
        ret = gst_pad_event_default(pad, parent, event);
        break;
    }
    return ret;
}

void GstreamerVideoPlayer::setCameraType(const CameraType& cameraType)
{
    _cameraType = cameraType;
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
    GstElement* myvideosink = NULL;

    std::cout << "Initializing..." << std::endl;
    gst_init(0, 0);

    data = g_new0(PlayerProgramData, 1);
    data->worker = this;

    data->loop = g_main_loop_new(NULL, FALSE);
    g_timeout_add(100, timeout_callback_player, data);

    int id = static_cast<int>(_cameraType);
    string = g_strdup_printf
        // good ("filesrc location=/workspace/gst-qt/samples/test.avi ! avidemux name=d ! queue ! xvimagesink d. ! audioconvert ! audioresample ! appsink caps=\"%s\" name=myaudiosink", filename, audio_caps);
        //("filesrc location=/workspace/gst-qt/samples/bunny.mkv ! matroskademux ! h264parse ! avdec_h264 ! videorate ! videoconvert ! videoscale ! video/x-raw,format=RGB,width=1920,height=1080 ! appsink name=myvideosink sync=true");
        ("filesrc location=/workspace/gst-qt/samples/bunny.mkv ! matroskademux !  h264parse ! avdec_h264 ! videoconvert ! videoscale ! "
         "video/x-raw,format=RGB,width=1920,height=1080 ! appsink name=myvideosink sync=true",
         id);
    std::cout << "Pipeline string: \n" << string << std::endl;  // filesrc location=file%d.ts ! tsparse ! tsdemux
    data->source = gst_parse_launch(string, NULL);
    g_free(string);
    std::cout << "Created pipeline..." << std::endl;

    if (data->source == NULL) {
        g_print("Bad source\n");
        return;
    }

    bus = gst_element_get_bus(data->source);
    gst_bus_add_watch(bus, (GstBusFunc) on_source_message_player, data);
    gst_object_unref(bus);

    myvideosink = gst_bin_get_by_name(GST_BIN(data->source), "myvideosink");
    g_object_set(G_OBJECT(myvideosink), "emit-signals", TRUE, "sync", TRUE, NULL);
    g_signal_connect(myvideosink, "new-sample", G_CALLBACK(on_new_video_sample_from_sink_player), data);
    gst_object_unref(myvideosink);

    std::cout << "Video sink ready..." << std::endl;

    GstPad* appsinkPad = gst_element_get_static_pad(GST_ELEMENT(myvideosink), "sink");
    gst_pad_set_event_function(appsinkPad, gst_my_filter_sink_event_player);

    gst_element_set_state(data->source, GST_STATE_PAUSED);

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
    std::cout << "Show player at: " << time << std::endl;
    PlayerSeekCommand* command = new PlayerSeekCommand(time);
    _mutex.lock();
    _commands.push(command);
    _mutex.unlock();
}
