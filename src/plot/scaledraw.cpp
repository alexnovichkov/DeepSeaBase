#include "scaledraw.h"
#include <QtDebug>
#include "plot.h"
#include "logging.h"
#include <QEvent>
#include <qwt_scale_map.h>
#include <QApplication>

ScaleDraw::ScaleDraw()
{DDD;

}

void ScaleDraw::drawBackbone(QPainter *painter) const
{DDD;
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

AxisOverlay::AxisOverlay(Plot *parent) : QwtPlotZoneItem(), m_plot(parent)
{DDD;
    setOrientation(Qt::Vertical);
    attach(m_plot);
    setVisible(false);
}

void AxisOverlay::setVisibility(bool visible)
{DDD;
    if (visible) {
        setGeom();
        setColor();
    }
    setVisible(visible);
}

Plot *AxisOverlay::plot()
{DDD;
    return m_plot;
}

void AxisOverlay::setColor()
{DDD;
    QColor Color = m_plot->palette().color(QPalette::Active, QPalette::Highlight);
    if (QApplication::keyboardModifiers() & Qt::CTRL) {
        auto blue = Color.blueF();
        Color.setBlueF(Color.greenF());
        Color.setGreenF(blue);
    }

    QPen Pen;
    Pen.setColor(Color.darker(120));
    Pen.setStyle(Qt::SolidLine);
    Pen.setWidth(1);
    Pen.setCosmetic(true);
    setPen(Pen);

    Color = Color.lighter(130);
    Color.setAlpha(64);
    setBrush(Color);
}

LeftAxisOverlay::LeftAxisOverlay(Plot *parent) : AxisOverlay(parent)
{DDD;
}

void LeftAxisOverlay::setGeom()
{DDD;
    const auto &scaleMap = plot()->canvasMap(QwtAxis::XBottom);
    if (plot()->xScaleIsLogarithmic)
        setInterval(scaleMap.s1()-scaleMap.sDist()/20, scaleMap.invTransform(scaleMap.p1()+scaleMap.pDist()/20));
    else
        setInterval(scaleMap.s1()-scaleMap.sDist()/20, scaleMap.s1()+scaleMap.sDist()/20);
}

RightAxisOverlay::RightAxisOverlay(Plot *parent) : AxisOverlay(parent)
{DDD;
}

void RightAxisOverlay::setGeom()
{DDD;
    const auto &scaleMap = plot()->canvasMap(QwtAxis::XBottom);
    if (plot()->xScaleIsLogarithmic)
        setInterval(scaleMap.invTransform(scaleMap.p2()-scaleMap.pDist()/20), scaleMap.s2()+scaleMap.sDist()/20);
    else
        setInterval(scaleMap.s2()-scaleMap.sDist()/20, scaleMap.s2()+scaleMap.sDist()/20);
}
