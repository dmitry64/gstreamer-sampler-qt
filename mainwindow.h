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
    int _mode;

    unsigned int _lastViewCoord;

public:
    explicit MainWindow(QWidget* parent = 0);
    ~MainWindow();

private:
    void closeEvent(QCloseEvent* event);
    void switchMode(int mode);
    GstClockTime getTimeAtCoord(unsigned int coord, int arrayIndex);
    void setFrameAtCoord(unsigned int coord);
    void stopAllWorkers();
    void startAllWorkers();
    std::pair<unsigned int, unsigned int> getMinMaxCoords();
    void updateSliderRange();
private slots:
    void on_audioPauseButton_released();
    void on_seekButton_released();
    void on_modeSwitchButton_released();

    void on_coordSlider_sliderMoved(int position);
    void onRegistrationStart(QString name);
    void onRegistrationStop();
    void onViewMode();
    void onRealtimeMode();
    void onSetCoord(unsigned int coord);


    void on_startRegistrationButton_released();

    void on_stopRegistrationButton_released();

    void on_hideUiButton_released();

    void on_coordSlider_sliderReleased();

public slots:
    void onSampleLeft(QSharedPointer<std::vector<signed short>> samples);
    void onSampleRight(QSharedPointer<std::vector<signed short>> samples);
    void onFrameLeft(QSharedPointer<std::vector<unsigned char>> frame);
    void onFrameRight(QSharedPointer<std::vector<unsigned char>> frame);
    void onSampleCutLeft(QSharedPointer<std::vector<signed short>> samples);
    void onSampleCutRight(QSharedPointer<std::vector<signed short>> samples);
    void onNumberDecodedLeft(unsigned int number);
    void onNewCoord(unsigned int coord, GstClockTime time, int cameraIndex);
    void onClientConnected();
    void onClientDisconnected();

private:
    Ui::MainWindow* ui;
    GstreamerThreadWorker workerLeft;
    GstreamerThreadWorker workerRight;
    GstreamerVideoPlayer playerLeft;
    GstreamerVideoPlayer playerRight;
    std::array<CoordVector, 2> _coordBuffers;
    ControlServer _server;
    QString _currentRegistrationName;
};

#endif  // MAINWINDOW_H
