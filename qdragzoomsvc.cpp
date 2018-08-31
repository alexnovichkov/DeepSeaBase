#include "qdragzoomsvc.h"

#include "logging.h"
#include "chartzoom.h"

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
void QDragZoomSvc::attach(ChartZoom *zm)
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
    // получаем геометрию канвы графика
    QRect cg = zoom->plot()->canvas()->geometry();
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
    dx = -(evpos.x() - cg.x() - horizontalCursorPosition) * horizontalFactor;
    // устанавливаем новые левую и правую границы шкалы для горизонтальной оси
    //     новые границы = начальные границы + смещение
    zoom->horizontalScaleBounds->set(minHorizontalBound + dx, maxHorizontalBound + dx);
    // аналогично определяем dy - смещение границ по вертикальной оси
    if (moveRightAxis) {
        dy1 = -(evpos.y() - cg.y() - verticalCursorPosition) * verticalFactor1;
        zoom->verticalScaleBoundsSlave->set(minVerticalBound1 + dy1, maxVerticalBound1 + dy1);
    }
    else {
        dy = -(evpos.y() - cg.y() - verticalCursorPosition) * verticalFactor;
        zoom->verticalScaleBounds->set(minVerticalBound + dy, maxVerticalBound + dy);
    }

    // перестраиваем график (синхронно с остальными)
    zoom->plot()->replot();
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
    // если в данный момент еще не включен ни один из режимов
    if (zoom->regim() == ChartZoom::ctNone)
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
                zoom->setRegime(ChartZoom::ctDrag);
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
    if (zoom->regim() == ChartZoom::ctDrag)
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
    if (zoom->regim() == ChartZoom::ctDrag)
        // если отпущена правая кнопка мыши, то
        if (mEvent->button() == Qt::RightButton)
        {
            // восстанавливаем курсор
            zoom->plot()->canvas()->setCursor(tCursor);
            zoom->setRegime(ChartZoom::ctNone);  // и очищаем признак режима

            // запоминаем совершенное перемещение
            ChartZoom::zoomCoordinates coords;
            if (!qFuzzyIsNull(dx)) {
                coords.coords.insert(zoom->masterH(), {minHorizontalBound + dx, maxHorizontalBound + dx});
            }
            if (!qFuzzyIsNull(dy)) {
                coords.coords.insert(zoom->masterV(), {minVerticalBound + dy, maxVerticalBound + dy});
            }
            if (!qFuzzyIsNull(dy1)) {
                coords.coords.insert(zoom->slaveV(), {minVerticalBound1 + dy1, maxVerticalBound1 + dy1});
            }
            if (!coords.coords.isEmpty()) zoom->addZoom(coords);
        }
}
