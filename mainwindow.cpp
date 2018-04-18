#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QCloseEvent>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QObject::connect(this, &MainWindow::seekPipeline, &worker, &GstreamerThreadWorker::seekPipeline);
    QObject::connect(&worker, &GstreamerThreadWorker::sampleReady, this, &MainWindow::onSample);
    QObject::connect(&worker, &GstreamerThreadWorker::frameReady, this, &MainWindow::onFrame);
    QObject::connect(&worker, &GstreamerThreadWorker::sampleCutReady, this, &MainWindow::onSampleCut);

    worker.start();
}

MainWindow::~MainWindow()
{
    qDebug() << "Deleting mainwindow...";

    delete ui;
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    worker.stopWorker();
    while (worker.isRunning()) {
        qApp->processEvents();
    }

    event->accept();
}

void MainWindow::on_startPipelineButton_released()
{
    emit startPipeline();
}

void MainWindow::onSample(std::vector<signed short> samples)
{
    ui->audioWidget->onSample(samples);
}

void MainWindow::onFrame(std::vector<unsigned char> frame)
{
    ui->videoWidget->drawFrame(frame);
}

void MainWindow::onSampleCut(std::vector<signed short> samples)
{
    ui->sampleViewer->drawSample(samples);
}

void MainWindow::on_seekButton_released()
{
    int pos = ui->seekPosSpinBox->value();
    emit seekPipeline(pos);
}

void MainWindow::on_pushButton_released()
{
    ui->audioWidget->pause();
}
