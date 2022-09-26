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
#include "selectable.h"

CanvasEventFilter::CanvasEventFilter(Plot *parent) : QObject(parent), plot(parent)
{DDD;
    //разрешаем обрабатывать события от клавиатуры
    plot->setFocusPolicy(Qt::StrongFocus);
    plot->canvas()->installEventFilter(this);

    for (int ax = 0; ax < QwtAxis::AxisPositions; ax++) {
        plot->axisWidget(ax)->installEventFilter(this);
        plot->axisWidget(ax)->setFocusPolicy(Qt::StrongFocus);
    }
}

bool CanvasEventFilter::eventFilter(QObject *target, QEvent *event)
{DDD;
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
                procWheelEvent(Enums::AxisType::atInvalid, event);
                break;
            default: break;
        }
    }
    else {
        Enums::AxisType axis = Enums::AxisType::atInvalid;
        for (int a = 0; a < QwtAxis::AxisPositions; a++) {
            if (target == plot->axisWidget(a)) {
                axis = toAxisType(a);
                break;
            }
        }
        if (axis != Enums::AxisType::atInvalid) {
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
{DDD;
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
        case QEvent::MouseButtonDblClick:
            mouseDoubleClick(mEvent);
            break;
        default: ;
    }
}

void CanvasEventFilter::procKeyboardEvent(QEvent *event)
{DDD;
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
//        case Qt::Key_Left: {
//            emit moveCursor(Enums::Left);
//            break;
//        }
//        case Qt::Key_Right: {
//            emit moveCursor(Enums::Right);
//            break;
//        }
//        case Qt::Key_Up: {
//            emit moveCursor(Enums::Up);
//            break;
//        }
//        case Qt::Key_Down: {
//            emit moveCursor(Enums::Down);
//            break;
//        }
        case Qt::Key_H: {
            plot->switchLabelsVisibility();
            break;
        }
        default: break;
    }
    if (picker) picker->procKeyboardEvent(kEvent->key());
}

void CanvasEventFilter::procWheelEvent(Enums::AxisType axis, QEvent *event)
{DDD;
    auto coords = wheelZoom->applyWheel(event, axis);
    zoomStack->addZoom(coords, false);
}

void CanvasEventFilter::procAxisEvent(Enums::AxisType axis, QEvent *event)
{DDD;
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
                if (axis == Enums::AxisType::atTop || axis == Enums::AxisType::atBottom)
                    axisZoom->startHorizontalAxisZoom(mEvent, axis);
                else
                    axisZoom->startVerticalAxisZoom(mEvent, axis);
                actionType = ActionType::Axis;
            }
            break;
        }
        case QEvent::MouseMove: {
            auto coords = axisZoom->proceedAxisZoom(mEvent, axis);
            if (!coords.coords.isEmpty()) zoomStack->addZoom(coords, false);
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
                auto range = plot->plotRange(axis);
                AxisBoundsDialog dialog(range.min, range.max, axis);
                if (dialog.exec()) {
                    ZoomStack::zoomCoordinates coords;
                    if (axis == Enums::AxisType::atBottom || axis == Enums::AxisType::atTop)
                        coords.coords.insert(Enums::AxisType::atBottom, {dialog.leftBorder(), dialog.rightBorder()});
                    else if (zoomStack->verticalScaleBounds->axis == axis ||
                             zoomStack->verticalScaleBoundsSlave->axis == axis)
                        coords.coords.insert(axis, {dialog.leftBorder(), dialog.rightBorder()});
                    zoomStack->addZoom(coords, true);

                }
            }
            break;
        }

        default: break;
    }
}

void CanvasEventFilter::mousePress(QMouseEvent *event)
{DDD;
    currentPosition = event->pos();

    switch (event->button()) {
        case Qt::RightButton: {
            actionType = ActionType::Drag;
            dragZoom->startDrag(event);
            break;
        }
        case Qt::LeftButton: {
            if (picker && picker->pickPriority() == Picker::PickPriority::PickFirst) {
                auto selected = picker->findObject(event);

                if (selected.object) {
                    actionType = ActionType::Pick;
                    picker->startPick(event->pos(), selected);
                }
                else {
                    if (picker) picker->deselect();
                    actionType = ActionType::Zoom;
                    plotZoom->startZoom(event);
                }
            }
            else {
                auto selected = picker->findObject(event);

                if (picker->alreadySelected(selected) ||
                    (selected.object && selected.object->draggable())) {
                    actionType = ActionType::Pick;
                    picker->startPick(event->pos(), selected);
                }
                else {
                   // if (picker) picker->deselect();
                    actionType = ActionType::Zoom;
                    plotZoom->startZoom(event);
                }
            }

            break;
        }
        default: break;
    }
}

void CanvasEventFilter::mouseMove(QMouseEvent *event)
{DDD;
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
{DDD;
    if (actionType == ActionType::Drag) {
        auto coords = dragZoom->endDrag(event);
        zoomStack->addZoom(coords, true);
        actionType = ActionType::None;
        if (currentPosition == event->pos()) {
            if (auto s = picker->findObject(event); s.object) {
                picker->showContextMenu(event);
            }
        }
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

void CanvasEventFilter::mouseDoubleClick(QMouseEvent *event)
{DDD;
    switch (event->button()) {
        case Qt::LeftButton: {
            emit canvasDoubleClicked(currentPosition);
            break;
        }
        default: break;
    }
}


QDebug operator<<(QDebug debug, const CanvasEventFilter::ActionType &c)
{
    QDebugStateSaver saver(debug);
    switch (c) {
        case CanvasEventFilter::ActionType::Axis: debug << "axis"; break;
        case CanvasEventFilter::ActionType::Drag: debug << "drag"; break;
        case CanvasEventFilter::ActionType::None: debug << "none"; break;
        case CanvasEventFilter::ActionType::Pick: debug << "pick"; break;
        case CanvasEventFilter::ActionType::Zoom: debug << "zoom"; break;
    }

    return debug;
}
