/**********************************************************/
/*                                                        */
/*             Реализация класса QwtChartZoom             */
/*                      Версия 1.5.2                      */
/*                                                        */
/* Разработал Мельников Сергей Андреевич,                 */
/* г. Каменск-Уральский Свердловской обл., 2012 г.,       */
/* при поддержке Ю. А. Роговского, г. Новосибирск.        */
/*                                                        */
/* Разрешается свободное использование и распространение. */
/* Упоминание автора обязательно.                         */
/*                                                        */
/**********************************************************/

#include "zoomstack.h"
#include "plot.h"

#include <qwt_scale_widget.h>
#include "logging.h"

#include <QKeyEvent>

ZoomStack::ZoomStack(Plot *plot) : QObject(plot),  plot(plot)
{DD;
    horizontalScaleBounds = new ScaleBounds(plot, QwtAxis::XBottom);
    verticalScaleBounds = new ScaleBounds(plot, QwtAxis::YLeft);
    verticalScaleBoundsSlave = new ScaleBounds(plot, QwtAxis::YRight);
}

ZoomStack::~ZoomStack()
{DD;
    delete horizontalScaleBounds;
    delete verticalScaleBounds;
    delete verticalScaleBoundsSlave;
}

void ZoomStack::addZoom(ZoomStack::zoomCoordinates coords, bool addToStack)
{DD;
    if (coords.coords.isEmpty()) {
        return;
    }
    if (addToStack) {
        zoomStack.push(coords);
    }

    if (coords.coords.contains(QwtAxis::XBottom))
        horizontalScaleBounds->set(coords.coords.value(QwtAxis::XBottom).x(),
                                   coords.coords.value(QwtAxis::XBottom).y());
    if (coords.coords.contains(QwtAxis::YLeft)) {
        verticalScaleBounds->set(coords.coords.value(QwtAxis::YLeft).x(),
                                 coords.coords.value(QwtAxis::YLeft).y());
    }
    if (coords.coords.contains(QwtAxis::YRight)/* && !qwtPlot->spectrogram*/) {
        verticalScaleBoundsSlave->set(coords.coords.value(QwtAxis::YRight).x(),
                                      coords.coords.value(QwtAxis::YRight).y());
    }
    plot->replot();
}

void ZoomStack::zoomBack()
{DD;
    if (zoomStack.isEmpty()) return;
    zoomStack.pop();
    if (zoomStack.isEmpty()) {
        // nothing to zoom back to, autoscaling to maximum
        plot->autoscale();
    }
    else {
        zoomCoordinates coords = zoomStack.top();
        if (coords.coords.contains(QwtAxis::XBottom))
            horizontalScaleBounds->set(coords.coords.value(QwtAxis::XBottom).x(),
                                       coords.coords.value(QwtAxis::XBottom).y());
        if (coords.coords.contains(QwtAxis::YLeft)) {
            verticalScaleBounds->set(coords.coords.value(QwtAxis::YLeft).x(),
                                     coords.coords.value(QwtAxis::YLeft).y());
        }
        if (coords.coords.contains(QwtAxis::YRight)) {
            verticalScaleBoundsSlave->set(coords.coords.value(QwtAxis::YRight).x(),
                                          coords.coords.value(QwtAxis::YRight).y());
        }
    }
    plot->replot();
}

void ZoomStack::moveToAxis(int axis, double min, double max)
{
    switch (axis) {
        case QwtAxis::XBottom:
            horizontalScaleBounds->add(min, max);
            if (!horizontalScaleBounds->isFixed()) horizontalScaleBounds->autoscale();
            break;
        case QwtAxis::YLeft:
            verticalScaleBoundsSlave->removeToAutoscale(min, max);
            verticalScaleBounds->add(min, max);
            if (!verticalScaleBounds->isFixed()) verticalScaleBounds->autoscale();
            break;
        case QwtAxis::YRight:
            verticalScaleBounds->removeToAutoscale(min, max);
            verticalScaleBoundsSlave->add(min, max);
            if (!verticalScaleBoundsSlave->isFixed()) verticalScaleBoundsSlave->autoscale();
            break;
        default: break;
    }
}

void ZoomStack::autoscale(int axis, bool spectrogram)
{DD;
    switch (axis) {
        case 0: // x axis
            horizontalScaleBounds->autoscale();
            break;
        case 1: // y axis
            verticalScaleBounds->autoscale();
            break;
        case 2: // y slave axis
            verticalScaleBoundsSlave->autoscale();
            break;
        case -1:
            zoomStack.clear();
            horizontalScaleBounds->autoscale();
            verticalScaleBounds->autoscale();
            if (!spectrogram) verticalScaleBoundsSlave->autoscale();
            //replot();
            //update();
            break;
        default:
            break;
    }
}

    /**************************************************/
    /*         Реализация класса QScaleBounds         */
    /*                  Версия 1.0.1                  */
    /**************************************************/

// Конструктор
ZoomStack::ScaleBounds::ScaleBounds(Plot *plot, QwtAxisId axis) : axis(axis), plot(plot)
{DD;
    fixed = false;  // границы еще не фиксированы
    min = 0.0;
    max = 10.0;

    set(min, max);
}

void ZoomStack::ScaleBounds::setFixed(bool fixed)
{DD;
    this->fixed = fixed;

    if (!fixed)
        autoscale();
}

// Фиксация исходных границ шкалы
void ZoomStack::ScaleBounds::add(double min, double max)
{DD;
    if (min==max) {
        if (min!=0.0) {
            mins << (min-1.0);
            maxes << (max+1.0);
        }
    }
    else {
        mins << min;
        maxes << max;
    }

    if (!fixed) {
        autoscale();
    }
}

void ZoomStack::ScaleBounds::set(double min, double max)
{DD;
    plot->setScale(axis, min,max);
}

// Восстановление исходных границ шкалы
void ZoomStack::ScaleBounds::reset()
{DD;
    // если границы уже фиксированы, то восстанавливаем исходные
    if (fixed) {
        //set(min,max);
    }
    else {
        mins.clear();
        maxes.clear();
        set(min, max);
    }
}

void ZoomStack::ScaleBounds::autoscale()
{DD;
    auto iter = std::min_element(mins.constBegin(), mins.constEnd());
    double minn = iter==mins.constEnd() ? this->min : *iter;
    iter = std::max_element(maxes.constBegin(), maxes.constEnd());
    double maxx = iter==maxes.constEnd() ? this->max : *iter;

    set(minn, maxx);
}

void ZoomStack::ScaleBounds::removeToAutoscale(double min, double max)
{DD;
    if (min==max) {
        if (min != 0.0) {
            mins.removeOne(min-1.0);
            maxes.removeOne(max+1.0);
        }
    }
    else {
        mins.removeOne(min);
        maxes.removeOne(max);
    }
}

