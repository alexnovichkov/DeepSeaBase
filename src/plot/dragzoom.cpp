#include "dragzoom.h"

#include "logging.h"
#include "zoomstack.h"
#include "plot.h"

DragZoom::DragZoom(Plot *plot) : QObject(plot), plot(plot)
{DD;
}

void DragZoom::startDrag(QMouseEvent *mEvent)
{DD;
    QWidget *canvas = plot->canvas();

    position = mEvent->pos();

    tCursor = canvas->cursor();
    canvas->setCursor(Qt::OpenHandCursor);

    dx = dy = dy1 = 0.0;

    QwtScaleMap sm = plot->canvasMap(QwtAxis::XBottom);
    minHorizontalBound = sm.s1();
    maxHorizontalBound = sm.s2();

    if (mEvent->modifiers() & Qt::ControlModifier) {
        QwtAxisId mY(QwtAxis::YRight);
        sm = plot->canvasMap(mY);
        minVerticalBound1 = sm.s1();
        maxVerticalBound1 = sm.s2();
    }
    else {
        QwtAxisId mY(QwtAxis::YLeft);
        sm = plot->canvasMap(mY);
        minVerticalBound = sm.s1();
        maxVerticalBound = sm.s2();
    }
}


ZoomStack::zoomCoordinates DragZoom::proceedDrag(QMouseEvent *mEvent)
{DD;
    plot->canvas()->setCursor(Qt::ClosedHandCursor);
    auto mousePos = mEvent->pos();

    dx = plot->invTransform(QwtAxis::XBottom, position.x()) - plot->invTransform(QwtAxis::XBottom, mousePos.x());

    if (mEvent->modifiers() & Qt::ControlModifier) {
        dy1 = plot->invTransform(QwtAxis::YRight, position.y()) - plot->invTransform(QwtAxis::YRight, mousePos.y());
        dy = 0.0;
    }
    else {
        dy = plot->invTransform(QwtAxis::YLeft, position.y()) - plot->invTransform(QwtAxis::YLeft, mousePos.y());
        dy1 = 0.0;
    }

    ZoomStack::zoomCoordinates coords;
    if (!qFuzzyIsNull(dx))
        coords.coords.insert(QwtAxis::XBottom, {minHorizontalBound + dx, maxHorizontalBound + dx});
    if (!qFuzzyIsNull(dy))
        coords.coords.insert(QwtAxis::YLeft, {minVerticalBound + dy, maxVerticalBound + dy});
    if (!qFuzzyIsNull(dy1))
        coords.coords.insert(QwtAxis::YRight, {minVerticalBound1 + dy1, maxVerticalBound1 + dy1});

    return coords;
}

ZoomStack::zoomCoordinates DragZoom::endDrag(QMouseEvent *mEvent)
{DD;
    Q_UNUSED(mEvent);

    plot->canvas()->setCursor(tCursor);

    ZoomStack::zoomCoordinates coords;
    if (!qFuzzyIsNull(dx)) {
        coords.coords.insert(QwtAxis::XBottom, {minHorizontalBound + dx, maxHorizontalBound + dx});
    }
    if (!qFuzzyIsNull(dy)) {
        coords.coords.insert(QwtAxis::YLeft, {minVerticalBound + dy, maxVerticalBound + dy});
    }
    if (!qFuzzyIsNull(dy1)) {
        coords.coords.insert(QwtAxis::YRight, {minVerticalBound1 + dy1, maxVerticalBound1 + dy1});
    }
    return coords;
}
