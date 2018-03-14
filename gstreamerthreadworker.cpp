#include "gstreamerthreadworker.h"
#include <unistd.h>

GstFlowReturn on_new_audio_sample_from_sink(GstElement* elt, GstreamerThreadWorker::ProgramData* data)
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
    data->worker->sendAudioSample(outputVector);

    app_buffer = gst_buffer_copy(buffer);

    gst_sample_unref(sample);

    source = gst_bin_get_by_name(GST_BIN(data->audiosink), "outputaudiosource");
    ret = gst_app_src_push_buffer(GST_APP_SRC(source), app_buffer);
    gst_object_unref(source);

    return ret;
}

GstFlowReturn on_new_video_sample_from_sink(GstElement* elt, GstreamerThreadWorker::ProgramData* data)
{
    GstSample* sample;
    GstBuffer *app_buffer, *buffer;
    GstMapInfo info;

    sample = gst_app_sink_pull_sample(GST_APP_SINK(elt));
    buffer = gst_sample_get_buffer(sample);
    gst_buffer_map(buffer, &info, GST_MAP_READ);

    std::vector<unsigned char> outputVector(info.size);
    memcpy(outputVector.data(), info.data, info.size);
    data->worker->sendVideoSample(outputVector);

    // std::cout << "InfoSize:" << info.size << std::endl;
    // app_buffer = gst_buffer_copy(buffer);
    gst_buffer_unmap(buffer, &info);

    gst_sample_unref(sample);


    return GST_FLOW_OK;
}

static gboolean on_source_message(GstBus* bus, GstMessage* message, GstreamerThreadWorker::ProgramData* data)
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

static gboolean on_audio_sink_message(GstBus* bus, GstMessage* message, GstreamerThreadWorker::ProgramData* data)
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

GstreamerThreadWorker::GstreamerThreadWorker(QObject* parent)
    : QObject(parent)
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

    string = g_strdup_printf
        // good ("filesrc location=/workspace/gst-qt/samples/test.avi ! avidemux name=d ! queue ! xvimagesink d. ! audioconvert ! audioresample ! appsink caps=\"%s\" name=myaudiosink", filename, audio_caps);
        // ("filesrc location=/workspace/gst-qt/samples/bunny.mkv ! matroskademux ! h264parse ! avdec_h264 ! videorate ! videoconvert ! videoscale ! video/x-raw,format=RGB16,width=640,height=480 ! appsink name=myvideosink sync=true");
        ("filesrc location=/workspace/gst-qt/samples/bunny.mkv ! matroskademux name=d ! queue ! h264parse ! vaapih264dec ! videorate ! videoconvert ! videoscale ! video/x-raw,format=RGB,width=640,height=420,framerate=30/1 ! appsink name=myvideosink "
         "caps=\"video/x-raw,format=RGB,width=640,height=420,framerate=30/1\" sync=true d. ! queue ! opusdec !"
         "audioconvert ! audioresample ! audio/x-raw,format=S16LE,channels=1,rate=48000,layout=interleaved ! appsink "
         "caps=\"audio/x-raw,format=S16LE,channels=1,rate=48000,layout=interleaved\" "
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
