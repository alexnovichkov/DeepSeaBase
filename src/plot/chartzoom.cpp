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

#include "chartzoom.h"
#include "plotzoom.h"
//#include "wheelzoom.h"
//#include "axiszoom.h"
//#include "dragzoom.h"
#include "plot.h"

#include <qwt_scale_widget.h>
#include "logging.h"

#include <QKeyEvent>

// Конструктор
ChartZoom::ChartZoom(Plot *plot) :
    QObject(plot),  qwtPlot(plot)
{DD;
    // сбрасываем признак режима
    convType = ctNone;

    // создаем контейнеры границ шкалы
    horizontalScaleBounds = new ScaleBounds(plot, QwtAxisId(QwtAxis::xBottom, 0));    // горизонтальной
    verticalScaleBounds = new ScaleBounds(plot, QwtAxisId(QwtAxis::yLeft, 0));    // и вертикальной
    verticalScaleBoundsSlave = new ScaleBounds(plot, QwtAxisId(QwtAxis::yRight, 0));

//    mainZoom = new PlotZoom();
//    mainZoom->attach(this);


//    axisZoom = new AxisZoom();
//    connect(axisZoom,SIGNAL(xAxisClicked(double,bool)),SIGNAL(updateTrackingCursorX(double, bool)));
//    connect(axisZoom,SIGNAL(yAxisClicked(double,bool)),SIGNAL(updateTrackingCursorY(double, bool)));
//    connect(axisZoom,SIGNAL(contextMenuRequested(QPoint,QwtAxisId)),SIGNAL(contextMenuRequested(QPoint,QwtAxisId)));
//    connect(axisZoom,SIGNAL(moveCursor(Enums::Direction)),SIGNAL(moveCursor(Enums::Direction)));
//    connect(axisZoom,SIGNAL(hover(QwtAxisId,int)),SIGNAL(hover(QwtAxisId,int)));
//    axisZoom->attach(this);
}

// Деструктор
ChartZoom::~ChartZoom()
{DD;
    // удаляем интерфейс перемещенния графика
//    delete dragZoom;
    // удаляем интерфейс масштабирования графика
//    delete mainZoom;
//    delete axisZoom;
//    delete wheelZoom;
    // удаляем контейнеры границ шкалы
    delete horizontalScaleBounds;    // горизонтальной
    delete verticalScaleBounds;    // и вертикальной
    delete verticalScaleBoundsSlave;
}

// Текущий режим масштабирования
ChartZoom::ConvType ChartZoom::regime()
{
    return convType;
}

// Переключение режима масштабирования
void ChartZoom::setRegime(ChartZoom::ConvType ct)
{DD;
    convType = ct;
    //emit setPickerEnabled(ct == ChartZoom::ctNone);
}

Plot *ChartZoom::plot() {
    return qwtPlot;
}

void ChartZoom::setZoomEnabled(bool enabled)
{DD;
    this->activated = enabled;
}

void ChartZoom::addZoom(ChartZoom::zoomCoordinates coords, bool addToStack)
{DD;
    if (coords.coords.isEmpty()) {
        return;
    }
    if (addToStack) zoomStack.push(coords);

    if (coords.coords.contains(QwtAxis::xBottom))
        horizontalScaleBounds->set(coords.coords.value(QwtAxis::xBottom).x(),
                                   coords.coords.value(QwtAxis::xBottom).y());
    if (coords.coords.contains(QwtAxis::yLeft)) {
        verticalScaleBounds->set(coords.coords.value(QwtAxis::yLeft).x(),
                                 coords.coords.value(QwtAxis::yLeft).y());
    }
    if (coords.coords.contains(QwtAxis::yRight)/* && !qwtPlot->spectrogram*/) {
        verticalScaleBoundsSlave->set(coords.coords.value(QwtAxis::yRight).x(),
                                      coords.coords.value(QwtAxis::yRight).y());
    }
    plot()->replot();
}

void ChartZoom::zoomBack()
{DD;
    if (zoomStack.isEmpty()) return;
    zoomStack.pop();
    if (zoomStack.isEmpty()) {
        // nothing to zoom back to, autoscaling to maximum
        plot()->autoscale();
    }
    else {
        zoomCoordinates coords = zoomStack.top();
        if (coords.coords.contains(QwtAxis::xBottom))
            horizontalScaleBounds->set(coords.coords.value(QwtAxis::xBottom).x(),
                                       coords.coords.value(QwtAxis::xBottom).y());
        if (coords.coords.contains(QwtAxis::yLeft)) {
            verticalScaleBounds->set(coords.coords.value(QwtAxis::yLeft).x(),
                                     coords.coords.value(QwtAxis::yLeft).y());
        }
        if (coords.coords.contains(QwtAxis::yRight)) {
            verticalScaleBoundsSlave->set(coords.coords.value(QwtAxis::yRight).x(),
                                          coords.coords.value(QwtAxis::yRight).y());
        }
    }
    // перестраиваем график
    plot()->replot();
}

void ChartZoom::moveToAxis(int axis, double min, double max)
{
    switch (axis) {
        case QwtAxis::xBottom:
            horizontalScaleBounds->add(min, max);
            if (!horizontalScaleBounds->isFixed()) horizontalScaleBounds->autoscale();
            break;
        case QwtAxis::yLeft:
            verticalScaleBoundsSlave->removeToAutoscale(min, max);
            verticalScaleBounds->add(min, max);
            if (!verticalScaleBounds->isFixed()) verticalScaleBounds->autoscale();
            break;
        case QwtAxis::yRight:
            verticalScaleBounds->removeToAutoscale(min, max);
            verticalScaleBoundsSlave->add(min, max);
            if (!verticalScaleBoundsSlave->isFixed()) verticalScaleBoundsSlave->autoscale();
            break;
        default: break;
    }
}

void ChartZoom::autoscale(int axis, bool spectrogram)
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
ChartZoom::ScaleBounds::ScaleBounds(Plot *plot, QwtAxisId axis) : axis(axis), plot(plot)
{DD;
    fixed = false;  // границы еще не фиксированы
    min = 0.0;
    max = 10.0;

    set(min, max);
}

void ChartZoom::ScaleBounds::setFixed(bool fixed)
{DD;
    this->fixed = fixed;

    if (!fixed)
        autoscale();
}

// Фиксация исходных границ шкалы
void ChartZoom::ScaleBounds::add(double min, double max)
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

void ChartZoom::ScaleBounds::set(double min, double max)
{DD;
    plot->setScale(axis, min,max);
}

// Восстановление исходных границ шкалы
void ChartZoom::ScaleBounds::reset()
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

void ChartZoom::ScaleBounds::autoscale()
{DD;
    auto iter = std::min_element(mins.constBegin(), mins.constEnd());
    double minn = iter==mins.constEnd() ? this->min : *iter;
    iter = std::max_element(maxes.constBegin(), maxes.constEnd());
    double maxx = iter==maxes.constEnd() ? this->max : *iter;

    set(minn, maxx);
}

void ChartZoom::ScaleBounds::removeToAutoscale(double min, double max)
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

