#ifndef WAVEANALYZER_HPP
#define WAVEANALYZER_HPP

#include <gst/gstbuffer.h>
#include <vector>
#include <queue>
#include <array>

const unsigned int SAMPLE_SIZE = 550;
const int FRONT_THRESHOLD = 12000;
const unsigned int SAMPLE_OFFSET = 100;

class WaveAnalyzer
{
    using SampleType = signed short;
    using SignalsBuffer = std::vector<WaveAnalyzer::SampleType>;

    struct SignalSlice
    {
        unsigned int begin;
        unsigned int end;
        unsigned int bufferIndex;
    };

    struct TWaveFront
    {
        int xl;
        int xr;
        int yl;
        int yr;
    };


private:
    SignalsBuffer _buffers;
    std::vector<GstClockTime> _timeBuffers;

    std::queue<SignalsBuffer> _outputBuffers;


private:
    void analyze();
    bool decodeBuffer(const SignalsBuffer& buffer, unsigned int& result);
    bool checkFront(const TWaveFront& front);

public:
    WaveAnalyzer();
    void addBufferWithTimecode(const SignalsBuffer& samples, GstClockTime timestamp, GstClockTime duration);
    bool getNextBuffer(SignalsBuffer& output);
};

#endif  // WAVEANALYZER_HPP
