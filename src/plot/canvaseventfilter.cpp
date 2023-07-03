#include "canvaseventfilter.h"

#include "zoomstack.h"
#include "picker.h"
#include <QEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include "logging.h"
#include "plot.h"
#include "axisboundsdialog.h"
#include "selectable.h"
#include "qcpplot.h"

CanvasEventFilter::CanvasEventFilter(Plot *parent) : QObject(parent), plot(parent)
{DD;

}

bool CanvasEventFilter::eventFilter(QObject *target, QEvent *event)
{DD;
    if (!enabled) return QObject::eventFilter(target, event);

    if (QMouseEvent * mEvent = dynamic_cast<QMouseEvent *>(event))
        procMouseEvent(mEvent);
    else if (event->type() == QEvent::KeyPress)
        procKeyboardEvent(event);
    else if (event->type() == QEvent::Wheel) {
        auto axis = plot->impl()->eventTargetAxis(event);
        if (axis) axis->wheelEvent(dynamic_cast<QWheelEvent*>(event));
    }

    return QObject::eventFilter(target, event);
}

void CanvasEventFilter::procMouseEvent(QMouseEvent *mEvent)
{DD;
    //Нажатия левой кнопки - масштабирование графика, выбор объектов или сброс выбора
    //Нажатия правой кнопки - сдвиг графика

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
        case QEvent::Leave:
            mouseLeave(mEvent);
            break;
        default: ;
    }
}

void CanvasEventFilter::mousePress(QMouseEvent *event)
{DD;
    startPosition = event->pos();

    auto axis = plot->impl()->eventTargetAxis(event);

    auto selected = picker->findObject(event);
    const bool maybePick = picker->alreadySelected(selected) || (selected.object && selected.object->draggable());

    //we need to prefer picking over axis if selected overlaps with the axis
    if (maybePick && event->button() == Qt::LeftButton) {
        //LOG(DEBUG) << "Start picking object"<<selected.object;
        actionType = ActionType::Pick;
        picker->startPick(event->pos(), selected);
        return;
    }

    if (axis) {
        //LOG(DEBUG) << "Start interacting with the axis"<<axis->axisType();
        currentAxis = axis;
        if (event->button()==Qt::RightButton) {
            emit contextMenuRequested(event->globalPos(), plot->impl()->axisType(axis));
            currentAxis = nullptr;
        }
        else axis->mousePressEvent(event, QVariant());
    }
    else {
        switch (event->button()) {
            case Qt::RightButton: {
                //LOG(DEBUG) << "Start dragging";
                actionType = ActionType::Drag;
                plot->impl()->axisRect()->mousePressEvent(event, QVariant());
                break;
            }
            case Qt::LeftButton: {
                if (maybePick) {
                    //LOG(DEBUG) << "Start picking object"<<selected.object;
                    actionType = ActionType::Pick;
                    picker->startPick(event->pos(), selected);
                }
                else {
                    actionType = ActionType::Zoom;
                    //LOG(DEBUG) << "Start zooming canvas";
                    plot->impl()->startZoom(event);
                }

                break;
            }
            default: break;
        }
    }
}

void CanvasEventFilter::mouseMove(QMouseEvent *event)
{DD;
    if (currentAxis) {
        //LOG(DEBUG) << "Proceed moving axis";
        currentAxis->mouseMoveEvent(event, startPosition);
    }

    else if (actionType == ActionType::Drag) {
        //LOG(DEBUG) << "Proceed dragging";
        plot->impl()->axisRect()->mouseMoveEvent(event, startPosition);
    }
    else if (actionType == ActionType::Zoom) {
        plot->impl()->proceedZoom(event);
        //LOG(DEBUG) << "Proceed zooming";
    }
    else if (actionType == ActionType::Pick) {
        //LOG(DEBUG) << "Proceed picking";
        picker->proceedPick(event);
    }
}

void CanvasEventFilter::mouseRelease(QMouseEvent *event)
{DD;
    if (currentAxis) {
        //LOG(DEBUG) << "Stop moving axis";
        currentAxis->mouseReleaseEvent(event, startPosition);
        currentAxis = nullptr;
    }
    else if (actionType == ActionType::Drag) {
        //LOG(DEBUG) << "Stop dragging";
        plot->impl()->axisRect()->mouseReleaseEvent(event, startPosition);

        actionType = ActionType::None;
        if (startPosition == event->pos()) {
            if (auto s = picker->findObject(event); s.object) {
                picker->showContextMenu(event);
            }
        }
    }
    else if (actionType == ActionType::Zoom) {
        //LOG(DEBUG) << "Stop zooming";
        if ((startPosition - event->pos()).manhattanLength() <= 3) {
            //no actual mouse move
            plot->impl()->cancelZoom();
            picker->endPick(event);
        }
        else
            plot->impl()->endZoom(event);
        actionType = ActionType::None;
    }
    else if (actionType == ActionType::Pick) {
        //LOG(DEBUG) << "End picking";
        picker->endPick(event);
        actionType = ActionType::None;
    }
}

void CanvasEventFilter::mouseDoubleClick(QMouseEvent *event)
{DD;
    auto axis = plot->impl()->eventTargetAxis(event);
    if (axis) {
        emit axisDoubleClicked(axis);
    }
    else switch (event->button()) {
        case Qt::LeftButton: {
            //перенаправляется в Plot и перемещает курсор проигрывателя
            emit canvasDoubleClicked(startPosition);
            break;
        }
        default: break;
    }
}

void CanvasEventFilter::mouseLeave(QMouseEvent *event)
{
    Q_UNUSED(event);
    if (currentAxis) {
        emit hover(fromQcpAxis(currentAxis->axisType()), 0);
        actionType = ActionType::None;
    }
}

void CanvasEventFilter::procKeyboardEvent(QEvent *event)
{DD;
    auto kEvent = dynamic_cast<QKeyEvent*>(event);
    switch (kEvent->key()) {
        case Qt::Key_Backspace: {
            zoomStack->zoomBack();
            break;
        }
        case Qt::Key_Escape: {//прерывание выделения
            if (actionType == ActionType::Zoom) {
                plot->impl()->cancelZoom();
                actionType = ActionType::None;
            }
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
