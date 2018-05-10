#ifndef WAVEANALYZERTHREAD_HPP
#define WAVEANALYZERTHREAD_HPP

#include <QThread>
#include <atomic>
#include <QMutex>
#include <QWaitCondition>
#include "waveanalyzer.hpp"

class WaveAnalyzerThread : public QThread
{
    Q_OBJECT
private:
    std::atomic_bool _isActive;
    WaveAnalyzer _analyzer;
    QMutex _inputMutex;
    QWaitCondition _dataAvailable;


protected:
    void run();

public:
    WaveAnalyzerThread(QObject* parent = nullptr);
    ~WaveAnalyzerThread();
    void startAnalysys();
    void addSampleAndTimestamp(const std::vector<signed short>& samples, GstClockTime time, GstClockTime duration);
    bool getNextBuffer(std::vector<signed short>& output);
    bool getNextCoord(unsigned int& result, GstClockTime& time);
};

#endif  // WAVEANALYZERTHREAD_HPP
