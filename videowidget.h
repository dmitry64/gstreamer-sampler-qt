#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include <QWidget>

namespace Ui
{
class VideoWidget;
}

class VideoWidget : public QWidget
{
    Q_OBJECT

    std::list<std::vector<unsigned char>> frameList;

public:
    explicit VideoWidget(QWidget* parent = 0);
    ~VideoWidget();
    void drawFrame(const std::vector<unsigned char>& frame);
    void cleanAll();

protected:
    void paintEvent(QPaintEvent* event);
};

#endif  // VIDEOWIDGET_H
