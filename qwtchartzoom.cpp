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

#include "qwtchartzoom.h"
#include "qmainzoomsvc.h"
#include "qwheelzoomsvc.h"
#include "qaxiszoomsvc.h"
#include <qwt_scale_widget.h>
#include "logging.h"

#include <QKeyEvent>

// Конструктор
QwtChartZoom::QwtChartZoom(QwtPlot *qp) :
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
    horizontalScaleBounds = new ScaleBounds(qp,masterX,slaveX);    // горизонтальной
    verticalScaleBounds = new ScaleBounds(qp,masterY,slaveY);    // и вертикальной

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
QwtChartZoom::~QwtChartZoom()
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
QObject *QwtChartZoom::generalParent(QObject *p)
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
QwtChartZoom::QConvType QwtChartZoom::regim()
{DD;
    return convType;
}

// Переключение режима масштабирования
void QwtChartZoom::setRegime(QwtChartZoom::QConvType ct) {DD;
    convType = ct;
}

// указатель на опекаемый компонент QwtPlot
QwtPlot *QwtChartZoom::plot() {
    return qwtPlot;
}

// Основная горизонтальная шкала
QwtPlot::Axis QwtChartZoom::masterH() const {DD;
    return masterX;
}

// Дополнительная горизонтальная шкала
QwtPlot::Axis QwtChartZoom::slaveH() const {DD;
    return slaveX;
}

// Основная вертикальная шкала
QwtPlot::Axis QwtChartZoom::masterV() const {DD;
    return masterY;
}

// Дополнительная вертикальная шкала
QwtPlot::Axis QwtChartZoom::slaveV() const {DD;
    return slaveY;
}

// Фиксация текущих границ графика в качестве исходных
void QwtChartZoom::fixBoundaries() {DD;
    // здесь только сбрасывается флаг и тем самым
    // указывается на необходимость фиксировать границы
    isbF = false;
    // фактическая фиксация границ произойдет в момент начала
    // какого-либо преобразования при вызове fixBounds()
}

// Обновление графика
void QwtChartZoom::updatePlot()
{DD;
    qwtPlot->replot();
}

// Обработчик всех событий
bool QwtChartZoom::eventFilter(QObject *target,QEvent *event)
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
void QwtChartZoom::fixBounds()
{DD;
    // если этого еще не было сделано
    if (!isbF)
    {
        // фиксируем границы
        horizontalScaleBounds->fix();   // горизонтальные
        verticalScaleBounds->fix();   // и вертикальные
        // устанавливаем флажок фиксации границ графика
        isbF = true;
    }
}

// Восстановление исходных границ графика
void QwtChartZoom::resetBounds(Qt::Orientations orientations)
{DD;
    // устанавливаем запомненные ранее границы
    if (orientations & Qt::Horizontal) horizontalScaleBounds->reset();  // горизонтальной шкалы
    if (orientations & Qt::Vertical) verticalScaleBounds->reset();  // и вертикальной
    // перестраиваем график
    qwtPlot->replot();
}

void QwtChartZoom::setZoomEnabled(bool enabled)
{DD;
    this->activated = enabled;
}

void QwtChartZoom::labelSelected(bool selected)
{DD;
    setZoomEnabled(!selected);
}

    /**************************************************/
    /*         Реализация класса QScaleBounds         */
    /*                  Версия 1.0.1                  */
    /**************************************************/

// Конструктор
QwtChartZoom::ScaleBounds::
    ScaleBounds(QwtPlot *plt,QwtPlot::Axis mst,QwtPlot::Axis slv)
{DD;
    // запоминаем
    plot = plt;     // опекаемый график
    master = mst;   // основную шкалу
    slave = slv;    // и дополнительную
    fixed = false;  // границы еще не фиксированы
}

// Фиксация исходных границ шкалы
void QwtChartZoom::ScaleBounds::fix()
{DD;
    // получаем карту основной шкалы
    QwtScaleMap sm = plot->canvasMap(master);
    // и запоминаем текущие левую и правую границы шкалы
    min = sm.s1(); max = sm.s2();
    // получаем карту дополнительной горизонтальной шкалы
    sm = plot->canvasMap(slave);
    // строим преобразование основной шкалы в дополнительную в виде
    //     s = a * m + b, где:
    // если границы основной шкалы не совпадают, то
    if (min != max)
    {
        // a = (s2 - s1) / (m2 - m1)
        ak = (sm.s2() - sm.s1()) / (max - min);
        // b = (m2*s1 - m1*s2) / (m2 - m1)
        bk = (max * sm.s1() - min * sm.s2()) / (max - min);
    }
    else    // иначе (границы основной шкалы совпадают,
            // значит и дополнительной тоже)
    {
        // a = 0
        ak = 0;
        // b = s1
        bk = sm.s1();
    }
    fixed = true;   // границы фиксированы
}

