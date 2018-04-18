#include "waveanalyzer.hpp"

#include <iostream>
#include <cassert>
#include <algorithm>
#include <numeric>

void WaveAnalyzer::analyze()
{
    while (true) {
        if (_buffers.size() > BASELINE_WINDOW_WIDTH * 4) {
            SignalsBuffer::iterator baseLine = _buffers.end();
            bool found = false;

            for (SignalsBuffer::iterator it = (_buffers.begin() + BASELINE_WINDOW_WIDTH); (it + BASELINE_WINDOW_WIDTH) <= _buffers.end(); ++it) {
                SignalsBuffer::iterator leftBegin = it - BASELINE_WINDOW_WIDTH;
                SignalsBuffer::iterator leftEnd = it;
                SignalsBuffer::iterator rightBegin = it;
                SignalsBuffer::iterator rightEnd = it + BASELINE_WINDOW_WIDTH;
                double leftAvg = std::accumulate(leftBegin, leftEnd, 0.0) / static_cast<double>(BASELINE_WINDOW_WIDTH);
                double rightAvg = std::accumulate(rightBegin, rightEnd, 0.0) / static_cast<double>(BASELINE_WINDOW_WIDTH);


                if (std::abs(leftAvg - rightAvg) < AvTOL) {
                    baseLine = it;
                    baseLine++;
                    // std::cout << "BASELINE" << std::endl;
                    // std::cout << "WINDOW:" << leftAvg << " " << rightAvg << std::endl;
                    break;
                }
                else {
                    // std::cout << "WINDOW:" << leftAvg << " " << rightAvg << " res:" << std::abs(leftAvg - rightAvg) << std::endl;
                }
            }

            if (baseLine == _buffers.end()) {
                std::cout << "Baseline not found!" << std::endl;
                _buffers.clear();
                _timeBuffers.clear();

                return;
            }
            SignalsBuffer::iterator beginIt = _buffers.end();
            SignalsBuffer::iterator endIt = _buffers.end();

            for (SignalsBuffer::iterator it = baseLine; (it + NEG_FRONT_WINDOW_WIDTH) <= _buffers.end(); ++it) {
                SignalsBuffer::iterator leftBegin = it - NEG_FRONT_WINDOW_WIDTH;
                SignalsBuffer::iterator leftEnd = it;
                SignalsBuffer::iterator rightBegin = it;
                SignalsBuffer::iterator rightEnd = it + NEG_FRONT_WINDOW_WIDTH;
                int leftAvg = std::accumulate(leftBegin, leftEnd, 0) / static_cast<int>(NEG_FRONT_WINDOW_WIDTH);
                int rightAvg = std::accumulate(rightBegin, rightEnd, 0) / static_cast<int>(NEG_FRONT_WINDOW_WIDTH);
                int diff = leftAvg - rightAvg;

                if (diff > FRONT_THRESHOLD) {
                    it += 2;
                    beginIt = it;
                    if (std::distance(beginIt, _buffers.end()) > SAMPLE_SIZE) {
                        endIt = beginIt + SAMPLE_SIZE;
                        found = true;
                        break;
                    }
                    else {
                        return;
                    }
                }
            }

            if (found) {
                SignalsBuffer buff;
                buff.insert(buff.begin(), beginIt, endIt);
                unsigned int res = 0;
                if (decodeBuffer(buff, res)) {
                    std::cout << "COORD:" << res << " ";
                }

                std::vector<GstClockTime>::iterator timeItBegin = _timeBuffers.begin() + std::distance(_buffers.begin(), beginIt);
                std::vector<GstClockTime>::iterator timeItEnd = _timeBuffers.begin() + std::distance(_buffers.begin(), endIt);

                GstClockTime bufferTime = timeItBegin.operator*();
                std::cout << "TIME:" << bufferTime << std::endl;
                _outputBuffers.push(buff);

                _buffers.erase(_buffers.begin(), endIt);
                _timeBuffers.erase(_timeBuffers.begin(), timeItEnd);
                found = false;
            }
            else {
                return;
            }
        }
        else {
            return;
        }
    }
}

