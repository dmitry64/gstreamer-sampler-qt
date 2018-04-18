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
    void on_startPipelineButton_released();

    void on_seekButton_released();

    void on_pushButton_released();

public slots:
    void onSample(std::vector<signed short> samples);
    void onFrame(std::vector<unsigned char> frame);
    void onSampleCut(std::vector<signed short> samples);

signals:
    void startPipeline();
    void seekPipeline(int pos);

private:
    Ui::MainWindow* ui;
    GstreamerThreadWorker worker;
};

#endif  // MAINWINDOW_H
