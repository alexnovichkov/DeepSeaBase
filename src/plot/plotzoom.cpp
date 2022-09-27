#include "plotzoom.h"

#include "logging.h"
#include "zoomstack.h"
#include <QRubberBand>
#include "qwt_scale_map.h"
#include <QtDebug>
#include "qwtplotimpl.h"
#include "plot.h"

constexpr int MinimumZoom = 8;

PlotZoom::PlotZoom(QwtPlotImpl *plot) : plot(plot)
{DDD;

}

void PlotZoom::startZoom(QMouseEvent *mEvent)
{DDD;
    QRect cg = plot->canvas()->geometry();

    startingPosX = mEvent->pos().x();
    startingPosY = mEvent->pos().y();

    if (startingPosX >= 0 && startingPosX < cg.width() &&
        startingPosY >= 0 && startingPosY < cg.height()) {
        if (!rubberBand) rubberBand = new QRubberBand(QRubberBand::Rectangle, plot->canvas());
    }
}

void PlotZoom::proceedZoom(QMouseEvent *mEvent)
{DDD;
    int dx = mEvent->pos().x() - startingPosX;
    if (dx == 0) dx = 1;

    int dy = mEvent->pos().y() - startingPosY;
    if (dy == 0) dy = 1;
    // отображаем выделенную область
    if (rubberBand) {
        rubberBand->setGeometry(QRect(startingPosX, startingPosY, dx,dy).normalized());
        rubberBand->show();
    }
}

ZoomStack::zoomCoordinates PlotZoom::endZoom(QMouseEvent *mEvent)
{DDD;
    stopZoom();
    // определяем положение курсора, т.е. координаты xp и yp
    // конечной точки выделенной области (в пикселах относительно канвы QwtPlot)
    int xp = mEvent->pos().x();
    int yp = mEvent->pos().y();
    ZoomStack::zoomCoordinates coords;

    if (qAbs(xp - startingPosX) >= MinimumZoom && qAbs(yp - startingPosY) >= MinimumZoom) {
        int leftmostX = qMin(xp, startingPosX); int rightmostX = qMax(xp, startingPosX);
        int leftmostY = qMin(yp, startingPosY); int rightmostY = qMax(yp, startingPosY);
        QwtAxis::Position pos = QwtAxis::XBottom;
        const double xMin = plot->invTransform(pos,leftmostX);
        const double xMax = plot->invTransform(pos,rightmostX);

        pos = QwtAxis::YLeft;
        double yMin = plot->invTransform(pos,rightmostY);
        double yMax = plot->invTransform(pos,leftmostY);

        pos = QwtAxis::YRight;
        double ySMin = plot->invTransform(pos,rightmostY);
        double ySMax = plot->invTransform(pos,leftmostY);

        coords.coords.insert(Enums::AxisType::atBottom, {xMin, xMax});
        coords.coords.insert(Enums::AxisType::atLeft, {yMin, yMax});
        if (plot->parent->type() != Enums::PlotType::Spectrogram)
            coords.coords.insert(Enums::AxisType::atRight, {ySMin, ySMax});
    }
    return coords;
}

void PlotZoom::stopZoom()
{DDD;
    if (rubberBand) rubberBand->hide();
}

