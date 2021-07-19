#include "canvaseventfilter.h"

#include "plot.h"
#include "chartzoom.h"
#include "dragzoom.h"
#include "wheelzoom.h"
#include "axiszoom.h"
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
            startPress(mEvent);
            break;
        case QEvent::MouseMove:
            moveMouse(mEvent);
            break;
        case QEvent::MouseButtonRelease:
            endPress(mEvent);
            break;
        default: ;
    }
}

void CanvasEventFilter::procKeyboardEvent(QEvent *event)
{

}

void CanvasEventFilter::procWheelEvent(QwtAxisId axis, QEvent *event)
{
    auto coords = wheelZoom->applyWheel(event, axis);
    zoom->addZoom(coords, false);
}

void CanvasEventFilter::procAxisEvent(QwtAxisId axis, QEvent *event)
{
    QMouseEvent *mEvent = static_cast<QMouseEvent *>(event);
    switch (event->type()) {
        case QEvent::Leave:
            emit hover(axis,0);
            actionType = ActionType::None;
            break;
        case QEvent::MouseButtonPress:
            if (mEvent->button()==Qt::RightButton) {
                emit contextMenuRequested(mEvent->globalPos(), axis);
                actionType = ActionType::None;
            }
            else if (mEvent->button()==Qt::LeftButton) {
                if (axis.isXAxis())
                    axisZoom->startHorizontalAxisZoom(mEvent, axis);
                else
                    axisZoom->startVerticalAxisZoom(mEvent, axis);
            }
            break;

        case QEvent::MouseMove: {
            auto coords = axisZoom->proceedAxisZoom(mEvent, axis);
            zoom->addZoom(coords, false);
            break;
        }
        case QEvent::MouseButtonRelease: {
            auto coords = axisZoom->endAxisZoom(mEvent, axis);
            zoom->addZoom(coords, true);
            break;
        }
        case QEvent::MouseButtonDblClick:
            if (mEvent->button()==Qt::LeftButton) {
                AxisBoundsDialog dialog(plot->canvasMap(axis).s1(), plot->canvasMap(axis).s2(), axis);
                if (dialog.exec()) {
                    ChartZoom::zoomCoordinates coords;
                    if (axis == QwtAxis::xBottom || axis == QwtPlot::xTop)
                        coords.coords.insert(QwtAxis::xBottom, {dialog.leftBorder(), dialog.rightBorder()});
                    else if (zoom->verticalScaleBounds->axis == axis.pos ||
                             zoom->verticalScaleBoundsSlave->axis == axis.pos)
                        coords.coords.insert(axis.pos, {dialog.leftBorder(), dialog.rightBorder()});
                    zoom->addZoom(coords, true);

//                    if (dialog.autoscale()) {
//                        emit needsAutoscale(axis);
//                    }
//                    zoom->plot()->replot();
                }
            }
            break;


        default: break;
    }
}

void CanvasEventFilter::startPress(QMouseEvent *event)
{
    currentPosition = event->pos();

    switch (event->button()) {
        case Qt::RightButton: {
            actionType = ActionType::Drag;
            dragZoom->startDrag(event);
            break;
        }
        case Qt::LeftButton: {

            break;
        }
    }
}

void CanvasEventFilter::moveMouse(QMouseEvent *event)
{
    if (actionType == ActionType::Drag) {
        auto coords = dragZoom->proceedDrag(event);
        zoom->addZoom(coords, false);
    }
}

void CanvasEventFilter::endPress(QMouseEvent *event)
{DD;
    switch (event->button()) {
        case Qt::RightButton: {
            actionType = ActionType::None;
            auto coords = dragZoom->endDrag(event);
            zoom->addZoom(coords, true);
            break;
        }
        case Qt::LeftButton: {

            break;
        }
    }
}
