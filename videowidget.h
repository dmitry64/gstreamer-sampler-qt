#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include <QWidget>
#include <QSharedPointer>

namespace Ui
{
class VideoWidget;
}

class VideoWidget : public QWidget
{
    Q_OBJECT

    std::list<QSharedPointer<std::vector<unsigned char>>> frameList;

public:
    explicit VideoWidget(QWidget* parent = 0);
    ~VideoWidget();
    void drawFrame(QSharedPointer<std::vector<unsigned char>> frame);
    void cleanAll();

protected:
    void paintEvent(QPaintEvent* event);
};

#endif  // VIDEOWIDGET_H
