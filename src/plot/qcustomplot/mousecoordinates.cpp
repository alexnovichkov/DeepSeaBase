#include "mousecoordinates.h"

#include "qcpplot.h"
#include "plot/plot.h"

MouseCoordinates::MouseCoordinates(QCPPlot *parent) : QCPItemText(parent), parent(parent)
{
    setText("text");
    setPositionAlignment(Qt::AlignBottom | Qt::AlignLeft);
    setSelectable(false);
    setVisible(true);
    position->setType(QCPItemPosition::ptAbsolute);
}

void MouseCoordinates::update(QMouseEvent *event)
{
    double x = parent->xAxis->pixelToCoord(event->pos().x());
    double y = parent->yAxis->pixelToCoord(event->pos().y());
    if (parent->xAxis->range().contains(x) && parent->yAxis->range().contains(y)) {
        parent->setCursor(Qt::CrossCursor);
        auto pos = event->pos();
        QString text = parent->parent->pointCoordinates({x, y});
        setText(text);
        pos.rx() += 10;
        pos.ry() -= 10;
        auto r = parent->xAxis->coordToPixel(parent->xAxis->range().upper);
        if (auto w = parent->fontMetrics().width(text); pos.x() + w > r)
            pos.rx() -= pos.x() + w - r + 2;
        position->setPixelPosition(pos);
        setVisible(true);
    }
    else {
        parent->setCursor(parent->oldCursor);
        setVisible(false);
    }
    parent->layer("mouse")->replot();
}
