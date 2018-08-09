#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QCloseEvent>
#include <unistd.h>
#include <QDir>
#include "settings.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , _mode(1)
{
    ui->setupUi(this);

    workerLeft.setObjectName("WorkerLeft");
    workerRight.setObjectName("WorkerRight");
    playerLeft.setObjectName("PlayerLeft");
    playerRight.setObjectName("PlayerRight");

    QObject::connect(&workerLeft, &GstreamerThreadWorker::sampleReady, this, &MainWindow::onSampleLeft);
    QObject::connect(&workerLeft, &GstreamerThreadWorker::sampleCutReady, this, &MainWindow::onSampleCutLeft);
    QObject::connect(&workerLeft, &GstreamerThreadWorker::coordReady, this, &MainWindow::onNewCoord);

    QObject::connect(&workerRight, &GstreamerThreadWorker::sampleReady, this, &MainWindow::onSampleRight);
    QObject::connect(&workerRight, &GstreamerThreadWorker::sampleCutReady, this, &MainWindow::onSampleCutRight);
    QObject::connect(&workerRight, &GstreamerThreadWorker::coordReady, this, &MainWindow::onNewCoord);

    QObject::connect(&_server, &ControlServer::doStartRegistration, this, &MainWindow::onRegistrationStart);
    QObject::connect(&_server, &ControlServer::doStopRegistration, this, &MainWindow::onRegistrationStop);
    QObject::connect(&_server, &ControlServer::doViewMode, this, &MainWindow::onViewMode);
    QObject::connect(&_server, &ControlServer::doRealtimeMode, this, &MainWindow::onRealtimeMode);
    QObject::connect(&_server, &ControlServer::doSetCoord, this, &MainWindow::onSetCoord);
    QObject::connect(&_server, &ControlServer::clientConnected, this, &MainWindow::onClientConnected);
    QObject::connect(&_server, &ControlServer::clientDisconnected, this, &MainWindow::onClientDisconnected);

    workerLeft.setCameraType(GstreamerThreadWorker::CameraType::eCameraLeft);
    workerRight.setCameraType(GstreamerThreadWorker::CameraType::eCameraRight);

    playerLeft.setCameraType(GstreamerVideoPlayer::CameraType::eCameraLeft);
    playerRight.setCameraType(GstreamerVideoPlayer::CameraType::eCameraRight);

    ui->uiGroupBox->hide();
    ui->registrationLabel->setText(tr("Waiting..."));
    startAllWorkers();
    // ui->audioWidgetLeft->hide();
    //  ui->sampleViewerLeft->hide();

    switchMode(1);
    _mode = 1;
    _lastViewCoord = 1;
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
        ui->modeLabel->setText(tr("Realtime"));
        ui->coordSlider->setEnabled(false);
    } break;
    case 1: {
        QObject::disconnect(&workerLeft, &GstreamerThreadWorker::frameReady, this, &MainWindow::onFrameLeft);
        QObject::disconnect(&workerRight, &GstreamerThreadWorker::frameReady, this, &MainWindow::onFrameRight);

        QObject::connect(&playerRight, &GstreamerVideoPlayer::frameReady, this, &MainWindow::onFrameRight);
        QObject::connect(&playerLeft, &GstreamerVideoPlayer::frameReady, this, &MainWindow::onFrameLeft);

        ui->modeSwitchButton->setText("Mode: View");
        ui->modeLabel->setText(tr("View"));
        ui->coordSlider->setEnabled(true);

    } break;
    }
}

GstClockTime MainWindow::getTimeAtCoord(unsigned int coord, int arrayIndex)
{
    CoordVector& currentVector = _coordBuffers.at(arrayIndex);
    // CoordVector::iterator it = currentVector.begin();
    /*it = std::lower_bound(currentVector.begin(), currentVector.end(), CoordPair(coord, 0));
    if (it != currentVector.end()) {
        return it.operator*().time();
    }*/
    for (auto it = currentVector.begin(); it != currentVector.end(); ++it) {
        if (it.operator*().coord() > coord) {
            if (it != currentVector.begin()) {
                // auto prevIt = it - 1;
                // GstClockTime avgTime = prevIt.operator*().time() + (it.operator*().time() - prevIt.operator*().time()) / 2;
                return it.operator*().time();
            }
            else {
                return it.operator*().time();
            }
        }
    }
    return 0;
}

