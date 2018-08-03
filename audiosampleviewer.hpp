#ifndef AUDIOSAMPLEVIEWER_HPP
#define AUDIOSAMPLEVIEWER_HPP

#include <QWidget>
#include <QSharedPointer>

namespace Ui
{
class AudioSampleViewer;
}

class AudioSampleViewer : public QWidget
{
    Q_OBJECT

public:
    explicit AudioSampleViewer(QWidget* parent = 0);
    ~AudioSampleViewer();
    std::vector<signed short> _currentSamples;
    void drawSample(QSharedPointer<std::vector<signed short>> samples);

protected:
    void paintEvent(QPaintEvent* event);

private:
    Ui::AudioSampleViewer* ui;
};

#endif  // AUDIOSAMPLEVIEWER_HPP
