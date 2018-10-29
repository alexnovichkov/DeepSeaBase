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
#include "qmainzoomsvc.h"
#include "qwheelzoomsvc.h"
#include "qaxiszoomsvc.h"
#include "qdragzoomsvc.h"

#include <qwt_scale_widget.h>
#include "logging.h"

#include <QKeyEvent>

// Конструктор
ChartZoom::ChartZoom(QwtPlot *qp) :
    QObject(qp),  qwtPlot(qp)
{DD;
    // получаем главное окно
    mwin = generalParent(qp);
    // и назначаем обработчик событий (фильтр событий)
    mwin->installEventFilter(this);

    // сбрасываем признак режима
    convType = ctNone;

    // устанавливаем ему свойство, разрешающее обрабатывать события от клавиатуры
    qwtPlot->setFocusPolicy(Qt::StrongFocus);

    // назначаем основную и дополнительную шкалу
    masterX = QwtPlot::xBottom;
    slaveX = QwtPlot::xTop;

    masterY = QwtPlot::yLeft;
    slaveY = QwtPlot::yRight;

    // создаем контейнеры границ шкалы
    horizontalScaleBounds = new ScaleBounds(qp, masterX);    // горизонтальной
    verticalScaleBounds = new ScaleBounds(qp, masterY);    // и вертикальной
    verticalScaleBoundsSlave = new ScaleBounds(qp, slaveY);

    // устанавливаем обработчик всех событий
    qwtPlot->installEventFilter(this);
    // для всех шкал графика
    for (int ax=0; ax < QwtPlot::axisCnt; ax++) {
        // назначаем обработчик событий (фильтр событий)
        qwtPlot->axisWidget(ax)->installEventFilter(this);
        qwtPlot->axisWidget(ax)->setFocusPolicy(Qt::StrongFocus);
    }

    // создаем интерфейс масштабирования графика
    mainZoom = new QMainZoomSvc();
    connect(mainZoom,SIGNAL(xAxisClicked(double,bool)),SIGNAL(updateTrackingCursor(double,bool)));
    mainZoom->attach(this);

    // создаем интерфейс перемещенния графика
    dragZoom = new QDragZoomSvc();
    dragZoom->attach(this);

    wheelZoom = new QWheelZoomSvc();
    wheelZoom->attach(this);

    axisZoom = new QAxisZoomSvc();
    connect(axisZoom,SIGNAL(xAxisClicked(double,bool)),SIGNAL(updateTrackingCursor(double,bool)));
    connect(axisZoom,SIGNAL(contextMenuRequested(QPoint,int)),SIGNAL(contextMenuRequested(QPoint,int)));
    axisZoom->attach(this);
}

// Деструктор
ChartZoom::~ChartZoom()
{DD;
    // удаляем интерфейс перемещенния графика
    delete dragZoom;
    // удаляем интерфейс масштабирования графика
    delete mainZoom;
    delete axisZoom;
    delete wheelZoom;
    // удаляем контейнеры границ шкалы
    delete horizontalScaleBounds;    // горизонтальной
    delete verticalScaleBounds;    // и вертикальной
    delete verticalScaleBoundsSlave;
}

// Определение главного родителя
QObject *ChartZoom::generalParent(QObject *p)
{DD;
    QObject *generalParent_ = p;
    QObject *tp = generalParent_->parent();
    while (tp != 0)
    {
        generalParent_ = tp;
        tp = generalParent_->parent();
    }
    return generalParent_;
}

// Текущий режим масштабирования
ChartZoom::QConvType ChartZoom::regim()
{
    return convType;
}

// Переключение режима масштабирования
void ChartZoom::setRegime(ChartZoom::QConvType ct) {DD;
    convType = ct;
}

// указатель на опекаемый компонент QwtPlot
QwtPlot *ChartZoom::plot() {
    return qwtPlot;
}

// Основная горизонтальная шкала
QwtPlot::Axis ChartZoom::masterH() const {DD;
    return masterX;
}

// Дополнительная горизонтальная шкала
QwtPlot::Axis ChartZoom::slaveH() const {DD;
    return slaveX;
}

// Основная вертикальная шкала
QwtPlot::Axis ChartZoom::masterV() const {DD;
    return masterY;
}

// Дополнительная вертикальная шкала
QwtPlot::Axis ChartZoom::slaveV() const {DD;
    return slaveY;
}

// Обновление графика
void ChartZoom::updatePlot()
{DD;
    qwtPlot->replot();
}

