#ifndef COMMANDS_H
#define COMMANDS_H

#include <gst/gst.h>

class ProgramData;
class PlayerProgramData;

class WorkerCommand
{
public:
    WorkerCommand() {}
    virtual ~WorkerCommand() {}
    virtual void handleCommand(ProgramData* data) = 0;
};


class PlayerCommand
{
public:
    PlayerCommand(){};
    virtual ~PlayerCommand() {}
    virtual void handleCommand(PlayerProgramData* data) = 0;
};

class PlayerSeekCommand : public PlayerCommand
{
    GstClockTime _pos;

public:
    PlayerSeekCommand(GstClockTime pos)
        : _pos(pos)
    {
    }
    void handleCommand(PlayerProgramData* data);
};

class PlayerStopCommand : public PlayerCommand
{
public:
    PlayerStopCommand() {}
    void handleCommand(PlayerProgramData* data);
};

class PlayerPlayCommand : public PlayerCommand
{
public:
    PlayerPlayCommand() {}
    void handleCommand(PlayerProgramData* data);
};

#endif  // COMMANDS_H
