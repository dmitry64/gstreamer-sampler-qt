#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "gstreamerthreadworker.h"

namespace Ui
{
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = 0);
    ~MainWindow();

private:
    void closeEvent(QCloseEvent* event);
private slots:
    void on_audioPauseButton_released();

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
    std::array<std::vector<std::pair<unsigned int, GstClockTime>>, 2> _coordBuffers;
};

#endif  // MAINWINDOW_H
