#include "qwtlinecurve.h"

#include "qwt_point_mapper.h"
#include "qwt_scale_map.h"
#include "qwt_painter.h"
#include "qwt_clipper.h"
#include "qwt_text.h"
#include "qwt_legend_data.h"
#include "logging.h"
#include "fileformats/filedescriptor.h"
#include "plot.h"
#include <QPainter>
#include "pointlabel.h"
#include "filterpointmapper.h"
#include "qwtplotimpl.h"

QwtLineCurve::QwtLineCurve(const QString &title, Channel *channel) :  QwtPlotCurve(title),
    Curve(title, channel)
{DDD;
    type = Type::Line;
    setPaintAttribute(QwtPlotCurve::ClipPolygons);
    setPaintAttribute(QwtPlotCurve::FilterPoints);
    setRenderHint(QwtPlotItem::RenderAntialiased);

    setLegendIconSize(QSize(16,8));

    dfddata = new LineData(this->channel->data());
    setData(dfddata);
    setMapper();
    // set filter points to true
    const bool noDuplicates = true;
    mapper->setFlag(QwtPointMapper::WeedOutIntermediatePoints, noDuplicates);
}

QwtLineCurve::~QwtLineCurve()
{DDD;
    delete mapper;
}

void QwtLineCurve::setMapper()
{DDD;
    mapper = new FilterPointMapper(channel->type()==Descriptor::TimeResponse);
}

void QwtLineCurve::drawLines(QPainter *painter,
                      const QwtScaleMap &xMap, const QwtScaleMap &yMap,
                      const QRectF &canvasRect,
                      int from, int to) const
{DDD;
    //reevaluating from, to
    evaluateScale(from, to, xMap.s1(), xMap.s2());
    if (from > to) return;

    const bool doAlign = QwtPainter::roundingAlignment( painter );

    QRectF clipRect;
    painter->save();

    //clip polygons
    qreal pw = qMax(qreal(1.0), painter->pen().widthF());
    clipRect = canvasRect.adjusted(-pw, -pw, pw, pw);

    mapper->setFlag(QwtPointMapper::RoundPoints, doAlign);
    mapper->setBoundingRect(canvasRect);

    QPolygonF polyline = mapper->getPolygon(xMap, yMap, dfddata, from, to);
    const bool close = doCloseLine();
    QwtClipper::clipPolygonF(clipRect, polyline, close);

    if (close) {
        QColor c = pen().color();
        c.setAlpha(200);
        painter->setBrush(QBrush(c));
        painter->setPen(c);
    }

    if (close)
        QwtPainter::drawPolygon(painter, polyline);
    else
        QwtPainter::drawPolyline(painter, polyline);

    painter->restore();
}

bool QwtLineCurve::doCloseLine() const
{DDD;
    return mapper->simplified && channel->type()==Descriptor::TimeResponse;
}

SamplePoint QwtLineCurve::samplePoint(SelectedPoint point) const
{DDD;
    auto s = QwtPlotCurve::sample(point.x);
    return {s.x(), s.y(), qQNaN()};
}

void QwtLineCurve::resetCashedData()
{DDD;
    mapper->cashedPolyline.clear();
}

void QwtLineCurve::attachTo(QwtPlot *plot)
{DDD;
    QwtPlotCurve::attach(plot);
}

QString QwtLineCurve::title() const
{DDD;
    return QwtPlotCurve::title().text();
}

void QwtLineCurve::setTitle(const QString &title)
{DDD;
    QwtPlotCurve::setTitle(title);
}

Enums::AxisType QwtLineCurve::yAxis() const
{DDD;
    return toAxisType(QwtPlotCurve::yAxis());
}

void QwtLineCurve::setYAxis(Enums::AxisType axis)
{DDD;
    QwtPlotCurve::setYAxis(toQwtAxisType(axis));
    foreach (PointLabel *l, labels)
        l->setYAxis(toQwtAxisType(axis));
}

Enums::AxisType QwtLineCurve::xAxis() const
{DDD;
    return toAxisType(QwtPlotCurve::xAxis());
}

void QwtLineCurve::setXAxis(Enums::AxisType axis)
{DDD;
    QwtPlotCurve::setXAxis(toQwtAxisType(axis));
    foreach (PointLabel *l, labels)
        l->setXAxis(toQwtAxisType(axis));
}

QPen QwtLineCurve::pen() const
{DDD;
    return QwtPlotCurve::pen();
}

void QwtLineCurve::updatePen()
{DDD;
    auto p = oldPen;
    if (selected()) p.setWidth(2);
    QwtPlotCurve::setPen(p);
}

QList<QwtLegendData> QwtLineCurve::legendData() const
{DDD;
    QList<QwtLegendData> result = QwtPlotCurve::legendData();
    QwtLegendData &data = result[0];
    data.setValues(commonLegendData());
    return result;
}

void QwtLineCurve::updateSelection(SelectedPoint point)
{DDD;
    Curve::updateSelection(point);
    if (selected()) setZ(1000);
    else setZ(20);
    plot()->updateLegend(this);
}

/** DfdData implementation */

LineData::LineData(DataHolder *data) : data(data)
{DDD;

}

QRectF LineData::boundingRect() const
{DDD;
    QRectF d_boundingRect;
    d_boundingRect.setLeft( data->xMin() );
    d_boundingRect.setRight( data->xMax() );
    d_boundingRect.setTop( data->yMin() );
    d_boundingRect.setBottom( data->yMax() );


    return d_boundingRect;
}

size_t LineData::size() const
{DDD;
    return data->samplesCount();
}

QPointF LineData::sample(size_t i) const
{DDD;
    return QPointF(data->xValue(i), data->yValue(i));
}

double LineData::xStep() const
{DDD;
    return data->xStep();
}

double LineData::xBegin() const
{DDD;
    return data->xMin();
}


SelectedPoint QwtLineCurve::closest(const QPoint &pos, double *dist1, double *dist2) const
{DDD;
    int index = -1;

    const size_t numSamples = channel->data()->samplesCount();
    if ( numSamples <= 0 )
        return {-1, -1};

    int from = 0;
    int to = numSamples-1;
    auto range = m_plot->plotRange(xAxis());
    evaluateScale(from, to, range.min, range.max);


    double dmin = qInf();

    for ( int i = from; i <= to; i++ ) {
        const auto sample = samplePoint( {i,0} );

        const double cx = qAbs(m_plot->plotToScreenCoordinates(xAxis(), sample.x ) - pos.x());
        const double cy = qAbs(m_plot->plotToScreenCoordinates(yAxis(), sample.y ) - pos.y());

        const double f = cx*cx + cy*cy;
        if ( f < dmin ) {
            index = i;
            dmin = f;
            if (dist1) *dist1 = cx;
            if (dist2) *dist2 = cy;
        }
    }

    return {index, 0};
}

void QwtLineCurve::setVisible(bool visible)
{DDD;
    QwtPlotItem::setVisible(visible);
    for (PointLabel *label: qAsConst(labels)) {
        label->setVisible(visible);
    }
}

TimeCurve::TimeCurve(const QString &title, Channel *channel) : QwtLineCurve(title, channel)
{DDD;

}

void TimeCurve::setMapper()
{DDD;
    mapper = new FilterPointMapper(true);
}

bool TimeCurve::doCloseLine() const
{DDD;
    return mapper->simplified;
}