void MainWindow::setFrameAtCoord(unsigned int coord)
{
    std::pair<unsigned int, unsigned int> minmax = getMinMaxCoords();
    unsigned int minCoord = minmax.first;
    unsigned int maxCoord = minmax.second;

    qDebug() << "Setting coord:" << coord;
    GstClockTime leftTime = getTimeAtCoord(coord, 0);
    GstClockTime rightTime = getTimeAtCoord(coord, 1);
    qDebug() << "LEFT TIME:" << leftTime;
    qDebug() << "RIGHT TIME:" << rightTime;

    playerLeft.showFrameAt(leftTime);
    playerRight.showFrameAt(rightTime);
}

void MainWindow::stopAllWorkers()
{
    sync();
    if (workerLeft.isRunning()) {
        workerLeft.stopWorker();
        while (workerLeft.isRunning()) {
            qApp->processEvents();
        }
        qDebug() << "Left worker stopped";
        QThread::msleep(100);
    }

    if (workerRight.isRunning()) {
        workerRight.stopWorker();
        while (workerRight.isRunning()) {
            qApp->processEvents();
        }
        qDebug() << "Right worker stopped";
        QThread::msleep(100);
    }

    if (playerLeft.isRunning()) {
        playerLeft.stopWorker();
        while (playerLeft.isRunning()) {
            qApp->processEvents();
        }
        qDebug() << "Left player stopped";
        QThread::msleep(100);
    }

    if (playerRight.isRunning()) {
        playerRight.stopWorker();
        while (playerRight.isRunning()) {
            qApp->processEvents();
        }
        qDebug() << "Right player stopped";
        QThread::msleep(100);
    }
    QThread::msleep(1000);
    sync();
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

std::pair<unsigned int, unsigned int> MainWindow::getMinMaxCoords()
{
    unsigned int maxCoord = 0;
    unsigned int minCoord = 0;

    if (!_coordBuffers.at(0).empty() && !_coordBuffers.at(1).empty()) {
        maxCoord = std::max(_coordBuffers.at(0).back().coord(), _coordBuffers.at(1).back().coord());
        minCoord = std::min(_coordBuffers.at(0).front().coord(), _coordBuffers.at(1).front().coord());
    }
    else {
        if (!_coordBuffers.at(0).empty()) {
            maxCoord = _coordBuffers.at(0).back().coord();
            minCoord = _coordBuffers.at(0).front().coord();
        }
        if (!_coordBuffers.at(1).empty()) {
            maxCoord = _coordBuffers.at(1).back().coord();
            minCoord = _coordBuffers.at(1).front().coord();
        }
    }

    return std::pair<unsigned int, unsigned int>(minCoord, maxCoord);
}

void MainWindow::updateSliderRange()
{
    std::pair<unsigned int, unsigned int> minmax = getMinMaxCoords();
    unsigned int minCoord = minmax.first;
    unsigned int maxCoord = minmax.second;

    unsigned int width = maxCoord - minCoord;

    unsigned int sliderPos = 0;
    if (_lastViewCoord < minCoord) {
        sliderPos = 0;
    }
    else {
        sliderPos = (static_cast<double>(_lastViewCoord - minCoord) / static_cast<double>(width)) * 1000.0;
    }
    ui->coordSlider->setValue(sliderPos);
}

void MainWindow::setFrameBySlider(int sliderPos)
{
    sync();
    std::pair<unsigned int, unsigned int> minmax = getMinMaxCoords();
    unsigned int minCoord = minmax.first;
    unsigned int maxCoord = minmax.second;

    unsigned int width = maxCoord - minCoord;
    unsigned int posCoord = minCoord + ((width / 1000) * sliderPos);
    _lastViewCoord = posCoord;

    setFrameAtCoord(posCoord);
}

void MainWindow::setFrameByTimeSlider(int timeSliderPos)
{
    sync();
    GstClockTime maxTime = playerLeft.getTotalDuration();
    GstClockTime minTime = 0;

    GstClockTime width = maxTime - minTime;
    GstClockTime posCoord = minTime + ((width / 1000) * timeSliderPos);

    playerLeft.showFrameAt(posCoord);
    playerRight.showFrameAt(posCoord);
}

void MainWindow::onSampleLeft(QSharedPointer<std::vector<signed short>> samples)
{
    ui->audioWidgetLeft->onSample(samples);
}

void MainWindow::onSampleRight(QSharedPointer<std::vector<signed short>> samples)
{
    ui->audioWidgetRight->onSample(samples);
}

void MainWindow::onFrameLeft(QSharedPointer<std::vector<unsigned char>> frame)
{
    ui->videoWidgetLeft->drawFrame(frame);
}

void MainWindow::onFrameRight(QSharedPointer<std::vector<unsigned char>> frame)
{
    ui->videoWidgetRight->drawFrame(frame);
}

void MainWindow::onSampleCutLeft(QSharedPointer<std::vector<signed short>> samples)
{
    ui->sampleViewerLeft->drawSample(samples);
}

void MainWindow::onSampleCutRight(QSharedPointer<std::vector<signed short>> samples)
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
    if (!_coordBuffers.at(cameraIndex).empty()) {
        if (coord < _coordBuffers.at(cameraIndex).back().coord()) {
            _coordBuffers.at(0).clear();
            _coordBuffers.at(1).clear();
        }
    }

    _coordBuffers.at(cameraIndex).emplace_back(coord, time);

    ui->coordCountLabelLeft->setText(QString::number(_coordBuffers.at(0).size()));
    ui->coordCountLabelRight->setText(QString::number(_coordBuffers.at(1).size()));


    if (!_coordBuffers.at(0).empty()) {
        ui->firstCoordLabelLeft->setText(QString::number(_coordBuffers.at(0).front().coord()));
        ui->lastCoordLabelLeft->setText(QString::number(_coordBuffers.at(0).back().coord()));
    }
    else {
        ui->firstCoordLabelLeft->setText("---");
        ui->lastCoordLabelLeft->setText("---");
    }

    if (!_coordBuffers.at(1).empty()) {
        ui->firstCoordLabelRight->setText(QString::number(_coordBuffers.at(1).front().coord()));
        ui->lastCoordLabelRight->setText(QString::number(_coordBuffers.at(1).back().coord()));
    }
    else {
        ui->firstCoordLabelRight->setText("---");
        ui->lastCoordLabelRight->setText("---");
    }

    if (_coordBuffers.at(cameraIndex).size() % 100 == 0) {
        updateSliderRange();
    }
    sync();
}

