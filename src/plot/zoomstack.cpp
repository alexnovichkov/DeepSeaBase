#include "zoomstack.h"
#include "plot.h"
#include "logging.h"
#include "algorithms.h"
#include "qcpplot.h"

ZoomStack::ZoomStack(Plot *plot) : QObject(plot),  m_plot(plot)
{DDD;
    m_scaleBounds.insert(Enums::AxisType::atTop, new ScaleBounds(plot,  Enums::AxisType::atTop));
    m_scaleBounds.insert(Enums::AxisType::atBottom, new ScaleBounds(plot,  Enums::AxisType::atBottom));
    m_scaleBounds.insert(Enums::AxisType::atLeft, new ScaleBounds(plot, Enums::AxisType::atLeft));
    m_scaleBounds.insert(Enums::AxisType::atRight, new ScaleBounds(plot, Enums::AxisType::atRight));
    m_scaleBounds.insert(Enums::AxisType::atColor, new ScaleBounds(plot, Enums::AxisType::atColor));
}

ZoomStack::~ZoomStack()
{DDD;
    for (auto b: m_scaleBounds.values()) delete b;
}

ZoomStack::ScaleBounds *ZoomStack::scaleBounds(Enums::AxisType axis)
{
    return m_scaleBounds.value(axis, nullptr);
}

void ZoomStack::addZoom(ZoomStack::zoomCoordinates coords, bool addToStack)
{DDD;
    if (coords.coords.isEmpty()) {
        return;
    }
    if (addToStack) {
        m_zoomStack.push(coords);
    }

    for (auto [key, val] : asKeyValueRange(coords.coords)) {
        scaleBounds(key)->set(val.x(), val.y());
    }

    m_plot->impl()->replot();
}

void ZoomStack::zoomBack()
{DDD;
    if (m_zoomStack.isEmpty()) return;
    m_zoomStack.pop();
    if (m_zoomStack.isEmpty()) {
        // nothing to zoom back to, autoscaling to maximum
        m_plot->autoscale();
    }
    else {
        zoomCoordinates coords = m_zoomStack.top();
        for (auto [key, val] : asKeyValueRange(coords.coords)) {
            scaleBounds(key)->set(val.x(), val.y());
        }
    }
    m_plot->impl()->replot();
}

void ZoomStack::moveToAxis(Enums::AxisType axis, double min, double max)
{DDD;
    auto b = scaleBounds(axis);
    if (!b) return;
    b->add(min, max);
    if (!b->isFixed()) b->autoscale();

    switch (axis) {
        case Enums::AxisType::atLeft:
            scaleBounds(Enums::AxisType::atRight)->removeToAutoscale(min, max);
            break;
        case Enums::AxisType::atRight:
            scaleBounds(Enums::AxisType::atLeft)->removeToAutoscale(min, max);
            break;
        default: break;
    }
}

void ZoomStack::autoscale(Enums::AxisType axis)
{DDD;
    auto b = scaleBounds(axis);
    if (b) {
        b->autoscale();
    }
    else {
        m_zoomStack.clear();
        for (auto b: m_scaleBounds.values()) b->autoscale();
    }
    m_plot->replot();
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
void ZoomStack::ScaleBounds::add(double min, double max, bool removePrevious)
{DDD;
    if (removePrevious) {
        mins.clear();
        maxes.clear();
    }
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

