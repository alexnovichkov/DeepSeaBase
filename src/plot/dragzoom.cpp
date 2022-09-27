#include "dragzoom.h"

#include "logging.h"
#include "zoomstack.h"
#include "qwtplotimpl.h"

DragZoom::DragZoom(QwtPlotImpl *plot) : QObject(plot), plot(plot)
{DDD;
}

void DragZoom::startDrag(QMouseEvent *mEvent)
{DDD;
    QWidget *canvas = plot->canvas();

    position = mEvent->pos();

    tCursor = canvas->cursor();
    canvas->setCursor(Qt::OpenHandCursor);

    dx = dy = dy1 = 0.0;

    auto range = plot->plotRange(Enums::AxisType::atBottom);
    minHorizontalBound = range.min;
    maxHorizontalBound = range.max;

    if (mEvent->modifiers() & Qt::ControlModifier) {
        range = plot->plotRange(Enums::AxisType::atRight);
        minVerticalBound1 = range.min;
        maxVerticalBound1 = range.max;
    }
    else {
        range = plot->plotRange(Enums::AxisType::atLeft);
        minVerticalBound = range.min;
        maxVerticalBound = range.max;
    }
}


ZoomStack::zoomCoordinates DragZoom::proceedDrag(QMouseEvent *mEvent)
{DDD;
    plot->canvas()->setCursor(Qt::ClosedHandCursor);
    auto mousePos = mEvent->pos();

    dx = plot->screenToPlotCoordinates(Enums::AxisType::atBottom, position.x())
         - plot->screenToPlotCoordinates(Enums::AxisType::atBottom, mousePos.x());

    if (mEvent->modifiers() & Qt::ControlModifier) {
        dy1 = plot->screenToPlotCoordinates(Enums::AxisType::atRight, position.y())
              - plot->screenToPlotCoordinates(Enums::AxisType::atRight, mousePos.y());
        dy = 0.0;
    }
    else {
        dy = plot->screenToPlotCoordinates(Enums::AxisType::atLeft, position.y())
             - plot->screenToPlotCoordinates(Enums::AxisType::atLeft, mousePos.y());
        dy1 = 0.0;
    }

    ZoomStack::zoomCoordinates coords;
    if (!qFuzzyIsNull(dx))
        coords.coords.insert(Enums::AxisType::atBottom, {minHorizontalBound + dx, maxHorizontalBound + dx});
    if (!qFuzzyIsNull(dy))
        coords.coords.insert(Enums::AxisType::atLeft, {minVerticalBound + dy, maxVerticalBound + dy});
    if (!qFuzzyIsNull(dy1))
        coords.coords.insert(Enums::AxisType::atRight, {minVerticalBound1 + dy1, maxVerticalBound1 + dy1});

    return coords;
}

ZoomStack::zoomCoordinates DragZoom::endDrag(QMouseEvent *mEvent)
{DDD;
    Q_UNUSED(mEvent);

    plot->canvas()->setCursor(tCursor);

    ZoomStack::zoomCoordinates coords;
    if (!qFuzzyIsNull(dx)) {
        coords.coords.insert(Enums::AxisType::atBottom, {minHorizontalBound + dx, maxHorizontalBound + dx});
    }
    if (!qFuzzyIsNull(dy)) {
        coords.coords.insert(Enums::AxisType::atLeft, {minVerticalBound + dy, maxVerticalBound + dy});
    }
    if (!qFuzzyIsNull(dy1)) {
        coords.coords.insert(Enums::AxisType::atRight, {minVerticalBound1 + dy1, maxVerticalBound1 + dy1});
    }
    return coords;
}
