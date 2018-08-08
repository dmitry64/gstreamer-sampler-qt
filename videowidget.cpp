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

void VideoWidget::drawFrame(QSharedPointer<std::vector<unsigned char>> frame)
{
    // qDebug() << "DRAW FRAME!";
    if (frameList.size() > 60) {
        frameList.pop_front();
    }
    frameList.push_back(frame);
    update();
}

void VideoWidget::cleanAll()
{
    std::vector<unsigned char> data;
    data.resize(1280 * 720 * 3);
    for (unsigned char& ptr : data) {
        ptr = rand() % 255;
    }
    // frameList.push_back(data);
}

void VideoWidget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);

    if (!frameList.empty()) {
        const auto& frame = frameList.front();

        QImage image((const unsigned char*) frame->data(), 1280, 720, QImage::Format_RGB888);
        painter.drawImage(QRect(0, 0, width(), height()), image);
        while (frameList.size() != 1) {
            frameList.pop_front();
        }
    }
    /*if (frameList.size() > 1) {
        update();
    }*/
    update();
}
