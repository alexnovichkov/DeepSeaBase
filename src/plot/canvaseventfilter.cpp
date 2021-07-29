#include "canvaseventfilter.h"

#include "plot.h"
#include "zoomstack.h"
#include "dragzoom.h"
#include "wheelzoom.h"
#include "axiszoom.h"
#include "plotzoom.h"
#include "picker.h"
#include <QEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include "logging.h"
#include "qwt_scale_widget.h"

#include "axisboundsdialog.h"

CanvasEventFilter::CanvasEventFilter(Plot *parent) : QObject(parent), plot(parent)
{
    //разрешаем обрабатывать события от клавиатуры
    plot->setFocusPolicy(Qt::StrongFocus);
    plot->canvas()->installEventFilter(this);

    for (int ax = 0; ax < QwtAxis::PosCount; ax++) {
        for (int j = 0; j < plot->axesCount(ax); ++j) {
            plot->axisWidget(QwtAxisId(ax,j))->installEventFilter(this);
            plot->axisWidget(QwtAxisId(ax,j))->setFocusPolicy(Qt::StrongFocus);
        }
    }
}

bool CanvasEventFilter::eventFilter(QObject *target, QEvent *event)
{
    if (!enabled) return QObject::eventFilter(target, event);


    if (target == plot->canvas()) {
        switch (event->type()) {
            case QEvent::MouseButtonPress:
            case QEvent::MouseMove:
            case QEvent::MouseButtonRelease:
            case QEvent::MouseButtonDblClick:
                procMouseEvent(event);
                break;
            case QEvent::KeyPress:
                procKeyboardEvent(event);
                break;
            case QEvent::Wheel:
                procWheelEvent(QwtAxisId(-1,0), event);
                break;
            default: break;
        }
    }
    else {
        QwtAxisId axis(-1,0);
        for (int a = 0; a < QwtAxis::PosCount; a++) {
            for (int j = 0; j < plot->axesCount(a); ++j) {
                if (target == plot->axisWidget(QwtAxisId(a,j))) {
                    axis = QwtAxisId(a,j);
                    break;
                }
            }
        }
        if (axis.id != -1) {
            switch (event->type()) {
                case QEvent::Wheel:
                    procWheelEvent(axis, event);
                    break;
                case QEvent::MouseButtonPress:
                case QEvent::MouseMove:
                case QEvent::MouseButtonRelease:
                case QEvent::MouseButtonDblClick:
                case QEvent::Leave:
                    procAxisEvent(axis,event);
                    break;
                case QEvent::KeyPress:
                    procKeyboardEvent(event);
                    break;
                default:
                    break;
            }
        }
    }

    return QObject::eventFilter(target, event);
}

void CanvasEventFilter::procMouseEvent(QEvent *event)
{
    //Нажатия левой кнопки - масштабирование графика, выбор объектов или сброс выбора
    //Нажатия правой кнопки - сдвиг графика

    QMouseEvent *mEvent = static_cast<QMouseEvent *>(event);

    switch (mEvent->type())  {
        case QEvent::MouseButtonPress:
            mousePress(mEvent);
            break;
        case QEvent::MouseMove:
            mouseMove(mEvent);
            break;
        case QEvent::MouseButtonRelease:
            mouseRelease(mEvent);
            break;
        default: ;
    }
}

void CanvasEventFilter::procKeyboardEvent(QEvent *event)
{
    QKeyEvent *kEvent = dynamic_cast<QKeyEvent*>(event);
    switch (kEvent->key()) {
        case Qt::Key_Backspace: {
            zoomStack->zoomBack();
            break;
        }
        case Qt::Key_Escape: {//прерывание выделения
            if (actionType == ActionType::Zoom) {
                plotZoom->stopZoom();
                actionType = ActionType::None;
            }
            break;
        }
        case Qt::Key_Left: {
            emit moveCursor(Enums::Left);
            break;
        }
        case Qt::Key_Right: {
            emit moveCursor(Enums::Right);
            break;
        }
        case Qt::Key_Up: {
            emit moveCursor(Enums::Up);
            break;
        }
        case Qt::Key_Down: {
            emit moveCursor(Enums::Down);
            break;
        }
        case Qt::Key_H: {
            plot->switchLabelsVisibility();
            break;
        }
        default: break;
    }
    if (picker) picker->procKeyboardEvent(kEvent->key());
}

