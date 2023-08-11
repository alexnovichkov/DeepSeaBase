#include "zoomstack.h"
#include "qcpplot.h"
#include "logging.h"
#include "algorithms.h"

ZoomStack::ZoomStack(QCPPlot *plot) : QObject(plot),  m_plot(plot)
{DD;
    m_scaleBounds.insert(Enums::AxisType::atTop, new ScaleBounds(plot,  Enums::AxisType::atTop));
    m_scaleBounds.insert(Enums::AxisType::atBottom, new ScaleBounds(plot,  Enums::AxisType::atBottom));
    m_scaleBounds.insert(Enums::AxisType::atLeft, new ScaleBounds(plot, Enums::AxisType::atLeft));
    m_scaleBounds.insert(Enums::AxisType::atRight, new ScaleBounds(plot, Enums::AxisType::atRight));
    m_scaleBounds.insert(Enums::AxisType::atColor, new ScaleBounds(plot, Enums::AxisType::atColor));
}

ZoomStack::~ZoomStack()
{DD;
    for (auto b: m_scaleBounds.values()) delete b;
}

ZoomStack::ScaleBounds *ZoomStack::scaleBounds(Enums::AxisType axis)
{
    return m_scaleBounds.value(axis, nullptr);
}

void ZoomStack::addZoom(ZoomStack::zoomCoordinates coords, bool addToStack)
{DD;
    if (coords.isEmpty()) {
        return;
    }
    if (addToStack) {
        m_zoomStack.push(coords);
    }

    for (auto [key, val] : asKeyValueRange(coords)) {
        scaleBounds(key)->set(val.lower, val.upper);
    }

    emit replotNeeded();
}

void ZoomStack::zoomBack()
{DD;
    if (m_zoomStack.isEmpty()) return;
    m_zoomStack.pop();
    if (m_zoomStack.isEmpty()) {
        // nothing to zoom back to, autoscaling to maximum
        m_plot->autoscale();
    }
    else {
        zoomCoordinates coords = m_zoomStack.top();
        for (auto [key, val] : asKeyValueRange(coords)) {
            scaleBounds(key)->set(val.lower, val.upper);
        }
    }
    emit replotNeeded();
}

void ZoomStack::moveToAxis(Enums::AxisType axis, double min, double max)
{DD;
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
{DD;
    auto b = scaleBounds(axis);
    if (b) {
        b->autoscale();
    }
    else {
        m_zoomStack.clear();
        for (auto b: m_scaleBounds.values()) b->autoscale();
    }
    emit replotNeeded();
}

    /**************************************************/
    /*         Реализация класса QScaleBounds         */
    /*                  Версия 1.0.1                  */
    /**************************************************/

// Конструктор
ZoomStack::ScaleBounds::ScaleBounds(QCPPlot *plot, Enums::AxisType axis) : axis(axis), plot(plot)
{DD;
    fixed = false;  // границы еще не фиксированы
    min = 0.0;
    max = 10.0;

    set(min, max);
}

void ZoomStack::ScaleBounds::setFixed(bool fixed, bool rescale)
{DD;
    this->fixed = fixed;

    if (!fixed && rescale)
        autoscale();
}

// Фиксация исходных границ шкалы
void ZoomStack::ScaleBounds::add(double min, double max, bool removePrevious)
{DD;
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
{DD;
    plot->setAxisRange(axis, min, max);
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


QDebug operator<<(QDebug dbg, const ZoomStack::ScaleBounds &b)
{
    dbg.nospace() << "ScaleBounds(" << static_cast<int>(b.axis) <<
                     b.fixed << " min: "<<b.min << " "<<b.mins <<
                     " max: "<<b.max << " "<<b.maxes << ")";
    return dbg.space();
}
