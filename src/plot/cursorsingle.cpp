#include "cursorsingle.h"

#include "trackingcursor.h"
#include "plotmodel.h"
#include "plot.h"
#include <QPen>
#include "cursorlabel.h"

CursorSingle::CursorSingle(Style style, Plot *plot) : Cursor(Cursor::Type::Single, style, plot)
{
    cursor = new TrackingCursor(color, style, this);
//    cursor->setCurrent(true);
    cursor->showLabel = false;
    if (style != Cursor::Style::Horizontal) {
        xlabel = new CursorLabel(plot, cursor);
        xlabel->setAxis(CursorLabel::Axis::XAxis);
        xlabel->setShowValues(true);
        xlabel->updateAlignment();
    }
    if (style != Cursor::Style::Vertical) {
        ylabel = new CursorLabel(plot, cursor);
        ylabel->setAxis(CursorLabel::Axis::YAxis);
        ylabel->setShowValues(false);
        ylabel->updateAlignment();
    }
}

CursorSingle::~CursorSingle()
{
    detach();
    delete xlabel;
    delete ylabel;
    delete cursor;
}

void CursorSingle::setColor(const QColor &color)
{
    Cursor::setColor(color);
    auto pen = cursor->linePen();
    pen.setColor(color);
    cursor->setLinePen(pen);
}

void CursorSingle::moveTo(const QPointF &pos1, const QPointF &pos2)
{
    Q_UNUSED(pos2);
    moveTo(pos1);
}

void CursorSingle::moveTo(const QPointF &pos1)
{
    auto pos = snapToValues ? correctedPos(pos1) : pos1;

    cursor->xVal = pos.x();
    cursor->yVal = 0;
    cursor->zVal = 0;
    cursor->moveTo(pos);
    if (xlabel) xlabel->updateLabel();
    if (ylabel) ylabel->updateLabel();
}

void CursorSingle::updatePos()
{
    auto pos = cursor->value();
    pos = correctedPos(pos);
    cursor->xVal = pos.x();
    cursor->yVal = 0;
    cursor->zVal = 0;
    cursor->moveTo(pos);
    if (xlabel) xlabel->updateLabel();
    if (ylabel) ylabel->updateLabel();
}

void CursorSingle::attach()
{
    cursor->attach(plot);
    if (xlabel) xlabel->attach(plot);
    if (ylabel) ylabel->attach(plot);
}

void CursorSingle::detach()
{
    cursor->detach();
    if (xlabel) xlabel->detach();
    if (ylabel) ylabel->detach();
}
