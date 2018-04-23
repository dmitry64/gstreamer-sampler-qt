#include "gstreamerthreadworker.h"
#include <unistd.h>
#include <iomanip>
#include <QApplication>

static void seek_to_time(GstElement* pipeline, gint64 time_nanoseconds)
{
    if (!gst_element_seek(pipeline, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET, time_nanoseconds, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE)) {
        g_print("Seek failed!\n");
    }
}

gboolean timeout_callback(gpointer dataptr)
{
    ProgramData* data = (ProgramData*) dataptr;

    data->worker->handleCommands(data);

    return TRUE;
}

GstFlowReturn on_new_audio_sample_from_sink(GstElement* elt, ProgramData* data)
{
    GstSample* sample;
    GstBuffer *app_buffer, *buffer;
    GstElement* source;
    GstMapInfo info;
    GstFlowReturn ret;

    sample = gst_app_sink_pull_sample(GST_APP_SINK(elt));
    buffer = gst_sample_get_buffer(sample);
    gst_buffer_map(buffer, &info, GST_MAP_READ);
    // std::cout << "InfoSize:" << info.size << std::endl;

    std::vector<signed short> outputVector(info.size / 2);
    memcpy(outputVector.data(), info.data, info.size);

    data->worker->addSampleAndTimestamp(outputVector, buffer->pts, buffer->duration);
    data->worker->sendSignalBuffers();


    data->worker->sendAudioSample(outputVector);
    // std::cout << "Duration:" << buffer->duration / 1000.0f / 1000.0f << "ms" << std::endl;

    app_buffer = gst_buffer_copy(buffer);

    gst_sample_unref(sample);

    source = gst_bin_get_by_name(GST_BIN(data->audiosink), "outputaudiosource");
    ret = gst_app_src_push_buffer(GST_APP_SRC(source), app_buffer);
    gst_object_unref(source);

    return ret;
}

GstFlowReturn on_new_video_sample_from_sink(GstElement* elt, ProgramData* data)
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
    /* std::cout << "Frame size:" << info.size << " bytes, " << info.size / 1024 << " KB, " << info.size / 1024 / 1024 << " MB"
               << " PTS:" << std::setprecision(3) << second << "s" << std::endl;*/

    /*if (second > 1.0f) {
        g_main_loop_quit(data->loop);
    }*/

    gst_buffer_unmap(buffer, &info);

    gst_sample_unref(sample);


    return GST_FLOW_OK;
}

static gboolean on_source_message(GstBus* bus, GstMessage* message, ProgramData* data)
{
    GstElement* source;

    switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_EOS:
        g_print("The source got dry\n");
        source = gst_bin_get_by_name(GST_BIN(data->audiosink), "outputaudiosource");
        gst_app_src_end_of_stream(GST_APP_SRC(source));
        gst_object_unref(source);
        break;
    case GST_MESSAGE_ERROR:
        g_print("Received error\n");
        g_main_loop_quit(data->loop);
        break;
    default:
        break;
    }
    return TRUE;
}

static gboolean on_audio_sink_message(GstBus* bus, GstMessage* message, ProgramData* data)
{
    switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_EOS:
        g_print("Finished playback\n");
        g_main_loop_quit(data->loop);
        break;
    case GST_MESSAGE_ERROR:
        g_print("Received error\n");
        g_main_loop_quit(data->loop);
        break;
    default:
        break;
    }
    return TRUE;
}

static gboolean gst_my_filter_sink_event(GstPad* pad, GstObject* parent, GstEvent* event)
{
    gboolean ret;
    std::cout << "EVENT:" << GST_EVENT_TYPE_NAME(event) << std::endl;

    switch (GST_EVENT_TYPE(event)) {
    default:
        /* just call the default handler */
        ret = gst_pad_event_default(pad, parent, event);
        break;
    }
    return ret;
}


void GstreamerThreadWorker::run()
{
    mainLoop();
}

void GstreamerThreadWorker::handleCommands(ProgramData* data)
{
    _mutex.lock();
    for (int i = 0; i < 50 && !_commands.empty(); ++i) {
        Command* command = _commands.front();

        command->handleCommand(data);

        delete command;
        _commands.pop();
    }
    _mutex.unlock();
}

void GstreamerThreadWorker::addSampleAndTimestamp(const std::vector<signed short>& samples, GstClockTime time, GstClockTime duration)
{
    _waveThread.addSampleAndTimestamp(samples, time, duration);
}

void GstreamerThreadWorker::sendSignalBuffers()
{
    std::vector<signed short> samples;
    while (_waveThread.getNextBuffer(samples)) {
        emit sampleCutReady(samples);
    }
    unsigned int coord;
    while (_waveThread.getNextCoord(coord)) {
        emit coordReady(coord);
    }
}

GstreamerThreadWorker::GstreamerThreadWorker(QObject* parent)
    : QThread(parent)
{
}

void GstreamerThreadWorker::sendAudioSample(std::vector<signed short>& samples)
{
    emit sampleReady(samples);
}

void GstreamerThreadWorker::sendVideoSample(std::vector<unsigned char>& frame)
{
    emit frameReady(frame);
}

