#include "commands.h"
#include "gstreamerthreadworker.h"
#include "gstreamervideoplayer.hpp"

void PlayerSeekCommand::handleCommand(PlayerProgramData* data)
{
    GstEvent* seek_event;
    gboolean res = FALSE;
    std::cout << "POS:" << _pos / 1000.0f / 1000.0f / 1000.0f << "s" << std::endl;
    gst_element_set_state(data->source, GST_STATE_PLAYING);
    // gst_element_set_state(data->source, GST_STATE_PAUSED);
    std::cout << "SEEK" << std::endl;
    seek_event = gst_event_new_seek(1.0, GST_FORMAT_TIME, static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT), GST_SEEK_TYPE_SET, 0, GST_SEEK_TYPE_SET, 1);
    std::cout << "SEND" << std::endl;
    res = gst_element_send_event(data->source, seek_event);
    // gst_element_set_state(data->source, GST_STATE_PLAYING);
    // gst_element_seek(data->source, 1.0, GST_FORMAT_TIME, static_cast<GstSeekFlags>(GST_SEEK_FLAG_KEY_UNIT), GST_SEEK_TYPE_SET, 0, GST_SEEK_TYPE_SET, _pos);

    if (!res) {
        std::cout << "seek failed" << std::endl;
    }
    std::cout << " EVENT SENT!" << std::endl;
    gst_element_get_state(data->source, NULL, NULL, GST_CLOCK_TIME_NONE);
}

void WorkerStopCommand::handleCommand(ProgramData* data)
{
    std::cout << "Stop pipeline..." << std::endl;
    g_main_loop_quit(data->loop);
}

void PlayerStopCommand::handleCommand(PlayerProgramData* data)
{
    std::cout << "Stop pipeline..." << std::endl;
    g_main_loop_quit(data->loop);
}

void PlayerPlayCommand::handleCommand(PlayerProgramData* data)
{
    gst_element_set_state(data->source, GST_STATE_PLAYING);
    GstEvent* seek_event;
    seek_event = gst_event_new_seek(1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_KEY_UNIT, GST_SEEK_TYPE_SET, 0, GST_SEEK_TYPE_NONE, 0);
    gst_element_send_event(data->source, seek_event);
}
