#include "qcpcursorsingle.h"

#include "qcptrackingcursor.h"
#include "plotmodel.h"
#include "plot.h"
#include <QPen>
#include "curve.h"
#include "logging.h"
#include "qcpaxistag.h"
#include "qcpplot.h"
#include "algorithms.h"

QCPCursorSingle::QCPCursorSingle(Style style, Plot *plot) : Cursor(Cursor::Type::Single, style, plot), plot(plot)
{DD;
    cursor = new QCPTrackingCursor(m_color, style, this);
    if (style != Cursor::Style::Horizontal)
        axisTagX = new QCPAxisTag(plot, cursor, Enums::AxisType::atBottom);

    if (style != Cursor::Style::Vertical)
        axisTagY = new QCPAxisTag(plot, cursor, Enums::AxisType::atLeft);

    plot->addSelectable(cursor);
}

QCPCursorSingle::~QCPCursorSingle()
{DD;
    detach();
    delete cursor;
    delete axisTagX;
    delete axisTagY;
}

void QCPCursorSingle::setColor(const QColor &color)
{DD;
    Cursor::setColor(color);
    cursor->setColor(color);
}

void QCPCursorSingle::moveTo(const QPointF &pos1, const QPointF &pos2, bool silent)
{DD;
    Q_UNUSED(pos2);
    moveTo(pos1, silent);
}

void QCPCursorSingle::moveTo(const QPointF &pos1, bool silent)
{DD;
    auto pos = m_snapToValues ? correctedPos(pos1) : pos1;

    cursor->moveTo(pos);
    if (axisTagX) axisTagX->updatePosition(pos.x());
    if (axisTagY) axisTagY->updatePosition(pos.y());
    if (!silent) emit cursorPositionChanged();
    update();
}

void QCPCursorSingle::moveTo(const QPointF &pos1, QCPTrackingCursor *source, bool silent)
{DD;
    if (source == cursor) moveTo(pos1, silent);
}

void QCPCursorSingle::moveTo(Qt::Key key, int count, QCPTrackingCursor *source, bool silent)
{DD;
    if (count == 0 || source != cursor) return;
    QPointF pos = cursor->value();
    double rangeX = m_plot->plotRange(cursor->xAxis()).dist();
    double rangeY = m_plot->plotRange(cursor->yAxis()).dist();

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
        case Qt::Key_Up: {
            if (m_snapToValues && m_plot->type()==Enums::PlotType::Spectrogram)
                pos = correctedPos(pos, 0, count);
            else
                pos.ry() += count*rangeY/100;
            break;
        }
        case Qt::Key_Down: {
            if (m_snapToValues && m_plot->type()==Enums::PlotType::Spectrogram)
                pos = correctedPos(pos, 0, -count);
            else
                pos.ry() -= count*rangeY/100;
            break;
        }
        default: break;
    }

    moveTo(pos, silent);
}

void QCPCursorSingle::updatePos()
{DD;
    auto pos = cursor->value();
    pos = correctedPos(pos);
    cursor->moveTo(pos);
    update();
}

void QCPCursorSingle::attach()
{DD;

}

void QCPCursorSingle::detach()
{DD;
    m_plot->removeSelectable(cursor);
    cursor->detach();
    if (axisTagX) axisTagX->detach();
    if (axisTagY) axisTagY->detach();
}

bool QCPCursorSingle::contains(Selectable *selected) const
{DD;
    if (auto c = dynamic_cast<QCPTrackingCursor*>(selected))
        return c == cursor;
    else if (auto l = dynamic_cast<QCPAxisTag*>(selected))
        return l == axisTagX || l == axisTagY;

    return false;
}

void QCPCursorSingle::update()
{DD;
    if (axisTagX) axisTagX->updateLabel(m_showValues);
    if (axisTagY) axisTagY->updateLabel(m_showValues);
    cursor->replot();
}

QStringList QCPCursorSingle::dataHeader(bool allData) const
{DD;
    Q_UNUSED(allData);
    return {/*"", "Время, с", QString("Частота ")+*/QLocale(QLocale::Russian).toString(cursor->xValue())};
}

QList<double> QCPCursorSingle::data(int curve, bool allData) const
{DD;
    Q_UNUSED(allData);

    if (auto c = m_plot->model()->curves().at(curve)) {
        bool success = false;
        return {c->channel->data()->YforXandZ(cursor->xValue(), cursor->yValue(), success)};
    }
    return QList<double>();
}

QPointF QCPCursorSingle::currentPosition() const
{DD;
    return cursor->value();
}

