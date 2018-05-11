#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QCloseEvent>

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


    workerLeft.setCameraType(GstreamerThreadWorker::CameraType::eCameraLeft);
    workerRight.setCameraType(GstreamerThreadWorker::CameraType::eCameraRight);

    workerLeft.start();
    workerRight.start();

    switchMode(1);


    playerLeft.setCameraType(GstreamerVideoPlayer::CameraType::eCameraLeft);
    playerRight.setCameraType(GstreamerVideoPlayer::CameraType::eCameraRight);

    // QThread::currentThread()->sleep(1);
    playerLeft.start();
    playerRight.start();
}

MainWindow::~MainWindow()
{
    qDebug() << "Deleting mainwindow...";

    delete ui;
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    workerLeft.stopWorker();
    while (workerLeft.isRunning()) {
        qApp->processEvents();
    }

    workerRight.stopWorker();
    while (workerRight.isRunning()) {
        qApp->processEvents();
    }

    playerLeft.stopWorker();
    while (playerLeft.isRunning()) {
        qApp->processEvents();
    }

    playerRight.stopWorker();
    while (playerRight.isRunning()) {
        qApp->processEvents();
    }


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
    } break;
    case 1: {
        QObject::connect(&playerRight, &GstreamerVideoPlayer::frameReady, this, &MainWindow::onFrameRight);
        QObject::connect(&playerLeft, &GstreamerVideoPlayer::frameReady, this, &MainWindow::onFrameLeft);

        QObject::disconnect(&workerLeft, &GstreamerThreadWorker::frameReady, this, &MainWindow::onFrameLeft);
        QObject::disconnect(&workerRight, &GstreamerThreadWorker::frameReady, this, &MainWindow::onFrameRight);
        ui->modeSwitchButton->setText("Mode: View");
    } break;
    }
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
    qDebug() << "ARRAYS:" << _coordBuffers.at(0).size() << _coordBuffers.at(1).size();
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
    qDebug() << "SEEK:" << _coordBuffers.at(0).at(3 * counter).second;
    playerLeft.showFrameAt(_coordBuffers.at(0).at(3 * counter).second);
    playerRight.showFrameAt(_coordBuffers.at(1).at(3 * counter).second);
    onNumberDecodedLeft(_coordBuffers.at(0).at(3 * counter).first);
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
