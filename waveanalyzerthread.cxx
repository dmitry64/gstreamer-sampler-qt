#include "waveanalyzerthread.hpp"
#include <iostream>

void WaveAnalyzerThread::run()
{
    while (_isActive.load()) {
        _inputMutex.lock();
        _dataAvailable.wait(&_inputMutex);
        _analyzer.analyze();
        _inputMutex.unlock();
        //_analyzer.dumpToFile();
    }
}

WaveAnalyzerThread::WaveAnalyzerThread(QObject* parent) {}

WaveAnalyzerThread::~WaveAnalyzerThread()
{
    _isActive.store(false);
    _dataAvailable.wakeAll();
    wait();
}

void WaveAnalyzerThread::startAnalysys()
{
    std::cout << "Starting analyzer thread..." << std::endl;
    _isActive.store(true);
    start();
    std::cout << "Analyzer thread started..." << std::endl;
}

void WaveAnalyzerThread::addSampleAndTimestamp(const std::vector<signed short>& samples, GstClockTime time, GstClockTime duration)
{
    _inputMutex.lock();
    _analyzer.addBufferWithTimecode(samples, time, duration);
    _dataAvailable.wakeAll();
    _inputMutex.unlock();
}

bool WaveAnalyzerThread::getNextBuffer(std::vector<signed short>& output)
{
    _inputMutex.lock();
    bool res = _analyzer.getNextBuffer(output);
    _inputMutex.unlock();
    return res;
}

bool WaveAnalyzerThread::getNextCoord(unsigned int& result)
{
    _inputMutex.lock();
    bool res = _analyzer.getNextCoord(result);
    _inputMutex.unlock();
    return res;
}
