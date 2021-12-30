#include "wheelzoom.h"
#include "qwt_scale_widget.h"
#include "logging.h"
#include <QtMath>
#include "plot.h"

WheelZoom::WheelZoom(Plot *plot) : QObject(plot), plot(plot)
{DD;

}

ZoomStack::zoomCoordinates WheelZoom::applyWheel(QEvent *event, QwtAxisId axis)
{DD;
    QWheelEvent *wEvent = static_cast<QWheelEvent *>(event);

    // определяем угол поворота колеса мыши
    // (значение 120 соответствует углу поворота 15°)
    QPoint delta = wEvent->angleDelta();
    const int wheelDelta = delta.y();

    ZoomStack::zoomCoordinates coords;

    if (wheelDelta != 0) {   // если колесо вращалось
        double wheelSteps = wheelDelta/120.0;
        double factor = qPow(0.85, wheelSteps);

        if (QwtAxis::isValid(axis.pos)) {
            int pos = axis.isXAxis() ? wEvent->pos().x() : wEvent->pos().y();
            coords.coords.insert(axis.pos, getCoords(axis, pos, factor));
        }
        else {
            int pos = wEvent->pos().x();
            axis = QwtAxisId(QwtAxis::xBottom, 0);
            coords.coords.insert(axis.pos, getCoords(axis, pos, factor));
            pos = wEvent->pos().y();
            axis = QwtAxisId(QwtAxis::yLeft, 0);
            coords.coords.insert(axis.pos, getCoords(axis, pos, factor));
            if (plot->type() != Plot::PlotType::Spectrogram) {
                axis = QwtAxisId(QwtAxis::yRight, 0);
                coords.coords.insert(axis.pos, getCoords(axis, pos, factor));
            }
        }
    }
    return coords;
}

QPointF WheelZoom::getCoords(QwtAxisId axis, int pos, double factor)
{
    double dPos = plot->invTransform(axis, pos);
    QwtScaleMap sm = plot->canvasMap(axis);
    double lower = (sm.s1()-dPos)*factor + dPos;
    double upper = (sm.s2()-dPos)*factor + dPos;
    return QPointF(lower, upper);
}

