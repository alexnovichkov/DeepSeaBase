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
#include "qcustomplot/qcpplot.h"

CanvasEventFilter::CanvasEventFilter(Plot *parent) : QObject(parent), plot(parent)
{DDD;

}

bool CanvasEventFilter::eventFilter(QObject *target, QEvent *event)
{DD;
    if (!enabled) return QObject::eventFilter(target, event);

    auto targetAxis = plot->impl()->eventTargetAxis(event, target);
    auto qcpAxis = dynamic_cast<QCPAxis*>(targetAxis);

    if (!qcpAxis) {
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
//                procWheelEvent(Enums::AxisType::atInvalid, event);
                break;
            default: break;
        }
    }
    else {
        switch (event->type()) {
            case QEvent::Wheel:
                qcpAxis->wheelEvent(dynamic_cast<QWheelEvent*>(event));
                break;
            case QEvent::MouseButtonPress:
            case QEvent::MouseMove:
            case QEvent::MouseButtonRelease:
            case QEvent::MouseButtonDblClick:
            case QEvent::Leave:
                procAxisEvent(qcpAxis, event);
                break;
            case QEvent::KeyPress:
                procKeyboardEvent(event);
                break;
            default:
                break;
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
                plot->impl()->cancelZoom();
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

void CanvasEventFilter::procAxisEvent(QCPAxis *axis, QEvent *event)
{DDD;
    QMouseEvent *mEvent = static_cast<QMouseEvent *>(event);
    switch (event->type()) {
        case QEvent::Leave: {
            emit hover(fromQcpAxis(axis->axisType()), 0);
            actionType = ActionType::None;
            break;
        }
        case QEvent::MouseButtonPress: {
            startPosition = mEvent->pos();
            axis->mousePressEvent(mEvent, QVariant());
            break;
        }
        case QEvent::MouseMove: {
            axis->mouseMoveEvent(mEvent, startPosition);
            break;
        }
        case QEvent::MouseButtonRelease: {
            axis->mouseReleaseEvent(mEvent, startPosition);
            break;
        }
        case QEvent::MouseButtonDblClick:
            // обрабатываем прямо в QCPPlot
            break;

        default: break;
    }
}

void CanvasEventFilter::mousePress(QMouseEvent *event)
{DDD;
    startPosition = event->pos();

    switch (event->button()) {
        case Qt::RightButton: {
            actionType = ActionType::Drag;
            plot->impl()->axisRect()->mousePressEvent(event, QVariant());
            break;
        }
        case Qt::LeftButton: {
            auto selected = picker->findObject(event);

            if (picker->alreadySelected(selected) ||
                (selected.object && selected.object->draggable())) {
                actionType = ActionType::Pick;
                picker->startPick(event->pos(), selected);
            }
            else {
                actionType = ActionType::Zoom;
                plot->impl()->startZoom(event);
            }

            break;
        }
        default: break;
    }
}

void CanvasEventFilter::mouseMove(QMouseEvent *event)
{DDD;
    if (actionType == ActionType::Drag) {
        plot->impl()->axisRect()->mouseMoveEvent(event, startPosition);
    }
    else if (actionType == ActionType::Zoom) {
        plot->impl()->proceedZoom(event);
    }
    else if (actionType == ActionType::Pick) {
       picker->proceedPick(event);
    }
}

void CanvasEventFilter::mouseRelease(QMouseEvent *event)
{DDD;
    if (actionType == ActionType::Drag) {
        plot->impl()->axisRect()->mouseReleaseEvent(event, startPosition);

        actionType = ActionType::None;
        if (startPosition == event->pos()) {
            if (auto s = picker->findObject(event); s.object) {
                picker->showContextMenu(event);
            }
        }
    }
    else if (actionType == ActionType::Zoom) {
        if ((startPosition - event->pos()).manhattanLength() <= 3) {
            //no actual mouse move
            plot->impl()->cancelZoom();
            picker->endPick(event);
        }
        else
            plot->impl()->endZoom(event);
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
            emit canvasDoubleClicked(startPosition);
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
