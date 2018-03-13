#ifndef AUDIOVISUALIZER_H
#define AUDIOVISUALIZER_H

#include <QWidget>

namespace Ui
{
class AudioVisualizer;
}

class AudioVisualizer : public QWidget
{
    Q_OBJECT

    std::list<std::vector<signed short>> _data;
    float _maxHeight;

public:
    explicit AudioVisualizer(QWidget* parent = 0);
    ~AudioVisualizer();

    void onSample(std::vector<signed short>& samples);

protected:
    void paintEvent(QPaintEvent* event);
};

#endif  // AUDIOVISUALIZER_H
