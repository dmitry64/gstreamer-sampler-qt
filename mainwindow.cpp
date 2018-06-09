#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QCloseEvent>
#include <unistd.h>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QObject::connect(&workerLeft, &GstreamerThreadWorker::sampleReady, this, &MainWindow::onSampleLeft);
    // QObject::connect(&workerLeft, &GstreamerThreadWorker::frameReady, this, &MainWindow::onFrameLeft);
    QObject::connect(&workerLeft, &GstreamerThreadWorker::sampleCutReady, this, &MainWindow::onSampleCutLeft);
    QObject::connect(&workerLeft, &GstreamerThreadWorker::coordReady, this, &MainWindow::onNewCoord);

    QObject::connect(&workerRight, &GstreamerThreadWorker::sampleReady, this, &MainWindow::onSampleRight);
    // QObject::connect(&workerRight, &GstreamerThreadWorker::frameReady, this, &MainWindow::onFrameRight);
    QObject::connect(&workerRight, &GstreamerThreadWorker::sampleCutReady, this, &MainWindow::onSampleCutRight);
    QObject::connect(&workerRight, &GstreamerThreadWorker::coordReady, this, &MainWindow::onNewCoord);

    QObject::connect(&_server, &ControlServer::doStartRegistration, this, &MainWindow::onRegistrationStart);
    QObject::connect(&_server, &ControlServer::doStopRegistration, this, &MainWindow::onRegistrationStop);
    QObject::connect(&_server, &ControlServer::doViewMode, this, &MainWindow::onViewMode);
    QObject::connect(&_server, &ControlServer::doRealtimeMode, this, &MainWindow::onRealtimeMode);
    QObject::connect(&_server, &ControlServer::doSetCoord, this, &MainWindow::onSetCoord);


    workerLeft.setCameraType(GstreamerThreadWorker::CameraType::eCameraLeft);
    workerRight.setCameraType(GstreamerThreadWorker::CameraType::eCameraRight);

    playerLeft.setCameraType(GstreamerVideoPlayer::CameraType::eCameraLeft);
    playerRight.setCameraType(GstreamerVideoPlayer::CameraType::eCameraRight);

    startAllWorkers();

    switchMode(1);
}

MainWindow::~MainWindow()
{
    qDebug() << "Deleting mainwindow...";

    delete ui;
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    qDebug() << "Close event!";

    stopAllWorkers();
    event->accept();
}

void MainWindow::switchMode(int mode)
{
    switch (mode) {
    case 0: {
        QObject::disconnect(&playerRight, &GstreamerVideoPlayer::frameReady, this, &MainWindow::onFrameRight);
        QObject::disconnect(&playerLeft, &GstreamerVideoPlayer::frameReady, this, &MainWindow::onFrameLeft);

        QObject::connect(&workerLeft, &GstreamerThreadWorker::frameReady, this, &MainWindow::onFrameLeft);
        QObject::connect(&workerRight, &GstreamerThreadWorker::frameReady, this, &MainWindow::onFrameRight);
        ui->modeSwitchButton->setText("Mode: Realtime");
        ui->coordSlider->setEnabled(false);
    } break;
    case 1: {
        QObject::disconnect(&workerLeft, &GstreamerThreadWorker::frameReady, this, &MainWindow::onFrameLeft);
        QObject::disconnect(&workerRight, &GstreamerThreadWorker::frameReady, this, &MainWindow::onFrameRight);

        QObject::connect(&playerRight, &GstreamerVideoPlayer::frameReady, this, &MainWindow::onFrameRight);
        QObject::connect(&playerLeft, &GstreamerVideoPlayer::frameReady, this, &MainWindow::onFrameLeft);

        ui->modeSwitchButton->setText("Mode: View");
        ui->coordSlider->setEnabled(true);

    } break;
    }
}

GstClockTime MainWindow::getTimeAtCoord(unsigned int coord, int arrayIndex)
{
    CoordVector& currentVector = _coordBuffers.at(arrayIndex);
    CoordVector::iterator it = currentVector.begin();
    it = std::lower_bound(currentVector.begin(), currentVector.end(), CoordPair(coord, 0));
    if (it != currentVector.end()) {
        return it.operator*().time();
    }
    return 0;
}

void MainWindow::setFrameAtCoord(unsigned int coord)
{
    GstClockTime leftTime = getTimeAtCoord(coord, 0);
    GstClockTime rightTime = getTimeAtCoord(coord, 1);
    playerLeft.showFrameAt(leftTime);
    playerRight.showFrameAt(rightTime);
}

