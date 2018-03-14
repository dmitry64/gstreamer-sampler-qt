#include "videowidget.h"

#include <QPainter>
#include <iostream>

VideoWidget::VideoWidget(QWidget* parent)
    : QWidget(parent)
{
}

VideoWidget::~VideoWidget() {}

void VideoWidget::drawFrame(std::vector<unsigned char>& frame)
{
    frameList.push_back(frame);
}

void VideoWidget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);

    if (!frameList.empty()) {
        const auto& frame = frameList.front();

        QImage image((const unsigned char*) frame.data(), 1280, 720, QImage::Format_RGB888);
        painter.drawImage(QRect(0, 0, width(), height()), image);
        if (frameList.size() > 1) {
            frameList.pop_front();
        }
    }

    update();
}