void CanvasEventFilter::procWheelEvent(QwtAxisId axis, QEvent *event)
{
    auto coords = wheelZoom->applyWheel(event, axis);
    zoomStack->addZoom(coords, false);
}

void CanvasEventFilter::procAxisEvent(QwtAxisId axis, QEvent *event)
{
    QMouseEvent *mEvent = static_cast<QMouseEvent *>(event);
    switch (event->type()) {
        case QEvent::Leave: {
            emit hover(axis,0);
            actionType = ActionType::None;
            break;
        }
        case QEvent::MouseButtonPress: {
            if (mEvent->button()==Qt::RightButton) {
                emit contextMenuRequested(mEvent->globalPos(), axis);
                actionType = ActionType::None;
            }
            else if (mEvent->button()==Qt::LeftButton) {
                if (axis.isXAxis())
                    axisZoom->startHorizontalAxisZoom(mEvent, axis);
                else
                    axisZoom->startVerticalAxisZoom(mEvent, axis);
                actionType = ActionType::Axis;
            }
            break;
        }
        case QEvent::MouseMove: {
            auto coords = axisZoom->proceedAxisZoom(mEvent, axis);
            zoomStack->addZoom(coords, false);
            break;
        }
        case QEvent::MouseButtonRelease: {
            auto coords = axisZoom->endAxisZoom(mEvent, axis);
            zoomStack->addZoom(coords, true);
            actionType = ActionType::None;
            break;
        }
        case QEvent::MouseButtonDblClick: {
            if (mEvent->button()==Qt::LeftButton) {
                AxisBoundsDialog dialog(plot->canvasMap(axis).s1(), plot->canvasMap(axis).s2(), axis);
                if (dialog.exec()) {
                    ZoomStack::zoomCoordinates coords;
                    if (axis == QwtAxis::xBottom || axis == QwtPlot::xTop)
                        coords.coords.insert(QwtAxis::xBottom, {dialog.leftBorder(), dialog.rightBorder()});
                    else if (zoomStack->verticalScaleBounds->axis == axis.pos ||
                             zoomStack->verticalScaleBoundsSlave->axis == axis.pos)
                        coords.coords.insert(axis.pos, {dialog.leftBorder(), dialog.rightBorder()});
                    zoomStack->addZoom(coords, true);

//                    if (dialog.autoscale()) {
//                        emit needsAutoscale(axis);
//                    }
//                    zoom->plot()->replot();
                }
            }
            break;
        }

        default: break;
    }
}

void CanvasEventFilter::mousePress(QMouseEvent *event)
{
    currentPosition = event->pos();

    switch (event->button()) {
        case Qt::RightButton: {
            actionType = ActionType::Drag;
            dragZoom->startDrag(event);
            break;
        }
        case Qt::LeftButton: {
            //либо выделение графика прямоугольником
            //либо выбор объекта
            //определяем, есть ли что под курсором
            //если есть, то это выбор объекта
            //если нет, то сбрасываем выбор объекта и это - выделение прямоугольником
            bool selected = picker ? picker->findObject(event) : false;
            if (selected) {
                actionType = ActionType::Pick;
            }
            else {
                actionType = ActionType::Zoom;
                plotZoom->startZoom(event);
            }
            break;
        }
        default: break;
    }
}

void CanvasEventFilter::mouseMove(QMouseEvent *event)
{
    if (actionType == ActionType::Drag) {
        auto coords = dragZoom->proceedDrag(event);
        zoomStack->addZoom(coords, false);
    }
    else if (actionType == ActionType::Zoom) {
        plotZoom->proceedZoom(event);
    }
    else if (actionType == ActionType::Pick) {
       picker->proceedPick(event);
    }
}

void CanvasEventFilter::mouseRelease(QMouseEvent *event)
{DD;
    if (actionType == ActionType::Drag) {
        auto coords = dragZoom->endDrag(event);
        zoomStack->addZoom(coords, true);
        actionType = ActionType::None;
    }
    else if (actionType == ActionType::Zoom) {
        auto coords = plotZoom->endZoom(event);
        if (coords.coords.isEmpty())
            picker->endPick(event);
        else
            zoomStack->addZoom(coords, true);
        actionType = ActionType::None;
    }
    else {
        picker->endPick(event);
        actionType = ActionType::None;
    }
}

