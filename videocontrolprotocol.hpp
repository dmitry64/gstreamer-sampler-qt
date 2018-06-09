#ifndef VIDEOCONTROLPROTOCOL_HPP
#define VIDEOCONTROLPROTOCOL_HPP

namespace VIDEO_PROTOCOL
{
const static unsigned int HEADER_SIZE = 2;
const static unsigned char MESSAGE_TYPE_START_REG = 0xf1;
const static unsigned char MESSAGE_TYPE_STOP_REG = 0xf2;
const static unsigned char MESSAGE_TYPE_VIEW_MODE = 0xf3;
const static unsigned char MESSAGE_TYPE_REALTIME_MODE = 0xf4;
const static unsigned char MESSAGE_TYPE_SHOW_COORD = 0xf5;
const static unsigned char MESSAGE_SIZE_SHOW_COORD = 0x06;
}  // namespace VIDEO_PROTOCOL


#endif  // VIDEOCONTROLPROTOCOL_HPP
