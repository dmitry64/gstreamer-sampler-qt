#ifndef WAVEANALYZER_HPP
#define WAVEANALYZER_HPP

#include <gst/gstbuffer.h>
#include <vector>
#include <queue>
#include <array>

const unsigned int SAMPLE_SIZE = 65 * 8 + 4;
const unsigned int FRONT_THRESHOLD = 18000;
const unsigned int SAMPLE_OFFSET = 4;

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


private:
    std::array<SignalsBuffer, 2> _buffers;
    std::array<GstClockTime, 2> _timestamps;
    std::queue<SignalsBuffer> _outputBuffers;

    unsigned int _lastPos;

private:
    SampleType getSampleAt(unsigned int index);
    unsigned int getSamplesAvailable();
    unsigned int getBufferIndexBySampleIndex(unsigned int index);
    SignalsBuffer cutSlice(const SignalSlice& slice);
    bool findNextSignal(SignalSlice& slice);

public:
    WaveAnalyzer();
    void addBufferWithTimecode(const SignalsBuffer& samples, GstClockTime timestamp);
    bool getNextBuffer(SignalsBuffer& output);
};

#endif  // WAVEANALYZER_HPP
