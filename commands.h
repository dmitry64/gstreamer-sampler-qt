#ifndef COMMANDS_H
#define COMMANDS_H


class ProgramData;

class Command
{
public:
    Command(){};
    virtual ~Command(){};
    virtual void handleCommand(ProgramData* data) = 0;
};

class SeekCommand : public Command
{
    long int _pos;

public:
    SeekCommand(long int pos)
        : _pos(pos)
    {
    }
    void handleCommand(ProgramData* data);
};

#endif  // COMMANDS_H
