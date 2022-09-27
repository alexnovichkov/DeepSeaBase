#include "wheelzoom.h"
#include "logging.h"
#include <QtMath>
#include "plot.h"

WheelZoom::WheelZoom(Plot *plot) : QObject(plot), plot(plot)
{DDD;

}

ZoomStack::zoomCoordinates WheelZoom::applyWheel(QEvent *event, Enums::AxisType axis)
{DDD;
    QWheelEvent *wEvent = static_cast<QWheelEvent *>(event);

    // определяем угол поворота колеса мыши
    // (значение 120 соответствует углу поворота 15°)
    QPoint delta = wEvent->angleDelta();
    const int wheelDelta = delta.y();

    ZoomStack::zoomCoordinates coords;

    if (wheelDelta != 0) {   // если колесо вращалось
        double wheelSteps = wheelDelta/120.0;
        double factor = qPow(0.85, wheelSteps);

        if (axis != Enums::AxisType::atInvalid) {
            int pos = (axis == Enums::AxisType::atBottom || axis == Enums::AxisType::atTop) ? wEvent->pos().x() : wEvent->pos().y();
            coords.coords.insert(axis, getCoords(axis, pos, factor));
        }
        else {
            int pos = wEvent->pos().x();
            axis = Enums::AxisType::atBottom;
            coords.coords.insert(axis, getCoords(axis, pos, factor));
            pos = wEvent->pos().y();
            axis = Enums::AxisType::atLeft;
            coords.coords.insert(axis, getCoords(axis, pos, factor));
            if (plot->type() != Enums::PlotType::Spectrogram) {
                axis = Enums::AxisType::atRight;
                coords.coords.insert(axis, getCoords(axis, pos, factor));
            }
        }
    }
    return coords;
}

QPointF WheelZoom::getCoords(Enums::AxisType axis, int pos, double factor)
{DDD;
    double dPos = plot->screenToPlotCoordinates(axis, pos);
    auto range = plot->plotRange(axis);
    double lower = (range.min-dPos)*factor + dPos;
    double upper = (range.max-dPos)*factor + dPos;
    return QPointF(lower, upper);
}