void MainWindow::onClientConnected()
{
    ui->connectionLabel->setText(tr("Connected"));
}

void MainWindow::onClientDisconnected()
{
    ui->connectionLabel->setText(tr("Disconnected"));
}

void MainWindow::on_audioPauseButton_released()
{
    ui->audioWidgetLeft->pause();
    ui->audioWidgetRight->pause();
    ui->sampleViewerLeft->pause();
    ui->sampleViewerRight->pause();
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
    if (_mode == 0) {
        _mode = 1;
    }
    else {
        _mode = 0;
    }
    switchMode(_mode);
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
    qDebug() << "SLIDER COORD:" << position;
    setFrameBySlider(position);
}

void MainWindow::onRegistrationStart(QString name)
{
    sync();
    QThread::msleep(1000);
    _currentRegistrationName = name;
    stopAllWorkers();
    QString path = restoreDefaultVideoFolder() + "/" + name;

    QDir dir;
    dir.mkpath(path);

    workerLeft.setRegistrationFileName(name + "/", name);
    workerRight.setRegistrationFileName(name + "/", name);
    playerLeft.setRegistrationFileName(name + "/", name);
    playerRight.setRegistrationFileName(name + "/", name);
    _coordBuffers.at(0).clear();
    _coordBuffers.at(1).clear();

    QThread::msleep(100);
    startAllWorkers();
    ui->registrationLabel->setText(tr("Recording..."));
    ui->registrationNameLabel->setText(name);
}

void MainWindow::onRegistrationStop()
{
    stopAllWorkers();
    _coordBuffers.at(0).clear();
    _coordBuffers.at(1).clear();
    ui->registrationLabel->setText(tr("Stopped"));
    ui->registrationNameLabel->setText("");
}

void MainWindow::onViewMode()
{
    if (_mode != 1) {
        switchMode(1);
        _mode = 1;
    }
}

void MainWindow::onRealtimeMode()
{
    if (_mode != 0) {
        switchMode(0);
        _mode = 0;
    }
}

void MainWindow::onSetCoord(unsigned int coord)
{
    if (coord != _lastViewCoord) {
        setFrameAtCoord(coord);
    }
    _lastViewCoord = coord;
}

void MainWindow::on_startRegistrationButton_released()
{
    onRegistrationStart(ui->registrationNameLineEdit->text());
}

void MainWindow::on_stopRegistrationButton_released()
{
    onRegistrationStop();
}

void MainWindow::on_hideUiButton_released()
{
    if (ui->uiGroupBox->isVisible()) {
        ui->uiGroupBox->hide();
    }
    else {
        ui->uiGroupBox->show();
    }
}

void MainWindow::on_coordSlider_sliderReleased() {}

void MainWindow::on_timeSlider_sliderMoved(int position)
{
    setFrameByTimeSlider(position);
}