// Установка заданных границ шкалы
void QwtChartZoom::ScaleBounds::set(double min, double max, int axis)
{DD;
    // если границы еще не фиксированы, фиксируем их
    if (!fixed) fix();
    // устанавливаем нижнюю и верхнюю границы шкалы
    plot->setAxisScale(axis == -1?master:axis, min,max);   // основной
}

// Восстановление исходных границ шкалы
void QwtChartZoom::ScaleBounds::reset()
{DD;
    // если границы уже фиксированы, то восстанавливаем исходные
    if (fixed) set(min,max,-1);
}

// Переустановка границ дополнительной шкалы
void QwtChartZoom::ScaleBounds::dup()
{DD;
    // если границы еще не фиксированы, фиксируем их
    if (!fixed) fix();
    // получаем карту основной шкалы
    QwtScaleMap sm = plot->canvasMap(master);
    // и устанавливаем границы для дополнительной
    plot->setAxisScale(slave,ak*sm.s1()+bk,ak*sm.s2()+bk);
}

/**********************************************************/
/*                                                        */
/*             Реализация класса QDragZoomSvc             */
/*                      Версия 1.0.1                      */
/*                                                        */
/* Разработал Мельников Сергей Андреевич,                 */
/* г. Каменск-Уральский Свердловской обл., 2012 г.,       */
/* при поддержке Ю. А. Роговского, г. Новосибирск.        */
/*                                                        */
/* Разрешается свободное использование и распространение. */
/* Упоминание автора обязательно.                         */
/*                                                        */
/**********************************************************/

// Конструктор
QDragZoomSvc::QDragZoomSvc() :
    QObject()
{DD;
}

// Прикрепление интерфейса к менеджеру масштабирования
void QDragZoomSvc::attach(QwtChartZoom *zm)
{DD;
    // запоминаем указатель на менеджер масштабирования
    zoom = zm;
    // назначаем для графика обработчик событий (фильтр событий)
    zm->plot()->installEventFilter(this);
}

// Обработчик всех событий
bool QDragZoomSvc::eventFilter(QObject *target,QEvent *event)
{
  //  if (zoom->activated)
        if (target == zoom->plot())
            if (event->type() == QEvent::MouseButtonPress ||
                event->type() == QEvent::MouseMove ||
                event->type() == QEvent::MouseButtonRelease)
                dragMouseEvent(event);
    return QObject::eventFilter(target,event);
}

// Применение результатов перемещения графика
void QDragZoomSvc::applyDrag(QPoint evpos, bool moveRightAxis)
{DD;
    // получаем указатель на график
    QwtPlot *plt = zoom->plot();
    // получаем геометрию канвы графика
    QRect cg = plt->canvas()->geometry();
    // scp_x - координата курсора в пикселах по горизонтальной оси
    //     в начальный момент времени (когда была нажата правая кнопка мыши)
    // evpos.x() - cg.x() - координата курсора
    //     в пикселах по горизонтальной оси в текущий момент времени
    // evpos.x() - cg.x() - scp_x - смещение курсора в пикселах
    //     по горизонтальной оси от начального положения
    // (evpos.x() - cg.x() - scp_x) * cs_kx -  это же смещение,
    //     но уже в единицах горизонтальной шкалы
    // dx - смещение границ по горизонтальной оси берется с обратным знаком
    //     (чтобы график относительно границ переместился вправо, сами границы следует сместить влево)
    double dx = -(evpos.x() - cg.x() - horizontalCursorPosition) * horizontalFactor;
    // устанавливаем новые левую и правую границы шкалы для горизонтальной оси
    //     новые границы = начальные границы + смещение
    zoom->horizontalScaleBounds->set(minHorizontalBound + dx,maxHorizontalBound + dx,-1);
    // аналогично определяем dy - смещение границ по вертикальной оси
    double dy = -(evpos.y() - cg.y() - verticalCursorPosition) * (moveRightAxis?verticalFactor1:verticalFactor);
    // устанавливаем новые нижнюю и верхнюю границы вертикальной шкалы
    if (moveRightAxis)
        zoom->verticalScaleBounds->set(minVerticalBound1 + dy,maxVerticalBound1 + dy,zoom->slaveV());
    else
        zoom->verticalScaleBounds->set(minVerticalBound + dy,maxVerticalBound + dy,zoom->masterV());
    // перестраиваем график (синхронно с остальными)
    plt->replot();
}

// Обработчик событий от мыши
void QDragZoomSvc::dragMouseEvent(QEvent *event)
{DD;
    // создаем указатель на событие от мыши
    QMouseEvent *mEvent = static_cast<QMouseEvent *>(event);
    // в зависимости от типа события вызываем соответствующий обработчик
    switch (event->type())
    {
        // нажата кнопка мыши
        case QEvent::MouseButtonPress: startDrag(mEvent); break;
            // перемещение мыши
        case QEvent::MouseMove: proceedDrag(mEvent); break;
            // отпущена кнопка мыши
        case QEvent::MouseButtonRelease: endDrag(mEvent); break;
            // для прочих событий ничего не делаем
        default: ;
    }
}

