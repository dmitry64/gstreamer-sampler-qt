#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "gstreamerthreadworker.h"
#include "gstreamervideoplayer.hpp"
#include "controlserver.hpp"

namespace Ui
{
class MainWindow;
}

class CoordPair
{
    unsigned int _coord;
    GstClockTime _time;

public:
    CoordPair(unsigned int coord, GstClockTime time)
        : _coord(coord)
        , _time(time)
    {
    }

    GstClockTime time() const;
    unsigned int coord() const;
    bool operator<(const CoordPair& p1) const
    {
        return this->coord() < p1.coord();
    }
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
private:
    // using CoordPair = std::pair<unsigned int, GstClockTime>;
    using CoordVector = std::vector<CoordPair>;

public:
    explicit MainWindow(QWidget* parent = 0);
    ~MainWindow();

private:
    void closeEvent(QCloseEvent* event);
    void switchMode(int mode);
    GstClockTime getTimeAtCoord(unsigned int coord, int arrayIndex);
    void setFrameAtCoord(unsigned int coord);
private slots:
    void on_audioPauseButton_released();
    void on_seekButton_released();
    void on_modeSwitchButton_released();

    void on_coordSlider_sliderMoved(int position);

public slots:
    void onSampleLeft(std::vector<signed short> samples);
    void onSampleRight(std::vector<signed short> samples);
    void onFrameLeft(std::vector<unsigned char> frame);
    void onFrameRight(std::vector<unsigned char> frame);
    void onSampleCutLeft(std::vector<signed short> samples);
    void onSampleCutRight(std::vector<signed short> samples);
    void onNumberDecodedLeft(unsigned int number);
    void onNewCoord(unsigned int coord, GstClockTime time, int cameraIndex);

private:
    Ui::MainWindow* ui;
    GstreamerThreadWorker workerLeft;
    GstreamerThreadWorker workerRight;
    GstreamerVideoPlayer playerLeft;
    GstreamerVideoPlayer playerRight;
    std::array<CoordVector, 2> _coordBuffers;
    ControlServer _server;
};

#endif  // MAINWINDOW_H
