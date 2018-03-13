#include "audiovisualizer.h"

#include <iostream>
#include <QPainter>

const int samplesSize = 6;

AudioVisualizer::AudioVisualizer(QWidget* parent)
    : QWidget(parent)
{
    _maxHeight = 0;
}

AudioVisualizer::~AudioVisualizer() {}

void AudioVisualizer::onSample(std::vector<signed short>& samples)
{
    if (_data.size() < samplesSize * 4) {
        _data.push_back(samples);
    }
    else {
        _data.pop_front();
    }
}

void AudioVisualizer::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);

    int w = width();
    int h = height();


    if (_data.size() > samplesSize) {
        int size = samplesSize * _data.front().size();
        float i = 0;
        QPointF begin(0, h / 2);
        QPointF end;
        for (int j = 0; j < samplesSize; ++j) {
            const auto& sample = _data.front();

            for (signed short value : sample) {
                float currentHeight = (static_cast<float>(value) / 32767.0f + 0.5f);
                if (std::abs(currentHeight) > _maxHeight) {
                    _maxHeight = std::abs(currentHeight);
                }
                currentHeight = currentHeight / _maxHeight;
                end = QPointF((i / static_cast<float>(size)) * w, h - currentHeight * h);
                painter.drawLine(begin, end);
                begin = end;
                i += 1.0f;
            }
        }
    }

    painter.drawRect(0, 0, w - 1, h - 1);

    update();
}
