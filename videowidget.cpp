#include "videowidget.h"

#include <QBuffer>
#include <QPainter>
#include <iostream>
#include <QDebug>

VideoWidget::VideoWidget(QWidget* parent)
    : QWidget(parent)
{
}

VideoWidget::~VideoWidget() {}

void VideoWidget::drawFrame(const std::vector<unsigned char>& frame)
{
    qDebug() << "NEW FRAME!";
    frameList.push_back(frame);
}

void VideoWidget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);

    if (!frameList.empty()) {
        const auto& frame = frameList.front();

        QImage image((const unsigned char*) frame.data(), 1920, 1080, QImage::Format_RGB888);

        /*QByteArray ba;
        QBuffer buffer(&ba);
        buffer.open(QIODevice::WriteOnly);
        image.save(&buffer, "PNG");
        std::cout << "buffer size:" << buffer.size() << std::endl;*/
        painter.drawImage(QRect(0, 0, width(), height()), image);
        while (frameList.size() != 1) {
            frameList.pop_front();
        }
    }

    update();
}
