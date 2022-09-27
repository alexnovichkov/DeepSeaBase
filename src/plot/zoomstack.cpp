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
#include "plotinterface.h"

#include "logging.h"

#include <QKeyEvent>

ZoomStack::ZoomStack(Plot *plot) : QObject(plot),  plot(plot)
{DDD;
    horizontalScaleBounds = new ScaleBounds(plot,  Enums::AxisType::atBottom);
    verticalScaleBounds = new ScaleBounds(plot, Enums::AxisType::atLeft);
    verticalScaleBoundsSlave = new ScaleBounds(plot, Enums::AxisType::atRight);
}

ZoomStack::~ZoomStack()
{DDD;
    delete horizontalScaleBounds;
    delete verticalScaleBounds;
    delete verticalScaleBoundsSlave;
}

void ZoomStack::addZoom(ZoomStack::zoomCoordinates coords, bool addToStack)
{DDD;
    if (coords.coords.isEmpty()) {
        return;
    }
    if (addToStack) {
        zoomStack.push(coords);
    }

    if (coords.coords.contains(Enums::AxisType::atBottom))
        horizontalScaleBounds->set(coords.coords.value(Enums::AxisType::atBottom).x(),
                                   coords.coords.value(Enums::AxisType::atBottom).y());
    if (coords.coords.contains(Enums::AxisType::atLeft)) {
        verticalScaleBounds->set(coords.coords.value(Enums::AxisType::atLeft).x(),
                                 coords.coords.value(Enums::AxisType::atLeft).y());
    }
    if (coords.coords.contains(Enums::AxisType::atRight)/* && !qwtPlot->spectrogram*/) {
        verticalScaleBoundsSlave->set(coords.coords.value(Enums::AxisType::atRight).x(),
                                      coords.coords.value(Enums::AxisType::atRight).y());
    }
    plot->impl()->replot();
}

void ZoomStack::zoomBack()
{DDD;
    if (zoomStack.isEmpty()) return;
    zoomStack.pop();
    if (zoomStack.isEmpty()) {
        // nothing to zoom back to, autoscaling to maximum
        plot->autoscale();
    }
    else {
        zoomCoordinates coords = zoomStack.top();
        if (coords.coords.contains(Enums::AxisType::atBottom))
            horizontalScaleBounds->set(coords.coords.value(Enums::AxisType::atBottom).x(),
                                       coords.coords.value(Enums::AxisType::atBottom).y());
        if (coords.coords.contains(Enums::AxisType::atLeft)) {
            verticalScaleBounds->set(coords.coords.value(Enums::AxisType::atLeft).x(),
                                     coords.coords.value(Enums::AxisType::atLeft).y());
        }
        if (coords.coords.contains(Enums::AxisType::atRight)) {
            verticalScaleBoundsSlave->set(coords.coords.value(Enums::AxisType::atRight).x(),
                                          coords.coords.value(Enums::AxisType::atRight).y());
        }
    }
    plot->impl()->replot();
}

void ZoomStack::moveToAxis(Enums::AxisType axis, double min, double max)
{DDD;
    switch (axis) {
        case Enums::AxisType::atBottom:
            horizontalScaleBounds->add(min, max);
            if (!horizontalScaleBounds->isFixed()) horizontalScaleBounds->autoscale();
            break;
        case Enums::AxisType::atLeft:
            verticalScaleBoundsSlave->removeToAutoscale(min, max);
            verticalScaleBounds->add(min, max);
            if (!verticalScaleBounds->isFixed()) verticalScaleBounds->autoscale();
            break;
        case Enums::AxisType::atRight:
            verticalScaleBounds->removeToAutoscale(min, max);
            verticalScaleBoundsSlave->add(min, max);
            if (!verticalScaleBoundsSlave->isFixed()) verticalScaleBoundsSlave->autoscale();
            break;
        default: break;
    }
}

void ZoomStack::autoscale(Enums::AxisType axis, bool spectrogram)
{DDD;
    switch (axis) {
        case Enums::AxisType::atBottom: // x axis
            horizontalScaleBounds->autoscale();
            break;
        case Enums::AxisType::atLeft: // y axis
            verticalScaleBounds->autoscale();
            break;
        case Enums::AxisType::atRight: // y slave axis
            verticalScaleBoundsSlave->autoscale();
            break;
        case Enums::AxisType::atInvalid:
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
ZoomStack::ScaleBounds::ScaleBounds(Plot *plot, Enums::AxisType axis) : axis(axis), plot(plot)
{DDD;
    fixed = false;  // границы еще не фиксированы
    min = 0.0;
    max = 10.0;

    set(min, max);
}

void ZoomStack::ScaleBounds::setFixed(bool fixed)
{DDD;
    this->fixed = fixed;

    if (!fixed)
        autoscale();
}

// Фиксация исходных границ шкалы
void ZoomStack::ScaleBounds::add(double min, double max)
{DDD;
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
{DDD;
    plot->setScale(axis, min, max);
}

// Восстановление исходных границ шкалы
void ZoomStack::ScaleBounds::reset()
{DDD;
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
{DDD;
    auto iter = std::min_element(mins.constBegin(), mins.constEnd());
    double minn = iter==mins.constEnd() ? this->min : *iter;
    iter = std::max_element(maxes.constBegin(), maxes.constEnd());
    double maxx = iter==maxes.constEnd() ? this->max : *iter;

    set(minn, maxx);
}

void ZoomStack::ScaleBounds::removeToAutoscale(double min, double max)
{DDD;
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

