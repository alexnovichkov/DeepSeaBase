/**********************************************************/
/*                                                        */
/*            Реализация класса QWheelZoomSvc             */
/*                      Версия 1.0.3                      */
/*                                                        */
/* Разработал Мельников Сергей Андреевич,                 */
/* г. Каменск-Уральский Свердловской обл., 2012 г.,       */
/* при поддержке Ю. А. Роговского, г. Новосибирск.        */
/*                                                        */
/* Разрешается свободное использование и распространение. */
/* Упоминание автора обязательно.                         */
/*                                                        */
/**********************************************************/

#include "wheelzoom.h"
#include "qwt_scale_widget.h"
#include <QApplication>
#include "logging.h"
#include <QtMath>
#include "plot.h"

WheelZoom::WheelZoom() : QObject()
{DD;

}

void WheelZoom::attach(ChartZoom *zm)
{DD;
    zoom = zm;

    zoom->plot()->canvas()->installEventFilter(this);
    for (int ax = 0; ax < QwtAxis::PosCount; ax++) {
        for (int j = 0; j < zoom->plot()->axesCount(ax); ++j)
            zoom->plot()->axisWidget(QwtAxisId(ax,j))->installEventFilter(this);
    }
}

bool WheelZoom::eventFilter(QObject *target, QEvent *event)
{
    if (event->type() != QEvent::Wheel)
        return QObject::eventFilter(target,event);

    QwtAxisId axis(-1,0);
    if (target == zoom->plot()->canvas()) {
        applyWheel(event, axis);
    }
    else {
        for (int a=0; a < QwtAxis::PosCount; ++a) {
            for (int j=0; j<zoom->plot()->axesCount(a); ++j)
            if (target == zoom->plot()->axisWidget(QwtAxisId(a,j))) {
                axis = QwtAxisId(a,j);
                break;
            }
        }
        if (axis.pos != -1) {
            applyWheel(event, axis);
        }
    }

    return true;
}

void WheelZoom::applyWheel(QEvent *event, QwtAxisId axis)
{DD;
    QWheelEvent *wEvent = static_cast<QWheelEvent *>(event);
    if (wEvent->orientation() != Qt::Vertical) return;

    // определяем угол поворота колеса мыши
    // (значение 120 соответствует углу поворота 15°)
    QPoint delta = wEvent->angleDelta();
    const int wheelDelta = delta.y();

    if (wheelDelta != 0) {   // если колесо вращалось
        double wheelSteps = wheelDelta/120.0;
        double factor = qPow(0.85, wheelSteps);

        ChartZoom::zoomCoordinates coords;

        if (QwtAxis::isValid(axis.pos)) {
            int pos = axis.isXAxis() ? wEvent->pos().x() : wEvent->pos().y();
            coords.coords.insert(axis.pos, getCoords(axis, pos, factor));
        }
        else {
            int pos = wEvent->pos().x();
            //: wEvent->pos().y();
            axis = QwtAxisId(QwtAxis::xBottom, 0);
            coords.coords.insert(axis.pos, getCoords(axis, pos, factor));
            pos = wEvent->pos().y();
            axis = QwtAxisId(QwtAxis::yLeft, 0);
            coords.coords.insert(axis.pos, getCoords(axis, pos, factor));
            if (!zoom->plot()->spectrogram) {
                axis = QwtAxisId(QwtAxis::yRight, 0);
                coords.coords.insert(axis.pos, getCoords(axis, pos, factor));
            }
        }

        zoom->addZoom(coords, true);
    }
}

QPointF WheelZoom::getCoords(QwtAxisId axis, int pos, double factor)
{
    double dPos = zoom->plot()->invTransform(axis, pos);
    QwtScaleMap sm = zoom->plot()->canvasMap(axis);
    double lower = (sm.s1()-dPos)*factor + dPos;
    double upper = (sm.s2()-dPos)*factor + dPos;
    return QPointF(lower, upper);
}

