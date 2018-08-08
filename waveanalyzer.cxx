#include "waveanalyzer.hpp"

#include <iostream>
#include <cassert>
#include <algorithm>
#include <numeric>
#include <fstream>
#include <bitset>
#include <iomanip>


void WaveAnalyzer::analyze()
{
    while (true) {
        if (getEnoughData()) {
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
                std::cout << "========= Baseline not found!" << std::endl;
                _buffers.clear();
                _timeBuffers.clear();

                return;
            }
            SignalsBuffer::iterator beginIt = _buffers.end();
            SignalsBuffer::iterator endIt = _buffers.end();

            for (SignalsBuffer::iterator it = baseLine; (it + NEG_FRONT_WINDOW_WIDTH) <= _buffers.end(); ++it) {
                SignalsBuffer::iterator leftBegin = it - NEG_FRONT_WINDOW_WIDTH;
                SignalsBuffer::iterator leftEnd = it + 1;
                SignalsBuffer::iterator rightBegin = it;
                SignalsBuffer::iterator rightEnd = it + NEG_FRONT_WINDOW_WIDTH + 1;
                int leftAvg = std::accumulate(leftBegin, leftEnd, 0) / static_cast<int>(NEG_FRONT_WINDOW_WIDTH);
                int rightAvg = std::accumulate(rightBegin, rightEnd, 0) / static_cast<int>(NEG_FRONT_WINDOW_WIDTH);
                int diff = leftAvg - rightAvg;

                if (diff > FRONT_THRESHOLD) {
                    // std::cout << "FRONT:" << diff << std::endl;
                    int min_val = it.operator*();
                    SignalsBuffer::iterator min_iter = it;
                    int count = 0;
                    for (SignalsBuffer::iterator it2 = it; it2 < _buffers.end() && count < 6; ++it2) {
                        if (it2.operator*() <= min_val) {
                            min_val = it2.operator*();
                            min_iter = it2;
                        }
                        else {
                            break;
                        }

                        ++count;
                    }
                    // it += 3;
                    beginIt = min_iter;
                    if (std::distance(beginIt, _buffers.end()) > SAMPLE_SIZE) {
                        endIt = beginIt + SAMPLE_SIZE;
                        found = true;
                        break;
                    }
                    else {
                        found = false;
                        return;
                    }
                }
            }

            if (found) {
                SignalsBuffer buff;
                buff.insert(buff.begin(), beginIt, endIt);
                unsigned int res = 0;


                std::vector<GstClockTime>::iterator timeItBegin = _timeBuffers.begin() + std::distance(_buffers.begin(), beginIt);
                std::vector<GstClockTime>::iterator timeItEnd = _timeBuffers.begin() + std::distance(_buffers.begin(), endIt);

                GstClockTime bufferTime = timeItBegin.operator*();
                if (decodeBuffer(buff, res)) {
                    // std::cout << "COORD:" << res << " bin:" << ((res >> 3) & 0x00000001) << ((res >> 2) & 0x00000001) << ((res >> 1) & 0x00000001) << (res & 0x00000001) << " ";
                    SyncPoint syncPoint;
                    syncPoint.coord = res;
                    syncPoint.time = bufferTime;
                    _syncPoints.push(syncPoint);
                    if (!_filename.empty()) {
                        writeSyncPoint(syncPoint);
                    }
                }
                else {
                    std::cout << "FAIL! :" << buff.size() << " " << res << " " << bufferTime << std::endl;
                }
                // std::cout << "TIME:" << bufferTime << std::endl;

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

std::string WaveAnalyzer::getFilename() const
{
    return _filename;
}

void WaveAnalyzer::setFilename(const std::string& filename)
{
    _filename = filename;
}

bool WaveAnalyzer::decodeBuffer(const WaveAnalyzer::SignalsBuffer& buffer, unsigned int& result)
{
    // return false;
    unsigned int acode = 0;
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
    int CodeLENGTH = 270;
    int STOL = 4;

    std::array<TWaveFront, 66> Fronts;
    TWaveFront emptyFront;
    emptyFront.xl = 0;
    emptyFront.xr = 0;
    emptyFront.yl = 0;
    emptyFront.yr = 0;
    std::fill(Fronts.begin(), Fronts.end(), emptyFront);

    bool Result = false;
    // std::cout << "wave:" << wavesize << " code:" << CodeLENGTH << " first:" << wave.at(0) << std::endl;
    assert(!((startpoint + CodeLENGTH) > wavesize));
    State = 1;
    i = startpoint;
    k = 0;


    while (i < (startpoint + CodeLENGTH)) {
        assert(k < Fronts.size());
        dy = wave.at(i + 1) - wave.at(i);
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
                assert(k < Fronts.size());
                Fronts.at(k).xl = i;  // k?
                Fronts.at(k).yl = wave.at(i);
                State = 2;
                break;
            }
            case 2: {
                assert(k < Fronts.size());
                Fronts.at(k).xl = i;
                Fronts.at(k).yl = wave.at(i);
                State = 3;
                break;
            }
            }
            break;
        }
        case 2: {
            switch (w) {
            case 0: {
                /*assert(k < Fronts.size());
                Fronts.at(k).xr = i;
                Fronts.at(k).yr = wave.at(i);
                if (checkFront(Fronts.at(k))) {
                    k++;
                }
                State = 1;*/
                break;
            }
            case 2: {
                assert(k < Fronts.size());
                Fronts.at(k).xr = i;
                Fronts.at(k).yr = wave.at(i);
                if (checkFront(Fronts.at(k))) {
                    k++;
                    assert(k < Fronts.size());
                    Fronts.at(k).xl = i;           //
                    Fronts.at(k).yl = wave.at(i);  //
                    State = 3;
                }

                break;
            }
            }
            break;
        }
        case 3: {
            switch (w) {
            case 0: {
                /*assert(k < Fronts.size());
                Fronts.at(k).xr = i;
                Fronts.at(k).yr = wave.at(i);
                if (checkFront(Fronts.at(k))) {
                    k++;
                    // Fronts[k+1].xl := i;  //
                    // Fronts[k+1].yl := wave.at(i); //
                }
                State = 1;*/
                break;
            }
            case 1: {
                assert(k < Fronts.size());
                Fronts.at(k).xr = i;
                Fronts.at(k).yr = wave.at(i);
                if (checkFront(Fronts.at(k))) {
                    k++;
                    assert(k < Fronts.size());
                    Fronts.at(k).xl = i;
                    Fronts.at(k).yl = wave.at(i);
                    State = 2;
                }

                break;
            }
            }
            break;
        }
        }
        i++;
    }

    FrontsSize = k;
    assert(k < Fronts.size());
    xref = startpoint;
    acode = 0;
    i = 0;

    for (int m = 0; m < FrontsSize; ++m) {
        assert(m < Fronts.size());
        if ((Fronts.at(m).xr > (xref + 6))) {
            acode = acode >> 1;
            if (Fronts.at(m).yl > Fronts.at(m).yr) {
                acode = acode | 0x80000000;  // 1 = '\'
            }
            xref = Fronts.at(m).xr;
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
    // std::cout << "Result:" << std::setw(10) << result << " bits:" << std::bitset<32>(acode) << std::endl;
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

WaveAnalyzer::WaveAnalyzer()
    : _outputFile("TESTSAMPLES4.bin")
{
    _outputFile.open(QIODevice::WriteOnly);
    _counter = 0;
}

void WaveAnalyzer::addBufferWithTimecode(QSharedPointer<std::vector<signed short>> samples, GstClockTime timestamp, GstClockTime duration)
{
    // std::cout << "NEW BUFFER" << std::endl;
    const std::vector<signed short>& currentBuffer = samples.operator*();
    Q_ASSERT(!currentBuffer.empty());
    _buffers.insert(_buffers.end(), currentBuffer.begin(), currentBuffer.end());
    unsigned long size = samples->size();

    if (_counter < 100) {
        _outputFile.write(reinterpret_cast<char*>(samples->data()), samples->size() * 2);
        _counter++;
    }
    else {
        if (_outputFile.isOpen()) {
            _outputFile.close();
        }
    }

    GstClockTime timePerSample = duration / size;

    for (unsigned long i = 0; i < size; ++i) {
        _timeBuffers.push_back(timestamp + i * timePerSample);
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

bool WaveAnalyzer::getNextCoord(unsigned int& coord, GstClockTime& time)
{
    if (!_syncPoints.empty()) {
        coord = _syncPoints.front().coord;
        time = _syncPoints.front().time;
        _syncPoints.pop();

        return true;
    }
    return false;
}

bool WaveAnalyzer::getEnoughData()
{
    return _buffers.size() > BASELINE_WINDOW_WIDTH * 4;
}

void WaveAnalyzer::dumpToFile()
{
    if (_syncPoints.size() > 30) {
        while (!_syncPoints.empty()) {
            _syncPoints.pop();
        }
    }
}

void WaveAnalyzer::writeSyncPoint(WaveAnalyzer::SyncPoint point)
{
    if (!_filename.empty()) {
        std::ofstream output(_filename + ".txt", std::ofstream::out | std::ofstream::app);
        if (output.is_open()) {
            output << point.time << " " << point.coord << "\n";
            output.close();
        }
    }
}
