#include "qcpcursorplayer.h"

#include "qcptrackingcursor.h"
#include "plotmodel.h"
#include "plot.h"
#include <QPen>
#include "curve.h"
#include "logging.h"
#include "qcpaxistag.h"
#include "qcpplot.h"
#include "algorithms.h"
#include "qcpaxishandle.h"

QCPCursorPlayer::QCPCursorPlayer(QCPPlot *plot) : Cursor(Cursor::Type::Single, Cursor::Style::Vertical, plot), plot(plot)
{DD;
    cursor = new QCPTrackingCursor(m_color, Cursor::Style::Vertical, this);
    axisTag = new QCPAxisTag(plot, cursor, Enums::AxisType::atBottom, this);
    axisHandle = new QCPAxisHandle(plot, cursor, Enums::AxisType::atBottom, this);

    plot->addSelectable(cursor);
}

QCPCursorPlayer::~QCPCursorPlayer()
{DD;
    detach();
    delete cursor;
}

void QCPCursorPlayer::setColor(const QColor &color)
{DD;
    Cursor::setColor(color);
    cursor->setColor(color);
    axisHandle->setPen(color);
}

void QCPCursorPlayer::moveTo(const QPointF &pos1, const QPointF &pos2, bool silent)
{DD;
    Q_UNUSED(pos2);
    moveTo(pos1, silent);
}

void QCPCursorPlayer::moveTo(const QPointF &pos1, bool silent)
{DD;
    auto pos = m_snapToValues ? correctedPos(pos1) : pos1;

    cursor->moveTo(pos);
    axisTag->updatePosition(pos.x());
    axisHandle->updatePosition(pos.x());
    if (!silent) emit cursorPositionChanged();
    update();
}

void QCPCursorPlayer::moveTo(const QPointF &pos1, QCPTrackingCursor *source, bool silent)
{DD;
    if (source == cursor) moveTo(pos1, silent);
}

void QCPCursorPlayer::moveTo(Qt::Key key, int count, QCPTrackingCursor *source, bool silent)
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

void QCPCursorPlayer::updatePos()
{DD;
    auto pos = cursor->value();
    pos = correctedPos(pos);
    cursor->moveTo(pos);
    update();
}

//void QCPCursorPlayer::attach()
//{DD;

//}

void QCPCursorPlayer::detach()
{DD;
    m_plot->removeSelectable(cursor);
    cursor->detach();
    if (axisTag) {
        m_plot->removeSelectable(axisTag);
        axisTag->detach();
    }
    if (axisHandle) {
        m_plot->removeSelectable(axisHandle);
        axisHandle->detach();
    }
    m_plot->layer("overlay")->replot();
}

bool QCPCursorPlayer::contains(Selectable *selected) const
{DD;
    if (auto c = dynamic_cast<QCPTrackingCursor*>(selected))
        return c == cursor;
    else if (auto l = dynamic_cast<QCPAxisTag*>(selected))
        return l == axisTag;
    else if (auto l = dynamic_cast<QCPAxisHandle*>(selected))
        return l == axisHandle;

    return false;
}

void QCPCursorPlayer::update()
{DD;
    if (axisTag) axisTag->updateLabel(m_showValues);
    cursor->replot();
}

QStringList QCPCursorPlayer::dataHeader(bool allData) const
{DD;
    Q_UNUSED(allData);
    return {QLocale(QLocale::Russian).toString(cursor->xValue())};
}

QList<double> QCPCursorPlayer::data(int curve, bool allData) const
{DD;
    Q_UNUSED(allData);

    if (auto c = m_plot->model()->curves().at(curve)) {
        bool success = false;
        return {c->channel->data()->YforXandZ(cursor->xValue(), cursor->yValue(), success)};
    }
    return QList<double>();
}

QPointF QCPCursorPlayer::currentPosition() const
{DD;
    return cursor->value();
}

