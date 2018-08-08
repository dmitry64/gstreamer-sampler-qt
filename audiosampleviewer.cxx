#include "audiosampleviewer.hpp"
#include "ui_audiosampleviewer.h"

#include <QPainter>
#include <QDebug>

AudioSampleViewer::AudioSampleViewer(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::AudioSampleViewer)
{
    ui->setupUi(this);
    _pause = false;
}

AudioSampleViewer::~AudioSampleViewer()
{
    delete ui;
}

void AudioSampleViewer::drawSample(QSharedPointer<std::vector<signed short>> samples)
{
    if (!_pause) {
        _currentSamples = samples.operator*();
    }
    update();
}

void AudioSampleViewer::pause()
{
    _pause = !_pause;
}

void AudioSampleViewer::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);

    int width = this->width();
    int height = this->height();

    painter.fillRect(QRect(0, 0, width, height), Qt::white);

    int size = _currentSamples.size();
    float step = static_cast<float>(width) / static_cast<float>(size);
    painter.setPen(Qt::red);

    float currentX = 0.0f;
    QPointF begin = QPointF(0, height / 2);
    for (const signed short sample : _currentSamples) {
        float samplePos = static_cast<float>(height) - ((static_cast<float>(sample) + 32767.0f) / 65535.0f) * static_cast<float>(height);
        QPointF end(static_cast<double>(currentX), samplePos);
        if (currentX != 0.0f) {
            painter.drawLine(begin, end);
        }
        begin = end;
        currentX += step;
    }
}
