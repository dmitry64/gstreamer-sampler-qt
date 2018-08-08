#include "commands.h"
#include "gstreamerthreadworker.h"
#include "gstreamervideoplayer.hpp"
#include <cassert>
#include <unistd.h>

void PlayerSeekCommand::handleCommand(PlayerProgramData* data)
{
    std::cout << "POS:" << _pos / 1000.0f / 1000.0f / 1000.0f << "s" << std::endl;
    sync();
    gint64 len;
    gst_element_query_duration(data->source, GST_FORMAT_TIME, &len);
    std::cout << "DURA:" << len << std::endl;
    if (len <= _pos) {
        std::cerr << "=================== NOT ENOUGH DATA!!!!!" << std::endl;
        return;
    }

    gst_element_set_state(data->source, GST_STATE_PLAYING);
    std::cout << "SEEK:" << _pos << std::endl;
    if (!gst_element_seek(data->source, 1.0, GST_FORMAT_TIME, static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_TRICKMODE_KEY_UNITS | GST_SEEK_FLAG_TRICKMODE_NO_AUDIO), GST_SEEK_TYPE_SET, _pos, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE)) {
        std::cout << "seek failed" << std::endl;
        assert(false);
    }
}

void WorkerStopCommand::handleCommand(ProgramData* data)
{
    std::cout << "Stop worker pipeline..." << std::endl;
    g_main_loop_quit(data->loop);
    data->worker->stopHandlerTimeout();
}

void PlayerStopCommand::handleCommand(PlayerProgramData* data)
{
    std::cout << "Stop player pipeline..." << std::endl;
    g_main_loop_quit(data->loop);
    data->worker->stopHandlerTimeout();
}

void PlayerPlayCommand::handleCommand(PlayerProgramData* data)
{
    gst_element_set_state(data->source, GST_STATE_PLAYING);
    GstEvent* seek_event;
    seek_event = gst_event_new_seek(1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_KEY_UNIT, GST_SEEK_TYPE_SET, 0, GST_SEEK_TYPE_NONE, 0);
    gst_element_send_event(data->source, seek_event);
}
