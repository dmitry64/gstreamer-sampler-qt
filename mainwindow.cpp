#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
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
