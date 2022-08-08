#include "cursorsingle.h"

#include "trackingcursor.h"
#include "plotmodel.h"
#include "plot.h"
#include <QPen>
#include "cursorlabel.h"
#include <qwt_scale_map.h>
#include "curve.h"

CursorSingle::CursorSingle(Style style, Plot *plot) : Cursor(Cursor::Type::Single, style, plot)
{
    cursor = new TrackingCursor(m_color, style, this);
    if (style != Cursor::Style::Horizontal) {
        xlabel = new CursorLabel(plot, cursor);
        xlabel->setAxis(CursorLabel::Axis::XAxis);
        xlabel->updateAlignment();
    }
    if (style != Cursor::Style::Vertical) {
        ylabel = new CursorLabel(plot, cursor);
        ylabel->setAxis(CursorLabel::Axis::YAxis);
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

void CursorSingle::moveTo(const QPointF &pos1, const QPointF &pos2, bool silent)
{
    Q_UNUSED(pos2);
    moveTo(pos1, silent);
}

void CursorSingle::moveTo(const QPointF &pos1, bool silent)
{
    auto pos = m_snapToValues ? correctedPos(pos1) : pos1;

    cursor->moveTo(pos);
    if (!silent) emit cursorPositionChanged();
    update();
}

void CursorSingle::moveTo(const QPointF &pos1, TrackingCursor *source, bool silent)
{
    if (source == cursor) moveTo(pos1, silent);
}

void CursorSingle::moveTo(Qt::Key key, int count, TrackingCursor *source, bool silent)
{
    if (count == 0 || source != cursor) return;
    QPointF pos = cursor->value();
    double rangeX = m_plot->canvasMap(cursor->xAxis()).sDist();
    double rangeY = m_plot->canvasMap(cursor->yAxis()).sDist();

    switch (key) {
        case Qt::Key_Left: {
            if (m_snapToValues) {
                pos = correctedPos(pos, -count, 0);
            }
            else
                pos.rx() -= count*rangeX/100;
            break;
        }
        case Qt::Key_Right: {
            if (m_snapToValues) {
                pos = correctedPos(pos, count, 0);
            }
            else
                pos.rx() += count*rangeX/100;
            break;
        }
        case Qt::Key_Up: pos.ry() += count*rangeY/100; break;
        case Qt::Key_Down: pos.ry() -= count*rangeY/100; break;
        default: break;
    }

    moveTo(pos, silent);
}

void CursorSingle::updatePos()
{
    auto pos = cursor->value();
    pos = correctedPos(pos);
    cursor->moveTo(pos);
    update();
}

void CursorSingle::attach()
{
    cursor->attach(m_plot);
    if (xlabel) xlabel->attach(m_plot);
    if (ylabel) ylabel->attach(m_plot);
}

void CursorSingle::detach()
{
    cursor->detach();
    if (xlabel) xlabel->detach();
    if (ylabel) ylabel->detach();
}

bool CursorSingle::contains(Selectable *selected) const
{
    if (auto c = dynamic_cast<TrackingCursor*>(selected))
        return c == cursor;
    else if (auto l = dynamic_cast<CursorLabel*>(selected))
        return l == xlabel || l == ylabel;

    return false;
}

void CursorSingle::update()
{
    if (xlabel) xlabel->updateLabel(m_showValues);
    if (ylabel) ylabel->updateLabel(m_showValues);
}

QStringList CursorSingle::dataHeader(bool allData) const
{
    Q_UNUSED(allData);
    return {/*"", "Время, с", QString("Частота ")+*/QLocale(QLocale::Russian).toString(cursor->xValue())};
}

QList<double> CursorSingle::data(int curve, bool allData) const
{
    Q_UNUSED(allData);

    if (auto c = m_plot->model()->curves().at(curve)) {
        bool success = false;
        return {c->channel->data()->YforXandZ(cursor->xValue(), cursor->yValue(), success)};
    }
    return QList<double>();
}

QPointF CursorSingle::currentPosition() const
{
    return cursor->value();
}