bool WaveAnalyzer::decodeBuffer(const WaveAnalyzer::SignalsBuffer& buffer, unsigned int& result)
{
    unsigned int acode;
    int startpoint = 0;
    int wavesize = buffer.size();
    const WaveAnalyzer::SignalsBuffer& wave = buffer;

    int i = 0;
    int k = 0;
    int dy = 0;
    int w = 0;
    int State = 0;
    int xref = 0;
    int FrontsSize = 0;
    int CodeLENGTH = 65 * 8 + 4;
    int STOL = 2;

    std::array<TWaveFront, 66> Fronts;
    TWaveFront emptyFront;
    emptyFront.xl = 0;
    emptyFront.xr = 0;
    emptyFront.yl = 0;
    emptyFront.yr = 0;
    std::fill(Fronts.begin(), Fronts.end(), emptyFront);

    bool Result = false;

    assert(!((startpoint + CodeLENGTH) > wavesize));
    State = 1;
    i = startpoint;
    k = 0;

    while (i < (startpoint + CodeLENGTH)) {
        dy = wave[i + 1] - wave[i];
        if (std::abs(dy) < STOL) {
            w = 0;
        }
        else if (dy > 0) {
            w = 1;
        }
        else {
            w = 2;
        }

        switch (State) {
        case 1: {
            switch (w) {
            case 1: {
                Fronts[k].xl = i;  // k?
                Fronts[k].yl = wave[i];
                State = 2;
                break;
            }
            case 2: {
                Fronts[k].xl = i;
                Fronts[k].yl = wave[i];
                State = 3;
                break;
            }
            }
            break;
        }
        case 2: {
            switch (w) {
            case 0: {
                Fronts[k].xr = i;
                Fronts[k].yr = wave[i];
                if (checkFront(Fronts[k])) {
                    k++;
                }
                State = 1;
                break;
            }
            case 2: {
                Fronts[k].xr = i;
                Fronts[k].yr = wave[i];
                if (checkFront(Fronts[k])) {
                    k++;
                }
                Fronts[k].xl = i;        //
                Fronts[k].yl = wave[i];  //
                State = 3;
                break;
            }
            }
            break;
        }
        case 3: {
            switch (w) {
            case 0: {
                Fronts[k].xr = i;
                Fronts[k].yr = wave[i];
                if (checkFront(Fronts[k])) {
                    k++;
                    // Fronts[k+1].xl := i;  //
                    // Fronts[k+1].yl := wave[i]; //
                }
                State = 1;
                break;
            }
            case 1: {
                Fronts[k].xr = i;
                Fronts[k].yr = wave[i];
                if (checkFront(Fronts[k])) {
                    k++;
                }
                Fronts[k].xl = i;
                Fronts[k].yl = wave[i];
                State = 2;
                break;
            }
            }
            break;
        }
        }
        i++;
    }

    FrontsSize = k;
    xref = startpoint;
    acode = 0;
    i = 0;

    for (k = 0; k < FrontsSize; ++k) {
        if ((Fronts[k].xr > (xref + 12)) && (Fronts[k].xr < (xref + 20))) {
            acode = acode >> 1;
            if (Fronts[k].yl > Fronts[k].yr) {
                acode = acode | 0x80000000;  // 1 = '\'
            }
            xref = Fronts[k].xr;
            i++;
            if (i >= 32) break;
        }
    }
    if (i != 32) {
        Result = false;
        result = 0;
        std::cout << "Unable to decode! fronts:" << i << std::endl;
    }
    else {
        Result = true;
        result = acode;
    }
    return Result;
}

bool WaveAnalyzer::checkFront(const WaveAnalyzer::TWaveFront& front)
{
    bool Result = false;
    if (std::abs(front.yr - front.yl) >= AFTOL) {
        if ((front.xr - front.xl) <= DFTOL) {
            Result = true;
        }
    }
    return Result;
}

WaveAnalyzer::WaveAnalyzer() {}

void WaveAnalyzer::addBufferWithTimecode(const SignalsBuffer& samples, GstClockTime timestamp, GstClockTime duration)
{
    // std::cout << "NEW BUFFER" << std::endl;
    _buffers.insert(_buffers.end(), samples.begin(), samples.end());
    unsigned long size = samples.size();

    GstClockTime timePerSample = duration / size;

    for (unsigned long i = 0; i < size; ++i) {
        _timeBuffers.push_back(timestamp + i * timePerSample);
    }

    analyze();
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
