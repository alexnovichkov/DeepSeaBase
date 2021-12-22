#include "scaledraw.h"
#include <QtDebug>
#include "plot.h"

ScaleDraw::ScaleDraw()
{

}

void ScaleDraw::drawBackbone(QPainter *painter) const
{
    const int pw = qMax(qRound(penWidthF()),1);

    const qreal len = length();
    const QPointF _pos = pos();

    switch (alignment()) {
        case QwtScaleDraw::LeftScale:
        {
            const qreal x = qRound( _pos.x() - ( pw - 1 ) / 2 );
            QwtPainter::drawLine( painter, x, _pos.y(), x, _pos.y() + len );
            if (hover==1) {
                QwtPainter::drawLine( painter, x-1, _pos.y(), x-1, _pos.y() + len/2.0 );
            }
            else if (hover==2) {
                QwtPainter::drawLine( painter, x-1, _pos.y() + len/2.0, x-1, _pos.y() + len );
            }

            break;
        }
        case QwtScaleDraw::RightScale:
        {
            const qreal x = qRound( _pos.x() + pw / 2 );
            QwtPainter::drawLine( painter, x, _pos.y(), x, _pos.y() + len );
            if (hover==1) {
                QwtPainter::drawLine( painter, x+1, _pos.y(), x+1, _pos.y() + len/2.0 );
            }
            else if (hover==2) {
                QwtPainter::drawLine( painter, x+1, _pos.y() + len/2.0, x+1, _pos.y() + len );
            }
            break;
        }
        case QwtScaleDraw::TopScale:
        {
            const qreal y = qRound( _pos.y() - ( pw - 1 ) / 2 );
            QwtPainter::drawLine( painter, _pos.x(), y, _pos.x() + len, y );
            if (hover==1) {
                QwtPainter::drawLine( painter, _pos.x(), y-1, _pos.x() + len/2, y-1 );
            }
            else if (hover==2) {
                QwtPainter::drawLine( painter, _pos.x()+len/2, y-1, _pos.x() + len, y-1 );
            }
            break;
        }
        case QwtScaleDraw::BottomScale:
        {
            const qreal y = qRound( _pos.y() + pw / 2 );
            QwtPainter::drawLine( painter, _pos.x(), y, _pos.x() + len, y );
            if (hover==1) {
                QwtPainter::drawLine( painter, _pos.x(), y+1, _pos.x() + len/2, y+1 );
            }
            else if (hover==2) {
                QwtPainter::drawLine( painter, _pos.x()+len/2, y+1, _pos.x() + len, y+1 );
            }
            break;
        }
    }
}

#include <QPainter>

AxisOverlay::AxisOverlay(Plot *parent)
    : QWidget(parent), m_plot(parent)
{
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setVisible(false);
    setMouseTracking(false);
}

void AxisOverlay::setVisible(bool visible)
{
    if (visible) {
        setGeom();

       // show();
    }
    QWidget::setVisible(visible);
}

void AxisOverlay::paintEvent(QPaintEvent *event)
{
    QRect r = rect();

    QPainter painter(this);
    QColor Color = palette().color(QPalette::Active, QPalette::Highlight);
    QPen Pen = painter.pen();
    Pen.setColor(Color.darker(120));
    Pen.setStyle(Qt::SolidLine);
    Pen.setWidth(1);
    Pen.setCosmetic(true);
    painter.setPen(Pen);
    Color = Color.lighter(130);
    Color.setAlpha(64);
    painter.setBrush(Color);
    painter.drawRect(r.adjusted(0, 0, -1, -1));
}

Plot *AxisOverlay::plot()
{
    return m_plot;
}

LeftAxisOverlay::LeftAxisOverlay(Plot *parent) : AxisOverlay(parent)
{
}

void LeftAxisOverlay::setGeom()
{
    resize({50, plot()->canvas()->height()});
    QPoint TopLeft = plot()->canvas()->mapToGlobal(plot()->canvas()->rect().topLeft());
    move(TopLeft);
}

RightAxisOverlay::RightAxisOverlay(Plot *parent) : AxisOverlay(parent)
{
}

void RightAxisOverlay::setGeom()
{
    resize({50,plot()->canvas()->height()});
    auto point = plot()->canvas()->mapToGlobal(plot()->canvas()->rect().topRight());
    move(point.x()-50, point.y());
}
