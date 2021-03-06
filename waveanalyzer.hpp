#ifndef WAVEANALYZER_HPP
#define WAVEANALYZER_HPP

#include <gst/gstbuffer.h>
#include <vector>
#include <queue>
#include <array>
#include <QSharedPointer>
#include <QFile>
const static unsigned int SAMPLE_SIZE = 300;
const static int FRONT_THRESHOLD = 10000;
const static unsigned int SAMPLE_OFFSET = 30;
const static int BASELINE_WINDOW_WIDTH = 6;
const static int NEG_FRONT_WINDOW_WIDTH = 3;
const static double AvTOL = 12;
const static int AFTOL = 32767 / 2;
const static int DFTOL = 12;

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

    struct SyncPoint
    {
        unsigned int coord;
        GstClockTime time;
    };

private:
    SignalsBuffer _buffers;
    std::vector<GstClockTime> _timeBuffers;
    std::queue<SignalsBuffer> _outputBuffers;
    std::queue<SyncPoint> _syncPoints;
    std::string _filename;
    QFile _outputFile;
    int _counter;

private:
    bool decodeBuffer(const SignalsBuffer& buffer, unsigned int& result);
    bool checkFront(const TWaveFront& front);

public:
    WaveAnalyzer();
    void addBufferWithTimecode(QSharedPointer<std::vector<signed short>> samples, GstClockTime timestamp, GstClockTime duration);
    void analyze();
    bool getNextBuffer(SignalsBuffer& output);
    bool getNextCoord(unsigned int& coord, GstClockTime& time);
    bool getEnoughData();
    void dumpToFile();
    void writeSyncPoint(SyncPoint point);
    std::string getFilename() const;
    void setFilename(const std::string& filename);
};

#endif  // WAVEANALYZER_HPP
