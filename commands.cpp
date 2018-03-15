#include "commands.h"
#include "gstreamerthreadworker.h"


void SeekCommand::handleCommand(ProgramData* data)
{
    gint64 ns = _pos * 1000 * 1000;

    GstEvent* seek_event;
    std::cout << "POS:" << ns / 1000.0f / 1000.0f / 1000.0f << "s" << std::endl;
    seek_event = gst_event_new_seek(8.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET, ns, GST_SEEK_TYPE_NONE, 0);
    gst_element_send_event(data->source, seek_event);
}