// Обработчик всех событий
bool ChartZoom::eventFilter(QObject *target,QEvent *event)
{
    // если событие произошло для главного окна,
    if (target == mwin)
        // если окно было отображено на экране, или изменились его размеры, то
        if (event->type() == QEvent::Show ||
            event->type() == QEvent::Resize)
            updatePlot();   // обновляем график
    // если событие произошло для графика, то
    if (target == qwtPlot)
        // если изменились размеры графика, то
        if (event->type() == QEvent::Resize)
            updatePlot();   // обновляем график
    // передаем управление стандартному обработчику событий
    return QObject::eventFilter(target,event);
}

void ChartZoom::setZoomEnabled(bool enabled)
{DD;
    this->activated = enabled;
}

void ChartZoom::addZoom(const ChartZoom::zoomCoordinates &coords, bool apply)
{DD;
    zoomStack.push(coords);
    if (apply) {
        if (coords.coords.contains(masterH()))
            horizontalScaleBounds->set(coords.coords.value(masterH()).x(),
                                       coords.coords.value(masterH()).y());
        if (coords.coords.contains(masterV())) {
            verticalScaleBounds->set(coords.coords.value(masterV()).x(),
                                     coords.coords.value(masterV()).y());
        }
        if (coords.coords.contains(slaveV())) {
            verticalScaleBoundsSlave->set(coords.coords.value(slaveV()).x(),
                                          coords.coords.value(slaveV()).y());
        }
        plot()->replot();
    }
}

void ChartZoom::zoomBack()
{DD;
    if (zoomStack.isEmpty()) return;
    zoomStack.pop();
    if (zoomStack.isEmpty()) {
        // nothing to zoom back to, autoscaling to
        horizontalScaleBounds->autoscale();
        verticalScaleBounds->autoscale();
        verticalScaleBoundsSlave->autoscale();
    }
    else {
        zoomCoordinates coords = zoomStack.top();
        if (coords.coords.contains(masterH()))
            horizontalScaleBounds->set(coords.coords.value(masterH()).x(),
                                       coords.coords.value(masterH()).y());
        if (coords.coords.contains(masterV())) {
            verticalScaleBounds->set(coords.coords.value(masterV()).x(),
                                     coords.coords.value(masterV()).y());
        }
        if (coords.coords.contains(slaveV())) {
            verticalScaleBoundsSlave->set(coords.coords.value(slaveV()).x(),
                                          coords.coords.value(slaveV()).y());
        }
    }
    // перестраиваем график
    plot()->replot();
}

void ChartZoom::labelSelected(bool selected)
{DD;
    setZoomEnabled(!selected);
}

    /**************************************************/
    /*         Реализация класса QScaleBounds         */
    /*                  Версия 1.0.1                  */
    /**************************************************/

// Конструктор
ChartZoom::ScaleBounds::
    ScaleBounds(QwtPlot *plt,QwtPlot::Axis mst)
{DD;
    // запоминаем
    plot = plt;     // опекаемый график
    axis = mst;   // шкалу
    fixed = false;  // границы еще не фиксированы
    min = 0.0;
    max = 10.0;

    if (plot)
        plot->setAxisScale(axis, min, max);
}

void ChartZoom::ScaleBounds::setFixed(bool fixed)
{
    this->fixed = fixed;

    if (!fixed)
        autoscale();
}

// Фиксация исходных границ шкалы
void ChartZoom::ScaleBounds::add(double min, double max)
{DD;
    if (min==max) {
        mins << (min-1.0);
        maxes << (max+1.0);
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
    plot->setAxisScale(axis, min,max);
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
    auto iter = std::min_element(mins.begin(), mins.end());
    double minn = iter==mins.end() ? this->min : *iter;
    iter = std::max_element(maxes.begin(), maxes.end());
    double maxx = iter==maxes.end() ? this->max : *iter;

    plot->setAxisScale(axis, minn, maxx);
}

void ChartZoom::ScaleBounds::removeToAutoscale(double min, double max)
{DD;
    if (min==max) {
        mins.removeOne(min-1.0);
        maxes.removeOne(max+1.0);
    }
    else {
        mins.removeOne(min);
        maxes.removeOne(max);
    }
}

void ChartZoom::ScaleBounds::back()
{DD;
    mins.removeLast();
    maxes.removeLast();
    double minn = mins.isEmpty() ? min : mins.last();
    double maxx = maxes.isEmpty()? max : maxes.last();
    set(minn, maxx);
}



