#include "cursorlabel.h"

#include <QPen>
#include <QPainter>
#include <QVector2D>

#include "plot.h"
#include "plotmodel.h"
#include "trackingcursor.h"
#include "curve.h"
#include "algorithms.h"
#include "qwt_scale_map.h"
//#include <QMenu>
//#include <app.h>
#include "logging.h"

CursorLabel::CursorLabel(Plot *parent, TrackingCursor *cursor)
    : QwtPlotItem(), m_plot{parent}, m_cursor{cursor}
{DDD;
    setZ(40.0);
    m_label.setBackgroundBrush(Qt::white);
    setXAxis(m_cursor->xAxis());
    setYAxis(m_cursor->yAxis());
}

int CursorLabel::rtti() const
{DDD;
    return QwtPlotItem::Rtti_PlotUserItem+2;
}

void CursorLabel::updateAlignment()
{DDD;
    setXAxis(m_cursor->xAxis());
    setYAxis(m_cursor->yAxis());
}

void CursorLabel::setAxis(CursorLabel::Axis axis)
{DDD;
    if (m_axis != axis) {
        m_axis = axis;
        itemChanged();
    }
}

void CursorLabel::updateLabel(bool showValues)
{DDD;
    QStringList label;
    char f = m_cursor->parent->format()==Cursor::Format::Fixed?'f':'e';
    if (showValues && m_axis != Axis::YAxis) {
        auto list = m_plot->model()->curves();
        for (auto curve: list) {
            if (curve->xAxis()==xAxis()) {
                bool success = false;
                auto val = curve->channel->data()->YforXandZ(m_cursor->xValue(), m_cursor->yValue(), success);
                QString s = QString::number(success?val:qQNaN(), f, m_cursor->parent->digits());

                label << QString("<font color=%1>%2</font>")
                         .arg(curve->pen().color().name())
                         .arg(s);
            }
        }
    }
    QString s = QString::number(m_axis==Axis::XAxis?m_cursor->xValue():m_cursor->yValue(), f, m_cursor->parent->digits());
    //while(s.rightRef(1)=="0") s.chop(1);
    label << QString("<b>%1</b>").arg(s);
    m_label.setText(label.join("<br>"),QwtText::RichText);
    itemChanged();
}

void CursorLabel::draw(QPainter *painter, const QwtScaleMap &xMap, const QwtScaleMap &yMap, const QRectF &canvasRect) const
{DDD;
    Q_UNUSED(canvasRect)
    double xval = 0.0;
    double yval = 0.0;
    switch (m_axis) {
        case Axis::XAxis:
            xval = xMap.transform(m_cursor->xValue());
            yval = xAxis() == QwtAxis::XBottom ? yMap.p1() : yMap.p2();
            break;
        case Axis::YAxis:
            yval = yMap.transform(m_cursor->yValue());
            xval = yAxis() == QwtAxis::YLeft ? xMap.p1() : xMap.p2();
            break;
    }

    QPointF pos(xval, yval);
    const QSizeF textSize = m_label.textSize(painter->font());
    pos.rx() += 3;
    pos.ry() -= textSize.height();

    painter->translate(pos.x(), pos.y());
    const QRectF textRect(0, 0, textSize.width(), textSize.height());
    m_label.draw(painter, textRect);
}

bool CursorLabel::underMouse(const QPoint &pos, double *distanceX, double *distanceY, SelectedPoint *point) const
{DDD;
    Q_UNUSED(point);
    double xval = 0.0;
    double yval = 0.0;
    switch (m_axis) {
        case Axis::XAxis:
            xval = m_plot->transform(m_cursor->xAxis(), m_cursor->xValue());
            yval = xAxis() == QwtAxis::XBottom ? m_plot->canvasMap(QwtAxis::YLeft).p1()
                                               : m_plot->canvasMap(QwtAxis::YLeft).p2();
            break;
        case Axis::YAxis:
            yval = m_plot->transform(m_cursor->yAxis(), m_cursor->yValue());
            xval = yAxis() == QwtAxis::YLeft ? m_plot->canvasMap(QwtAxis::XBottom).p1()
                                             : m_plot->canvasMap(QwtAxis::XBottom).p2();
            break;
    }

    QPointF p(xval, yval);

    const QSizeF textSize = m_label.textSize();

    p.rx() += 3;
    p.ry() -= textSize.height();

    auto vec = QVector2D(p);
    vec.setX(vec.x()+textSize.width()/2);
    vec.setY(vec.y()+textSize.height()/2);

    if (distanceX) *distanceX = qAbs(vec.x()-pos.x());
    if (distanceY) *distanceY = qAbs(vec.y()-pos.y());

    return QRectF(p.x(),
                  p.y(),
                  textSize.width(), textSize.height()).contains(pos);
}

QList<QAction *> CursorLabel::actions()
{DDD;
    return m_cursor->actions();
}

void CursorLabel::updateSelection(SelectedPoint point)
{DDD;
    Q_UNUSED(point);
    if (selected()) {
        m_label.setBorderPen(QPen(Qt::darkGray, 0.5, Qt::SolidLine));
        setZ(1000);
    }
    else {
        m_label.setBorderPen(QPen(Qt::NoPen));
        setZ(40);
    }
    itemChanged();
}
