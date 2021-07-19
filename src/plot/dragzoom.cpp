#include "dragzoom.h"

#include "logging.h"
#include "zoomstack.h"
#include "plot.h"

DragZoom::DragZoom(Plot *plot) : QObject(plot), plot(plot)
{DD;
}

void DragZoom::startDrag(QMouseEvent *mEvent)
{DDD;
    QWidget *canvas = plot->canvas();

    position = mEvent->pos();

    tCursor = canvas->cursor();
    canvas->setCursor(Qt::OpenHandCursor);

    dx = dy = dy1 = 0.0;

    QwtScaleMap sm = plot->canvasMap(QwtAxis::xBottom);
    minHorizontalBound = sm.s1();
    maxHorizontalBound = sm.s2();

    if (mEvent->modifiers() & Qt::ControlModifier) {
        QwtAxisId mY(QwtAxis::yRight);
        sm = plot->canvasMap(mY);
        minVerticalBound1 = sm.s1();
        maxVerticalBound1 = sm.s2();
    }
    else {
        QwtAxisId mY(QwtAxis::yLeft);
        sm = plot->canvasMap(mY);
        minVerticalBound = sm.s1();
        maxVerticalBound = sm.s2();
    }
}


ZoomStack::zoomCoordinates DragZoom::proceedDrag(QMouseEvent *mEvent)
{DD;
    plot->canvas()->setCursor(Qt::ClosedHandCursor);
    auto mousePos = mEvent->pos();

    dx = plot->invTransform(QwtAxis::xBottom, position.x()) - plot->invTransform(QwtAxis::xBottom, mousePos.x());

    if (mEvent->modifiers() & Qt::ControlModifier) {
        dy1 = plot->invTransform(QwtAxis::yRight, position.y()) - plot->invTransform(QwtAxis::yRight, mousePos.y());
        dy = 0.0;
    }
    else {
        dy = plot->invTransform(QwtAxis::yLeft, position.y()) - plot->invTransform(QwtAxis::yLeft, mousePos.y());
        dy1 = 0.0;
    }

    ZoomStack::zoomCoordinates coords;
    if (!qFuzzyIsNull(dx))
        coords.coords.insert(QwtAxis::xBottom, {minHorizontalBound + dx, maxHorizontalBound + dx});
    if (!qFuzzyIsNull(dy))
        coords.coords.insert(QwtAxis::yLeft, {minVerticalBound + dy, maxVerticalBound + dy});
    if (!qFuzzyIsNull(dy1))
        coords.coords.insert(QwtAxis::yRight, {minVerticalBound1 + dy1, maxVerticalBound1 + dy1});

    return coords;
}

ZoomStack::zoomCoordinates DragZoom::endDrag(QMouseEvent *mEvent)
{DDD;
    Q_UNUSED(mEvent);

    plot->canvas()->setCursor(tCursor);

    ZoomStack::zoomCoordinates coords;
    if (!qFuzzyIsNull(dx)) {
        coords.coords.insert(QwtAxis::xBottom, {minHorizontalBound + dx, maxHorizontalBound + dx});
    }
    if (!qFuzzyIsNull(dy)) {
        coords.coords.insert(QwtAxis::yLeft, {minVerticalBound + dy, maxVerticalBound + dy});
    }
    if (!qFuzzyIsNull(dy1)) {
        coords.coords.insert(QwtAxis::yRight, {minVerticalBound1 + dy1, maxVerticalBound1 + dy1});
    }
    return coords;
}
