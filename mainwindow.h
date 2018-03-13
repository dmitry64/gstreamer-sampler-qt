#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

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

private slots:
    void on_startPipelineButton_released();
public slots:
    void onSample(std::vector<signed short> samples);
    void onFrame(std::vector<unsigned char> frame);
signals:
    void startPipeline();

private:
    Ui::MainWindow* ui;
};

#endif  // MAINWINDOW_H
