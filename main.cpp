#include "mainwindow.h"
#include <QApplication>
#include "gstreamerthreadworker.h"

#include <QThread>

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    qRegisterMetaType<std::vector<signed short>>("std::vector<signed short>");
    qRegisterMetaType<std::vector<unsigned char>>("std::vector<unsigned char>");

    GstreamerThreadWorker worker;
    // QThread gstreamerThread;
    // QObject::connect(&worker, &GstreamerThreadWorker::finished, &gstreamerThread, &QThread::quit);
    // worker.moveToThread(&gstreamerThread);
    // gstreamerThread.start();

    worker.start();


    MainWindow w;

    // QObject::connect(&w, &MainWindow::startPipeline, &worker, &GstreamerThreadWorker::start);
    QObject::connect(&w, &MainWindow::seekPipeline, &worker, &GstreamerThreadWorker::seekPipeline);
    QObject::connect(&worker, &GstreamerThreadWorker::sampleReady, &w, &MainWindow::onSample);
    QObject::connect(&worker, &GstreamerThreadWorker::frameReady, &w, &MainWindow::onFrame);
    w.show();

    int ret = a.exec();

    return ret;
}
