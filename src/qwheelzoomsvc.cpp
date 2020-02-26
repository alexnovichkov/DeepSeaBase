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

#include "qwheelzoomsvc.h"
#include "qwt_scale_widget.h"
#include <QApplication>
#include "logging.h"

QWheelZoomSvc::QWheelZoomSvc() : QObject()
{DD;

}

void QWheelZoomSvc::attach(ChartZoom *zm)
{DD;
    zoom = zm;

    zoom->plot()->canvas()->installEventFilter(this);
    for (int axis=0; axis < QwtPlot::axisCnt; ++axis)
        zoom->plot()->axisWidget(axis)->installEventFilter(this);
}

bool QWheelZoomSvc::eventFilter(QObject *target, QEvent *event)
{
    if (event->type() != QEvent::Wheel)
        return QObject::eventFilter(target,event);

    if (target == zoom->plot()->canvas()) {
        applyWheel(event);
    }
    else {
        int axis = -1;
        for (int a=0; a < QwtPlot::axisCnt; ++a) {
            if (target == zoom->plot()->axisWidget(a)) {
                axis = a;
                break;
            }
        }
        if (axis != -1) {
            applyWheel(event, axis);
        }
    }

    return true;
}

void QWheelZoomSvc::applyWheel(QEvent *event, int axis)
{DD;
    QWheelEvent *wEvent = static_cast<QWheelEvent *>(event);
    if (wEvent->orientation() != Qt::Vertical) return;

    // определяем угол поворота колеса мыши
    // (значение 120 соответствует углу поворота 15°)
    QPoint delta = wEvent->angleDelta();
    const int wheelDelta = delta.y();

    if (wheelDelta != 0) {   // если колесо вращалось
        QwtPlot *plot = zoom->plot();

        double wheelSteps = wheelDelta/120.0;
        double factor = pow(0.85, wheelSteps);

        ChartZoom::zoomCoordinates coords;

        if (axis == -1 || axis == QwtPlot::xBottom) {
            double horPos = plot->invTransform(QwtPlot::xBottom, wEvent->pos().x());
            QwtScaleMap sm = plot->canvasMap(QwtPlot::xBottom);

            double lower = (sm.s1()-horPos)*factor + horPos;
            double upper = (sm.s2()-horPos)*factor + horPos;
            coords.coords.insert(QwtPlot::xBottom, {lower, upper});
        }
        if (axis == -1 || axis == QwtPlot::yLeft) {
            double verPos = plot->invTransform(QwtPlot::yLeft, wEvent->pos().y());
            QwtScaleMap sm = plot->canvasMap(QwtPlot::yLeft);
            double lower = (sm.s1()-verPos)*factor + verPos;
            double upper = (sm.s2()-verPos)*factor + verPos;
            coords.coords.insert(QwtPlot::yLeft, {lower, upper});
        }
        if (axis == -1 || axis == QwtPlot::yRight) {
            double verPos = plot->invTransform(QwtPlot::yRight, wEvent->pos().y());
            QwtScaleMap sm = plot->canvasMap(QwtPlot::yRight);
            double lower = (sm.s1()-verPos)*factor + verPos;
            double upper = (sm.s2()-verPos)*factor + verPos;
            coords.coords.insert(QwtPlot::yRight, {lower, upper});
        }
        if (!coords.coords.isEmpty()) zoom->addZoom(coords, true);
    }
}

