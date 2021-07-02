#include "dragzoom.h"

#include "logging.h"
#include "chartzoom.h"
#include "plot.h"

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

DragZoom::DragZoom() : QObject()
{DD;
}

void DragZoom::attach(ChartZoom *zm)
{DD;
    zoom = zm;
    zm->plot()->installEventFilter(this);
}

bool DragZoom::eventFilter(QObject *target,QEvent *event)
{
    if (target == zoom->plot()) {
        if (event->type() == QEvent::MouseButtonPress ||
            event->type() == QEvent::MouseMove ||
            event->type() == QEvent::MouseButtonRelease)
            dragMouseEvent(event);
    }
    return QObject::eventFilter(target,event);
}

void DragZoom::applyDrag(QPoint mousePos, bool moveRightAxis)
{DD;
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
    dx = -(mousePos.x() - cg.x() - horizontalCursorPosition) * horizontalFactor;
    // устанавливаем новые левую и правую границы шкалы для горизонтальной оси
    //     новые границы = начальные границы + смещение
    zoom->horizontalScaleBounds->set(minHorizontalBound + dx, maxHorizontalBound + dx);
    // аналогично определяем dy - смещение границ по вертикальной оси
    if (moveRightAxis) {
        dy1 = -(mousePos.y() - cg.y() - verticalCursorPosition) * verticalFactor1;
        zoom->verticalScaleBoundsSlave->set(minVerticalBound1 + dy1, maxVerticalBound1 + dy1);
    }
    else {
        dy = -(mousePos.y() - cg.y() - verticalCursorPosition) * verticalFactor;
        zoom->verticalScaleBounds->set(minVerticalBound + dy, maxVerticalBound + dy);
    }
    zoom->plot()->replot();
}

void DragZoom::dragMouseEvent(QEvent *event)
{DD;
    QMouseEvent *mEvent = static_cast<QMouseEvent *>(event);

    switch (event->type())  {
        case QEvent::MouseButtonPress:
            if (mEvent->button() == Qt::RightButton)
                startDrag(mEvent);
            break;
        case QEvent::MouseMove: proceedDrag(mEvent); break;
        case QEvent::MouseButtonRelease: endDrag(mEvent); break;
        default: break;
    }
}

void DragZoom::startDrag(QMouseEvent *mEvent)
{DD;
    if (zoom->regime() == ChartZoom::ctNone) {
        QwtPlot *plot = zoom->plot();
        QWidget *canvas = plot->canvas();
        QRect cg = canvas->geometry();

        horizontalCursorPosition = mEvent->pos().x() - cg.x();
        verticalCursorPosition = mEvent->pos().y() - cg.y();
        // если курсор находится над канвой графика
        if (horizontalCursorPosition >= 0 && horizontalCursorPosition < cg.width()
            && verticalCursorPosition >= 0 && verticalCursorPosition < cg.height()) {
            zoom->setRegime(ChartZoom::ctDrag);

            tCursor = canvas->cursor();
            canvas->setCursor(Qt::OpenHandCursor);
            // определяем текущий масштабирующий множитель по горизонтальной оси
            // (т.е. узнаем на сколько изменяется координата по шкале x
            // при перемещении курсора вправо на один пиксел)
            horizontalFactor = plot->invTransform(QwtAxis::xBottom, horizontalCursorPosition + 1) -
                               plot->invTransform(QwtAxis::xBottom,horizontalCursorPosition);

            QwtScaleMap sm = plot->canvasMap(QwtAxis::xBottom);
            minHorizontalBound = sm.s1();
            maxHorizontalBound = sm.s2();

            if (mEvent->modifiers() & Qt::ControlModifier) {
                QwtAxisId mY(QwtAxis::yRight);
                verticalFactor1 = plot->invTransform(mY,verticalCursorPosition + 1) -
                                  plot->invTransform(mY,verticalCursorPosition);
                sm = plot->canvasMap(mY);
                minVerticalBound1 = sm.s1();
                maxVerticalBound1 = sm.s2();
            }
            else {
                QwtAxisId mY(QwtAxis::yLeft);
                verticalFactor = plot->invTransform(mY,verticalCursorPosition + 1) -
                                 plot->invTransform(mY,verticalCursorPosition);
                sm = plot->canvasMap(mY);
                minVerticalBound = sm.s1();
                maxVerticalBound = sm.s2();
            }
        }
    }
}


void DragZoom::proceedDrag(QMouseEvent *mEvent)
{DD;
    if (zoom->regime() == ChartZoom::ctDrag) {
        zoom->plot()->canvas()->setCursor(Qt::ClosedHandCursor);
        applyDrag(mEvent->pos(), mEvent->modifiers() & Qt::ControlModifier);
    }
}

void DragZoom::endDrag(QMouseEvent *mEvent)
{DD;
    Q_UNUSED(mEvent);

    if (zoom->regime() == ChartZoom::ctDrag) {
        zoom->plot()->canvas()->setCursor(tCursor);
        zoom->setRegime(ChartZoom::ctNone);

        ChartZoom::zoomCoordinates coords;
        if (!qFuzzyIsNull(dx)) {
            coords.coords.insert(QwtAxis::xBottom, {minHorizontalBound + dx, maxHorizontalBound + dx});
        }
        if (!qFuzzyIsNull(dy)) {
            coords.coords.insert(QwtAxis::yLeft, {minVerticalBound + dy, maxVerticalBound + dy});
        }
        if (!qFuzzyIsNull(dy1)) {
            coords.coords.insert(QwtAxis::yRight, {minVerticalBound1 + dy1, maxVerticalBound1 + dy1});
        }
        zoom->addZoom(coords, true);
    }
}
