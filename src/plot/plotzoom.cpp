#include "plotzoom.h"

/**********************************************************/
/*                                                        */
/*             Реализация класса QMainZoomSvc             */
/*                      Версия 1.0.1                      */
/*                                                        */
/* Разработал Мельников Сергей Андреевич,                 */
/* г. Каменск-Уральский Свердловской обл., 2012 г.,       */
/* при поддержке Ю. А. Роговского, г. Новосибирск.        */
/*                                                        */
/* Разрешается свободное использование и распространение. */
/* Упоминание автора обязательно.                         */
/*                                                        */
/**********************************************************/

#include "logging.h"
#include "chartzoom.h"
#include <QRubberBand>
#include "qwt_scale_map.h"
#include <QtDebug>
#include "plot.h"

constexpr int MinimumZoom = 8;

PlotZoom::PlotZoom(Plot *plot) : plot(plot)
{DD;

}

void PlotZoom::startZoom(QMouseEvent *mEvent)
{
    QRect cg = plot->canvas()->geometry();

    startingPosX = mEvent->pos().x();
    startingPosY = mEvent->pos().y();

    if (startingPosX >= 0 && startingPosX < cg.width() &&
        startingPosY >= 0 && startingPosY < cg.height()) {
        if (!rubberBand) rubberBand = new QRubberBand(QRubberBand::Rectangle, plot->canvas());
    }
}

void PlotZoom::proceedZoom(QMouseEvent *mEvent)
{
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

ChartZoom::zoomCoordinates PlotZoom::endZoom(QMouseEvent *mEvent)
{
    stopZoom();
    // определяем положение курсора, т.е. координаты xp и yp
    // конечной точки выделенной области (в пикселах относительно канвы QwtPlot)
    int xp = mEvent->pos().x();
    int yp = mEvent->pos().y();
    ChartZoom::zoomCoordinates coords;

    if (qAbs(xp - startingPosX) >= MinimumZoom && qAbs(yp - startingPosY) >= MinimumZoom) {
        int leftmostX = qMin(xp, startingPosX); int rightmostX = qMax(xp, startingPosX);
        int leftmostY = qMin(yp, startingPosY); int rightmostY = qMax(yp, startingPosY);
        QwtAxis::Position pos = QwtAxis::xBottom;
        const double xMin = plot->invTransform(pos,leftmostX);
        const double xMax = plot->invTransform(pos,rightmostX);

        pos = QwtAxis::yLeft;
        double yMin = plot->invTransform(pos,rightmostY);
        double yMax = plot->invTransform(pos,leftmostY);

        pos = QwtAxis::yRight;
        double ySMin = plot->invTransform(pos,rightmostY);
        double ySMax = plot->invTransform(pos,leftmostY);

        coords.coords.insert(QwtAxis::xBottom, {xMin, xMax});
        coords.coords.insert(QwtAxis::yLeft, {yMin, yMax});
        if (!plot->spectrogram)
            coords.coords.insert(QwtAxis::yRight, {ySMin, ySMax});
    }
    return coords;
}

void PlotZoom::stopZoom()
{
    if (rubberBand) rubberBand->hide();
}

