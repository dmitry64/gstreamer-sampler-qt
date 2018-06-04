#include "audiovisualizer.h"

#include <iostream>
#include <QPainter>
#include <QDebug>

const int samplesSize = 4;

AudioVisualizer::AudioVisualizer(QWidget* parent)
    : QWidget(parent)
{
    _maxHeight = 1.0f;
    _pause = false;

    _counter = 0;
}

AudioVisualizer::~AudioVisualizer() {}

void AudioVisualizer::onSample(const std::vector<signed short>& samples)
{
    if (!_pause) {
        if (_data.size() < samplesSize * 4) {
            _data.push_back(samples);
        }
        else {
            _data.push_back(samples);
            _data.pop_front();
        }
    }
}

void AudioVisualizer::pause()
{
    _pause = !_pause;
}

void AudioVisualizer::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);

    int w = width();
    int h = height();

    painter.fillRect(QRect(0, 0, w, h), QBrush(Qt::white));
    painter.setPen(Qt::black);

    if (_data.size() > samplesSize) {
        int size = samplesSize * _data.front().size();
        float i = 0;
        QPointF begin(0, h / 2);
        QPointF end;
        auto it = _data.begin();
        for (int j = 0; j < samplesSize; ++j) {
            const auto& sample = it.operator*();
            ++it;


            for (signed short value : sample) {
                float currentHeight = (static_cast<float>(value) / 32767.0f);
                if (std::abs(currentHeight) > _maxHeight) {
                    //_maxHeight = std::abs(currentHeight);
                }
                currentHeight = currentHeight / (_maxHeight * 2.0f);
                end = QPointF((i / static_cast<float>(size)) * w, h - (currentHeight + 0.5f) * h);
                painter.drawLine(begin, end);
                begin = end;
                if (!(static_cast<int>(i) % 40)) {
                    painter.drawLine((i / static_cast<float>(size)) * w, h, (i / static_cast<float>(size)) * w, h - 6);
                }

                i += 1.0f;
            }

            painter.drawLine(w - (i / static_cast<float>(size)) * w, h, w - (i / static_cast<float>(size)) * w, h - 10);
            painter.drawText(w - (i / static_cast<float>(size)) * w, h - 15, QString::number(40 * (j + 1)) + "ms");
        }
        painter.drawText(10, 20, "Max: " + QString::number(_maxHeight));
    }

    painter.drawRect(0, 0, w - 1, h - 1);

    update();
}
