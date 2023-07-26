#include "mousecoordinates.h"

#include "qcpplot.h"
#include "plot/plot.h"

MouseCoordinates::MouseCoordinates(QCPPlot *parent) : QCPItemText(parent), parent(parent)
{
    setText("text");
    setPositionAlignment(Qt::AlignBottom | Qt::AlignLeft);
    setSelectable(false);
    setVisible(false);

    QColor col(Qt::white);
    col.setAlphaF(0.8);
    setBrush(col);
    position->setType(QCPItemPosition::ptAbsolute);
}

void MouseCoordinates::update(QMouseEvent *event)
{
    double x = parent->xAxis->pixelToCoord(event->pos().x());
    double y = parent->yAxis->pixelToCoord(event->pos().y());
    if (parent->xAxis->range().contains(x) && parent->yAxis->range().contains(y)) {
        if (auto c = parent->itemAt<QCPItemStraightLine>(event->pos())) {
            bool horizontal = qFuzzyCompare(c->point1->value()+1, c->point2->value()+1);

            if (horizontal) {
                int newY = (int)(parent->yAxis->coordToPixel(c->point1->value()));
                int posY = event->y();
                if (qAbs(newY-posY)<=4) {
                    parent->setCursor(QCursor(Qt::SizeVerCursor));
                }
            }
            else {
                int newX = (int)(parent->xAxis->coordToPixel(c->point1->key()));
                int posX = event->x();
                if (qAbs(newX-posX)<=4) {
                    parent->setCursor(QCursor(Qt::SizeHorCursor));
                }
            }
        }
        else
            parent->setCursor(Qt::CrossCursor);
        auto pos = event->pos();
        QString text = parent->pointCoordinates({x, y});
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