void GstreamerThreadWorker::mainLoop()
{
    ProgramData* data = NULL;
    gchar* string = NULL;
    GstBus* bus = NULL;
    GstElement* myaudiosink = NULL;
    GstElement* myvideosink = NULL;
    GstElement* outputaudiosource = NULL;

    std::cout << "Initializing..." << std::endl;
    gst_init(0, 0);

    data = g_new0(ProgramData, 1);
    data->worker = this;

    data->loop = g_main_loop_new(NULL, FALSE);
    g_timeout_add(100, timeout_callback, data);

    string = g_strdup_printf
        // good ("filesrc location=/workspace/gst-qt/samples/test.avi ! avidemux name=d ! queue ! xvimagesink d. ! audioconvert ! audioresample ! appsink caps=\"%s\" name=myaudiosink", filename, audio_caps);
        // ("filesrc location=/workspace/gst-qt/samples/bunny.mkv ! matroskademux ! h264parse ! avdec_h264 ! videorate ! videoconvert ! videoscale ! video/x-raw,format=RGB16,width=640,height=480 ! appsink name=myvideosink sync=true");
        ("rtspsrc location=rtsp://192.168.1.100/H.264/media.smp sync=true name=demux demux. ! queue ! capsfilter caps=\"application/x-rtp,media=video\" ! rtph264depay ! h264parse ! decodebin ! videoconvert ! videoscale ! "
         "video/x-raw,format=RGB,width=1920,height=1080 ! appsink "
         "name=myvideosink "
         "caps=\"video/x-raw,format=RGB,width=1920,height=1080\" sync=true demux. ! queue ! capsfilter caps=\"application/x-rtp,media=audio\" ! decodebin !"
         "audioconvert ! audioresample ! audio/x-raw,format=S16LE,channels=1,rate=8000,layout=interleaved ! appsink "
         "caps=\"audio/x-raw,format=S16LE,channels=1,rate=8000,layout=interleaved\" "
         "name=myaudiosink sync=true");
    std::cout << "Pipeline string: \n" << string << std::endl;
    data->source = gst_parse_launch(string, NULL);
    g_free(string);
    std::cout << "Created pipeline..." << std::endl;

    if (data->source == NULL) {
        g_print("Bad source\n");
        return;
    }

    bus = gst_element_get_bus(data->source);
    gst_bus_add_watch(bus, (GstBusFunc) on_source_message, data);
    gst_object_unref(bus);

    myaudiosink = gst_bin_get_by_name(GST_BIN(data->source), "myaudiosink");
    g_object_set(G_OBJECT(myaudiosink), "emit-signals", TRUE, "sync", TRUE, NULL);
    g_signal_connect(myaudiosink, "new-sample", G_CALLBACK(on_new_audio_sample_from_sink), data);
    gst_object_unref(myaudiosink);

    std::cout << "Audio sink ready..." << std::endl;

    myvideosink = gst_bin_get_by_name(GST_BIN(data->source), "myvideosink");
    g_object_set(G_OBJECT(myvideosink), "emit-signals", TRUE, "sync", TRUE, NULL);
    g_signal_connect(myvideosink, "new-sample", G_CALLBACK(on_new_video_sample_from_sink), data);
    gst_object_unref(myvideosink);

    std::cout << "Video sink ready..." << std::endl;

    GstPad* appsinkPad = gst_element_get_static_pad(GST_ELEMENT(myvideosink), "sink");
    gst_pad_set_event_function(appsinkPad, gst_my_filter_sink_event);

    string = g_strdup_printf("appsrc name=outputaudiosource caps=\"audio/x-raw,format=S16LE,channels=1,rate=48000,layout=interleaved\" ! autoaudiosink sync=true");
    data->audiosink = gst_parse_launch(string, NULL);
    g_free(string);

    if (data->audiosink == NULL) {
        g_print("Bad sink\n");
        return;
    }

    outputaudiosource = gst_bin_get_by_name(GST_BIN(data->audiosink), "outputaudiosource");
    g_object_set(outputaudiosource, "format", GST_FORMAT_TIME, NULL);
    gst_object_unref(outputaudiosource);

    bus = gst_element_get_bus(data->audiosink);
    gst_bus_add_watch(bus, (GstBusFunc) on_audio_sink_message, data);
    gst_object_unref(bus);

    gst_element_set_state(data->audiosink, GST_STATE_PLAYING);
    gst_element_set_state(data->source, GST_STATE_PLAYING);

    _waveThread.startAnalysys();
    std::cout << "Starting main loop..." << std::endl;
    g_main_loop_run(data->loop);
    std::cout << "Main loop finished..." << std::endl;

    gst_element_set_state(data->source, GST_STATE_NULL);
    gst_element_set_state(data->audiosink, GST_STATE_NULL);

    gst_object_unref(data->source);
    gst_object_unref(data->audiosink);
    g_main_loop_unref(data->loop);
    g_free(data);

    std::cout << "GStreamer thread finished..." << std::endl;
    emit finished();
}

void GstreamerThreadWorker::stopWorker()
{
    std::cout << "Stop worker" << std::endl;
    StopCommand* command = new StopCommand();
    _mutex.lock();
    _commands.push(command);
    _mutex.unlock();
}

void GstreamerThreadWorker::seekPipeline(int pos)
{
    std::cout << "Seek command to:" << pos << std::endl;
    SeekCommand* command = new SeekCommand(pos);
    _mutex.lock();
    _commands.push(command);
    _mutex.unlock();
}
