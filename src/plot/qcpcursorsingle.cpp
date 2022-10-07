#include "qcpcursorsingle.h"

#include "qcptrackingcursor.h"
#include "plotmodel.h"
#include "plot.h"
#include <QPen>
#include "cursorlabel.h"
#include "curve.h"
#include "logging.h"


QCPCursorSingle::QCPCursorSingle(Style style, Plot *plot) : Cursor(Cursor::Type::Single, style, plot)
{DDD;
    cursor = new QCPTrackingCursor(m_color, style, this);
    if (style != Cursor::Style::Horizontal) {
//        xlabel = new CursorLabel(plot, cursor);
//        xlabel->setAxis(CursorLabel::Axis::XAxis);
//        xlabel->updateAlignment();
    }
    if (style != Cursor::Style::Vertical) {
//        ylabel = new CursorLabel(plot, cursor);
//        ylabel->setAxis(CursorLabel::Axis::YAxis);
//        ylabel->updateAlignment();
    }
    plot->addSelectable(cursor);
}

QCPCursorSingle::~QCPCursorSingle()
{DDD;
    detach();
//    delete xlabel;
//    delete ylabel;
    delete cursor;
}

void QCPCursorSingle::setColor(const QColor &color)
{DDD;
    Cursor::setColor(color);
    cursor->setColor(color);
}

void QCPCursorSingle::moveTo(const QPointF &pos1, const QPointF &pos2, bool silent)
{DDD;
    Q_UNUSED(pos2);
    moveTo(pos1, silent);
}

void QCPCursorSingle::moveTo(const QPointF &pos1, bool silent)
{DDD;
    auto pos = m_snapToValues ? correctedPos(pos1) : pos1;

    cursor->moveTo(pos);
    if (!silent) emit cursorPositionChanged();
    update();
}

void QCPCursorSingle::moveTo(const QPointF &pos1, QCPTrackingCursor *source, bool silent)
{DDD;
    if (source == cursor) moveTo(pos1, silent);
}

void QCPCursorSingle::moveTo(Qt::Key key, int count, QCPTrackingCursor *source, bool silent)
{DDD;
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
{DDD;
    auto pos = cursor->value();
    pos = correctedPos(pos);
    cursor->moveTo(pos);
    update();
}

void QCPCursorSingle::attach()
{DDD;

}

void QCPCursorSingle::detach()
{DDD;
    m_plot->removeSelectable(cursor);
    cursor->detach();
//    if (xlabel) xlabel->detach();
//    if (ylabel) ylabel->detach();
}

bool QCPCursorSingle::contains(Selectable *selected) const
{DDD;
    if (auto c = dynamic_cast<QCPTrackingCursor*>(selected))
        return c == cursor;
//    else if (auto l = dynamic_cast<CursorLabel*>(selected))
//        return l == xlabel || l == ylabel;

    return false;
}

void QCPCursorSingle::update()
{DDD;
//    if (xlabel) xlabel->updateLabel(m_showValues);
//    if (ylabel) ylabel->updateLabel(m_showValues);
}

QStringList QCPCursorSingle::dataHeader(bool allData) const
{DDD;
    Q_UNUSED(allData);
    return {/*"", "Время, с", QString("Частота ")+*/QLocale(QLocale::Russian).toString(cursor->xValue())};
}

QList<double> QCPCursorSingle::data(int curve, bool allData) const
{DDD;
    Q_UNUSED(allData);

    if (auto c = m_plot->model()->curves().at(curve)) {
        bool success = false;
        return {c->channel->data()->YforXandZ(cursor->xValue(), cursor->yValue(), success)};
    }
    return QList<double>();
}

QPointF QCPCursorSingle::currentPosition() const
{DDD;
    return cursor->value();
}

