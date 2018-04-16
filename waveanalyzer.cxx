#include "waveanalyzer.hpp"

#include <iostream>

WaveAnalyzer::SampleType WaveAnalyzer::getSampleAt(unsigned int index)
{
    for (const auto& buffer : _buffers) {
        if (index < buffer.size()) {
            return buffer[index];
        }
        else {
            index -= buffer.size();
        }
    }
    return -32000;
}

unsigned int WaveAnalyzer::getSamplesAvailable()
{
    unsigned int size = 0;
    for (const auto& buffer : _buffers) {
        size += buffer.size();
    }
    return size;
}

unsigned int WaveAnalyzer::getBufferIndexBySampleIndex(unsigned int index)
{
    unsigned int bufferIndex = 0;
    for (const auto& buffer : _buffers) {
        if (index < buffer.size()) {
            return bufferIndex;
        }
        else {
            index -= buffer.size();
        }
        ++bufferIndex;
    }
    return bufferIndex;
}

WaveAnalyzer::SignalsBuffer WaveAnalyzer::cutSlice(const WaveAnalyzer::SignalSlice& slice)
{
    unsigned int size = slice.end - slice.begin;
    SignalsBuffer buff(size);
    unsigned int buffIndex = 0;
    for (unsigned int i = slice.begin; i < slice.end; ++i) {
        buff[buffIndex] = getSampleAt(i);
        ++buffIndex;
    }
    return buff;
}

WaveAnalyzer::WaveAnalyzer()
    : _lastPos(0)
{
}

void WaveAnalyzer::addBufferWithTimecode(const SignalsBuffer& samples, GstClockTime timestamp)
{
    if (_buffers[0].size() > 0) {
        _lastPos -= _buffers[0].size();
    }
    _lastPos = 0;
    std::swap(_buffers[0], _buffers[1]);
    _buffers[1] = samples;
    std::swap(_timestamps[0], _timestamps[1]);
    _timestamps[1] = timestamp;

    if (getSamplesAvailable() > 1100) {
        SignalSlice slice;
        while (findNextSignal(slice)) {
            std::cout << "NEW SLICE!!! begin:" << slice.begin << " end:" << slice.end << " index:" << slice.bufferIndex << std::endl;
            /*for (int i = slice.begin; i < slice.end; ++i) {
                std::cout << getSampleAt(i) << " ";
            }
            std::cout << std::endl;*/
            _outputBuffers.push(cutSlice(slice));
        }
    }
}

bool WaveAnalyzer::getNextBuffer(WaveAnalyzer::SignalsBuffer& output)
{
    if (!_outputBuffers.empty()) {
        output = _outputBuffers.front();
        _outputBuffers.pop();
        return true;
    }
    return false;
}

bool WaveAnalyzer::findNextSignal(WaveAnalyzer::SignalSlice& slice)
{
    unsigned int size = getSamplesAvailable();
    SampleType prevSample = getSampleAt(_lastPos);
    for (unsigned int i = _lastPos + 1; i < size; ++i) {
        SampleType nextSample = getSampleAt(i);
        if (std::abs(prevSample - nextSample) > FRONT_THRESHOLD) {
            std::cout << "Ramp:" << prevSample - nextSample << std::endl;
            unsigned int beginPos = std::max(0, static_cast<int>(i) - static_cast<int>(SAMPLE_OFFSET));
            unsigned int endPos = beginPos + SAMPLE_SIZE;
            if (endPos > size) {
                return false;
            }
            else {
                SignalSlice newSlice;
                newSlice.begin = beginPos;
                newSlice.end = endPos;
                newSlice.bufferIndex = getBufferIndexBySampleIndex(beginPos);
                slice = newSlice;
                _lastPos = std::min(endPos + 40, size);
                return true;
            }
        }
    }


    return false;
}
