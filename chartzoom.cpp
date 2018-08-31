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
    QObject(qp)
{DD;
    // получаем главное окно
    mwin = generalParent(qp);
    // и назначаем обработчик событий (фильтр событий)
    mwin->installEventFilter(this);

    // сбрасываем флаг для того, чтобы перед первым изменением масштаба
    // текущие границы графика были зафиксированы в качестве исходных
    isbF = false;
    // сбрасываем признак режима
    convType = ctNone;

    // получаем компонент QwtPlot, над которым будут производиться все преобразования
    qwtPlot = qp;
    // устанавливаем ему свойство, разрешающее обрабатывать события от клавиатуры
    qp->setFocusPolicy(Qt::StrongFocus);

    // назначаем основную и дополнительную шкалу
    masterX = QwtPlot::xBottom;
    slaveX = QwtPlot::xTop;

    masterY = QwtPlot::yLeft;
    slaveY = QwtPlot::yRight;

    // запоминаем количество делений на горизонтальной шкале
    mstHorDiv = qp->axisMaxMajor(masterX);
    slvHorDiv = qp->axisMaxMajor(slaveX);
    // запоминаем количество делений на вертикальной шкале
    mstVerDiv = qp->axisMaxMajor(masterY);
    slvVerDiv = qp->axisMaxMajor(slaveY);
    // создаем контейнеры границ шкалы
    horizontalScaleBounds = new ScaleBounds(qp,masterX);    // горизонтальной
    verticalScaleBounds = new ScaleBounds(qp,masterY);    // и вертикальной
    verticalScaleBoundsSlave = new ScaleBounds(qp,slaveY);

    // устанавливаем обработчик всех событий
    qwtPlot->installEventFilter(this);
    // для всех шкал графика
    for (int ax=0; ax < QwtPlot::axisCnt; ax++)
        // назначаем обработчик событий (фильтр событий)
        qwtPlot->axisWidget(ax)->installEventFilter(this);

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
}

// Определение главного родителя
QObject *ChartZoom::generalParent(QObject *p)
{DD;
    // берем в качестве предыдущего родителя график
    // (возможен и другой объект в аргументе функции)
    QObject *generalParent_ = p;
    // определяем родителя на текущем уровне
    QObject *tp = generalParent_->parent();
    // пока родитель на текущем уровне не NULL
    while (tp != NULL)
    {
        // понижаем уровень:
        // запоминаем в качестве предыдущего родителя текущий
        generalParent_ = tp;
        // определяем родителя на следующем уровне
        tp = generalParent_->parent();
    }
    // возвращаем в качестве главного родителя предыдущий
    return generalParent_;
}

// Текущий режим масштабирования
ChartZoom::QConvType ChartZoom::regim()
{DD;
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

// Фиксация текущих границ графика в качестве исходных
void ChartZoom::fixBoundaries() {DD;
    // здесь только сбрасывается флаг и тем самым
    // указывается на необходимость фиксировать границы
    isbF = false;
    // фактическая фиксация границ произойдет в момент начала
    // какого-либо преобразования при вызове fixBounds()
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

// Фактическая фиксация текущих границ графика
// в качестве исходных (если флаг isbF сброшен)
void ChartZoom::fixBounds()
{DD;
    // если этого еще не было сделано
//    if (!isbF)
//    {
//        // фиксируем границы
//        horizontalScaleBounds->fix();   // горизонтальные
//        verticalScaleBounds->fix();   // и вертикальные
//        // устанавливаем флажок фиксации границ графика
//        isbF = true;
//    }
}

// Восстановление исходных границ графика
void ChartZoom::resetBounds(Qt::Orientations orientations)
{DD;
    // устанавливаем запомненные ранее границы
    if (orientations & Qt::Horizontal) horizontalScaleBounds->autoscale();  // горизонтальной шкалы
    if (orientations & Qt::Vertical) {
        verticalScaleBounds->autoscale();  // и вертикальной
        verticalScaleBoundsSlave->autoscale();
    }
    // перестраиваем график
    qwtPlot->replot();
}

void ChartZoom::setZoomEnabled(bool enabled)
{DD;
    this->activated = enabled;
}

void ChartZoom::addZoom(const ChartZoom::zoomCoordinates &coords, bool apply)
{DD;
    zoomStack.push(coords);
    if (apply) {
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
        plot()->replot();
    }
}

void ChartZoom::zoomBack(Qt::Orientations orientations)
{DD;
    if (zoomStack.isEmpty()) return;
    zoomStack.pop();
    if (zoomStack.isEmpty()) {
        // nothing to zoom back to, autoscaling to
        if (orientations & Qt::Horizontal) horizontalScaleBounds->autoscale();
        if (orientations & Qt::Vertical) {
            verticalScaleBounds->autoscale();
            verticalScaleBoundsSlave->autoscale();
        }
    }
    else {
        zoomCoordinates coords = zoomStack.top();

        if ((orientations & Qt::Horizontal) && coords.coords.contains(masterH()))
            horizontalScaleBounds->set(coords.coords.value(masterH()).x(),
                                       coords.coords.value(masterH()).y());
        if ((orientations & Qt::Vertical)) {
            if (coords.coords.contains(masterV())) {
                verticalScaleBounds->set(coords.coords.value(masterV()).x(),
                                           coords.coords.value(masterV()).y());
            }
            if (coords.coords.contains(slaveV())) {
                verticalScaleBoundsSlave->set(coords.coords.value(slaveV()).x(),
                                           coords.coords.value(slaveV()).y());
            }
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
//    min = 0.0;
//    max = 10.0;
    mins << 0.0;
    maxes << 10.0;

    if (plot)
        plot->setAxisScale(axis, 0, 10.0);
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
    mins << min;
    maxes << max;

    if (!fixed) {
//        this->min = min;
//        this->max = max;
        plot->setAxisScale(axis, min,max);
    }
}

// Установка заданных границ шкалы
void ChartZoom::ScaleBounds::set(double min, double max)
{DD;
    // устанавливаем нижнюю и верхнюю границы шкалы
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
//        min = 0;
//        max = 10;
        mins.clear();
        maxes.clear();
        mins << 0.0;
        maxes << 10.0;
        set(0.0, 10.0);
    }
}

void ChartZoom::ScaleBounds::autoscale()
{DD;
    double min = *(std::min_element(mins.begin(), mins.end()));
    double max = *(std::max_element(maxes.begin(), maxes.end()));

    plot->setAxisScale(axis, min, max);
}

void ChartZoom::ScaleBounds::removeToAutoscale(double min, double max)
{DD;
    mins.removeAll(min);
    if (mins.isEmpty()) mins << min;

    maxes.removeAll(max);
    if (maxes.isEmpty()) maxes << max;
}

void ChartZoom::ScaleBounds::back()
{DD;
    if (mins.size()>1 && maxes.size()>1) {
        mins.removeLast();
        maxes.removeLast();
        set(mins.last(), maxes.last());
    }
    else {
        mins.clear();
        maxes.clear();
        mins << 0.0;
        maxes << 10.0;
        set(0.0, 10.0);
    }
}

//// Переустановка границ дополнительной шкалы
//void QwtChartZoom::ScaleBounds::dup()
//{DD;
//    // если границы еще не фиксированы, фиксируем их
//    if (!fixed) fix();
//    // получаем карту основной шкалы
//    QwtScaleMap sm = plot->canvasMap(master);
//    // и устанавливаем границы для дополнительной
//    plot->setAxisScale(slave,ak*sm.s1()+bk,ak*sm.s2()+bk);
//}


