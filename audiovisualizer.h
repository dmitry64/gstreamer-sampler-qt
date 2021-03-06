#ifndef AUDIOVISUALIZER_H
#define AUDIOVISUALIZER_H

#include <QWidget>
#include <QFile>
#include <QSharedPointer>

namespace Ui
{
class AudioVisualizer;
}

class AudioVisualizer : public QWidget
{
    Q_OBJECT

    std::list<std::vector<signed short>> _data;
    float _maxHeight;
    bool _pause;


public:
    explicit AudioVisualizer(QWidget* parent = 0);
    ~AudioVisualizer();

    void onSample(QSharedPointer<std::vector<signed short>> samples);
    void pause();

protected:
    void paintEvent(QPaintEvent* event);
};

#endif  // AUDIOVISUALIZER_H