void MainWindow::stopAllWorkers()
{
    workerLeft.stopWorker();
    while (workerLeft.isRunning()) {
        qApp->processEvents();
    }
    qDebug() << "Left worker stopped";

    workerRight.stopWorker();
    while (workerRight.isRunning()) {
        qApp->processEvents();
    }
    qDebug() << "Right worker stopped";

    playerLeft.stopWorker();
    while (playerLeft.isRunning()) {
        qApp->processEvents();
    }
    qDebug() << "Left player stopped";

    playerRight.stopWorker();
    while (playerRight.isRunning()) {
        qApp->processEvents();
    }
    qDebug() << "Right player stopped";
}

void MainWindow::startAllWorkers()
{
    qDebug() << "Starting all workers...";

    sync();
    workerLeft.start();
    workerRight.start();
    QThread::msleep(1000);
    sync();
    playerLeft.start();
    playerRight.start();
}

void MainWindow::onSampleLeft(std::vector<signed short> samples)
{
    ui->audioWidgetLeft->onSample(samples);
}

void MainWindow::onSampleRight(std::vector<signed short> samples)
{
    ui->audioWidgetRight->onSample(samples);
}

void MainWindow::onFrameLeft(std::vector<unsigned char> frame)
{
    ui->videoWidgetLeft->drawFrame(frame);
}

void MainWindow::onFrameRight(std::vector<unsigned char> frame)
{
    ui->videoWidgetRight->drawFrame(frame);
}

void MainWindow::onSampleCutLeft(std::vector<signed short> samples)
{
    ui->sampleViewerLeft->drawSample(samples);
}

void MainWindow::onSampleCutRight(std::vector<signed short> samples)
{
    ui->sampleViewerRight->drawSample(samples);
}

void MainWindow::onNumberDecodedLeft(unsigned int number)
{
    ui->numberLabel->setText(QString::number(number));
    QString bitsStr;
    bitsStr += QString::number((number >> 3) & 0x00000001);
    bitsStr += QString::number((number >> 2) & 0x00000001);
    bitsStr += QString::number((number >> 1) & 0x00000001);
    bitsStr += QString::number(number & 0x00000001);
    ui->lastBitsLabel->setText(bitsStr);
}

void MainWindow::onNewCoord(unsigned int coord, GstClockTime time, int cameraIndex)
{
    _coordBuffers.at(cameraIndex).emplace_back(coord, time);
    // qDebug() << "ARRAYS:" << _coordBuffers.at(0).size() << _coordBuffers.at(1).size();

    if (!_coordBuffers.at(0).empty() && !_coordBuffers.at(1).empty()) {
        unsigned int maxCoord = std::max(_coordBuffers.at(0).back().coord(), _coordBuffers.at(1).back().coord());
        ui->coordSlider->setMaximum(maxCoord);
    }
    else {
        ui->coordSlider->setMinimum(coord);
    }
}

void MainWindow::on_audioPauseButton_released()
{
    ui->audioWidgetLeft->pause();
    ui->audioWidgetRight->pause();
}

void MainWindow::on_seekButton_released()
{
    static int counter = 0;
    counter++;
    qDebug() << "SEEK:" << _coordBuffers.at(0).at(3 * counter).time();
    playerLeft.showFrameAt(_coordBuffers.at(0).at(3 * counter).time());
    playerRight.showFrameAt(_coordBuffers.at(1).at(3 * counter).time());
    onNumberDecodedLeft(_coordBuffers.at(0).at(3 * counter).coord());
}

void MainWindow::on_modeSwitchButton_released()
{
    static int mode = 1;
    if (mode == 0) {
        mode = 1;
    }
    else {
        mode = 0;
    }
    switchMode(mode);
}

unsigned int CoordPair::coord() const
{
    return _coord;
}

GstClockTime CoordPair::time() const
{
    return _time;
}

void MainWindow::on_coordSlider_sliderMoved(int position)
{
    setFrameAtCoord(position);
}

void MainWindow::onRegistrationStart(QString name)
{
    _currentRegistrationName = name;
    stopAllWorkers();
    startAllWorkers();
}

void MainWindow::onRegistrationStop()
{
    stopAllWorkers();
}

void MainWindow::onViewMode()
{
    switchMode(1);
}

void MainWindow::onRealtimeMode()
{
    switchMode(0);
}

void MainWindow::onSetCoord(unsigned int coord) {}