// Обработчик нажатия на кнопку мыши
// (включение перемещения графика)
void QDragZoomSvc::startDrag(QMouseEvent *mEvent)
{DD;
    // фиксируем исходные границы графика (если этого еще не было сделано)
    zoom->fixBounds();
    // если в данный момент еще не включен ни один из режимов
    if (zoom->regim() == QwtChartZoom::ctNone)
    {
        // получаем указатели на
        QwtPlot *plot = zoom->plot();        // график
        QWidget *canvas = plot->canvas();  // и канву
        // получаем геометрию канвы графика
        QRect cg = canvas->geometry();
        // определяем текущее положение курсора (относительно канвы графика)
        horizontalCursorPosition = mEvent->pos().x() - cg.x();
        verticalCursorPosition = mEvent->pos().y() - cg.y();
        // если курсор находится над канвой графика
        if (horizontalCursorPosition >= 0 && horizontalCursorPosition < cg.width() &&
            verticalCursorPosition >= 0 && verticalCursorPosition < cg.height())
            // если нажата правая кнопка мыши, то
            if (mEvent->button() == Qt::RightButton /*&& !(mEvent->modifiers() & Qt::ControlModifier)*/)
            {
                // прописываем соответствующий признак режима
                zoom->setRegime(QwtChartZoom::ctDrag);
                // запоминаем текущий курсор
                tCursor = canvas->cursor();
                // устанавливаем курсор OpenHand
                canvas->setCursor(Qt::OpenHandCursor);
                // определяем текущий масштабирующий множитель по горизонтальной оси
                // (т.е. узнаем на сколько изменяется координата по шкале x
                // при перемещении курсора вправо на один пиксел)
                horizontalFactor = plot->invTransform(zoom->masterH(),horizontalCursorPosition + 1) -
                    plot->invTransform(zoom->masterH(),horizontalCursorPosition);
                // получаем карту основной горизонтальной шкалы
                QwtScaleMap sm = plot->canvasMap(zoom->masterH());
                // для того чтобы фиксировать начальные левую и правую границы
                minHorizontalBound = sm.s1(); maxHorizontalBound = sm.s2();

                if (mEvent->modifiers() & Qt::ControlModifier) {
                    // получаем вертикальную шкалу
                    QwtPlot::Axis mY = zoom->slaveV();
                    // определяем текущий масштабирующий множитель по вертикальной оси
                    // (аналогично)
                    verticalFactor1 = plot->invTransform(mY,verticalCursorPosition + 1) -
                        plot->invTransform(mY,verticalCursorPosition);
                    // аналогично получаем карту основной вертикальной шкалы
                    sm = plot->canvasMap(mY);
                    // для того чтобы фиксировать начальные нижнюю и верхнюю границы
                    minVerticalBound1 = sm.s1(); maxVerticalBound1 = sm.s2();
                }
                else {
                    // получаем основную вертикальную шкалу
                    QwtPlot::Axis mY = zoom->masterV();
                    // определяем текущий масштабирующий множитель по вертикальной оси
                    // (аналогично)
                    verticalFactor = plot->invTransform(mY,verticalCursorPosition + 1) -
                        plot->invTransform(mY,verticalCursorPosition);
                    // аналогично получаем карту основной вертикальной шкалы
                    sm = plot->canvasMap(mY);
                    // для того чтобы фиксировать начальные нижнюю и верхнюю границы
                    minVerticalBound = sm.s1(); maxVerticalBound = sm.s2();
                }
            }
    }
}

// Обработчик перемещения мыши
// (выполнение перемещения или выбор нового положения графика)
void QDragZoomSvc::proceedDrag(QMouseEvent *mEvent)
{DD;
    // если включен режим перемещения графика, то
    if (zoom->regim() == QwtChartZoom::ctDrag)
    {
        // устанавливаем курсор ClosedHand
        zoom->plot()->canvas()->setCursor(Qt::ClosedHandCursor);
        // применяем результаты перемещения графика
        applyDrag(mEvent->pos(), mEvent->modifiers() & Qt::ControlModifier);
    }
}

// Обработчик отпускания кнопки мыши
// (выключение перемещения графика)
void QDragZoomSvc::endDrag(QMouseEvent *mEvent)
{DD;
    // если включен режим изменения масштаба или режим перемещения графика
    if (zoom->regim() == QwtChartZoom::ctDrag)
        // если отпущена правая кнопка мыши, то
        if (mEvent->button() == Qt::RightButton)
        {
            // восстанавливаем курсор
            zoom->plot()->canvas()->setCursor(tCursor);
            zoom->setRegime(QwtChartZoom::ctNone);  // и очищаем признак режима
        }
}
