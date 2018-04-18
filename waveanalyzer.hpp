#ifndef WAVEANALYZER_HPP
#define WAVEANALYZER_HPP

#include <gst/gstbuffer.h>
#include <vector>
#include <queue>
#include <array>

const static unsigned int SAMPLE_SIZE = 550;
const static int FRONT_THRESHOLD = 12000;
const static unsigned int SAMPLE_OFFSET = 100;
const static int BASELINE_WINDOW_WIDTH = 8 * 2 - 3;
const static int NEG_FRONT_WINDOW_WIDTH = 4;
const static double AvTOL = 12;
const static int AFTOL = 32767 / 2;
const static int DFTOL = 4;

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
