#include "linecurve.h"

#include "qwt_point_mapper.h"
#include "qwt_scale_map.h"
#include "qwt_painter.h"
#include "qwt_clipper.h"
#include "qwt_text.h"
#include "qwt_legend_data.h"
#include "logging.h"
#include "fileformats/filedescriptor.h"
#include "qwt_plot.h"
#include <QPainter>
#include "pointlabel.h"

struct PointBlock
{
    double minX = 0;
    double maxX = 0;
    double minY = 0;
    double maxY = 0;
    int from = 0;
    int to = 0;
};

typedef QList<PointBlock> PointBlocks;

class FilterPointMapper : public QwtPointMapper
{
public:
    FilterPointMapper(bool createPolygon) : QwtPointMapper(), polygon(createPolygon)
    { }
    QPolygonF getPolygon( const QwtScaleMap &xMap, const QwtScaleMap &yMap,
            const QwtSeriesData<QPointF> *series, int from, int to )
    {
        if (from == oldFrom && to == oldTo) return cashedPolyline;

        //number of visible points for current zoom
        const int pointCount = to - from + 1;
        if (pointCount < 5) {
            simplified = false;
            cashedPolyline = QwtPointMapper::toPolygonF(xMap, yMap, series, from, to);
            return cashedPolyline;
        }

        //number of pixels
//        const int pixels = xMap.pDist();
        const int pixels = int(xMap.transform(series->sample(to).x()) - xMap.transform(series->sample(from).x()));
        if (pixels == 0) {
            simplified = false;
            cashedPolyline = QwtPointMapper::toPolygonF(xMap, yMap, series, from, to);
            return cashedPolyline;
        }

        //we have less than 5* more points than screen pixels - no need to use resample
        if (pointCount <= pixels*5) {
            simplified = false;
            cashedPolyline = QwtPointMapper::toPolygonF(xMap, yMap, series, from, to);
            return cashedPolyline;
        }

        simplified = true;

        //array that will be used to store calculated plot points in screen coordinates
        QPolygonF polyline(pixels*2);
        QPointF *points = polyline.data();

        //iterate over pixels
        int start = from;
        PointBlock block;
//        int startPixel = qRound(xMap.transform(series->sample(start).x()));

        for (int pixel=0; pixel<pixels; ++pixel) {
            if (start == to) {
                qDebug()<<"reached end";
                break;
            }

            int end = (((double)pixel+1.0)/pixels)*pointCount + from;
            if (end > to) end = to;

            QPointF sample1 = series->sample(start);
            QPointF sample2 = series->sample(start+1);

#if 0
            for (int k=start+1; ; ++k) {
                if (k > to) break;
                int endPixel = qRound(xMap.transform(series->sample(k).x()));
                if (endPixel != startPixel) break;
                else end = k;
            }
#endif
            //now find range [min;max] for current pixel

            //first two points
            if(sample1.y() < sample2.y()) {
                block.minY = sample1.y();
                block.maxY = sample2.y();
                block.minX = sample1.x();
                block.maxX = sample2.x();
            }
            else {
                block.minY = sample2.y();
                block.maxY = sample1.y();
                block.minX = sample2.x();
                block.maxX = sample1.x();
            }

            //rest of points
            for(int k=start+2; k<end; k+=2) {
                sample1 = series->sample(k);
                sample2 = series->sample(k+1);
                if(sample1.y() > sample2.y()) {
                    if(sample1.y() > block.maxY) {
                        block.maxY = sample1.y();
                        block.maxX = sample1.x();
                    }

                    if(sample2.y()<block.minY) {
                        block.minY = sample2.y();
                        block.minX = sample2.x();
                    }
                }
                else {
                    if(sample2.y()>block.maxY) {
                        block.maxY = sample2.y();
                        block.maxX = sample2.x();
                    }

                    if(sample1.y()<block.minY) {
                        block.minY = sample1.y();
                        block.minX = sample1.x();
                    }
                }
                block.from = start;
                block.to = end;
            }

            //new start for next iteration
            start = end;
            if (polygon) {
                QPointF &minValue = points[pixel];
                QPointF &maxValue = points[2 * pixels - 1 - pixel];
                minValue.rx() = (xMap.transform(block.minX)+xMap.transform(block.maxX))/2.0;
                minValue.ry() = yMap.transform(block.minY);
                maxValue.rx() = minValue.x();
                maxValue.ry() = yMap.transform(block.maxY);
            }
            else {
                double p1x = 0.0, p2x = 0.0, p1y = 0.0, p2y = 0.0;

                if(block.minX < block.maxX) {
                    //rising function, push points in direct order
                    p1x = xMap.transform(block.minX);
                    p2x = xMap.transform(block.maxX);
                    p1y = yMap.transform(block.minY);
                    p2y = yMap.transform(block.maxY);
                }
                else {
                    //falling function, push points in reverse order
                    p2x = xMap.transform(block.minX);
                    p1x = xMap.transform(block.maxX);
                    p2y = yMap.transform(block.minY);
                    p1y = yMap.transform(block.maxY);
                }
                points[pixel*2+0].setX(p1x);
                points[pixel*2+0].setY(p1y);
                points[pixel*2+1].setX(p2x);
                points[pixel*2+1].setY(p2y);
            }
        }
        oldFrom = from;
        oldTo = to;
        cashedPolyline = polyline;
        return cashedPolyline;
    }

    bool simplified = false;
    bool polygon = false;
    QPolygonF cashedPolyline;
    int oldFrom = 0;
    int oldTo = 0;
};

LineCurve::LineCurve(const QString &title, Channel *channel) :  QwtPlotCurve(title),
    Curve(title, channel)
{DD;
    setPaintAttribute(QwtPlotCurve::ClipPolygons);
    setPaintAttribute(QwtPlotCurve::FilterPoints);
    setRenderHint(QwtPlotItem::RenderAntialiased);

    setLegendIconSize(QSize(16,8));

    dfddata = new DfdData(this->channel->data());
    setData(dfddata);

    mapper = new FilterPointMapper(channel->type()==Descriptor::TimeResponse);
    // set filter points to true
    const bool noDuplicates = true;
    mapper->setFlag(QwtPointMapper::WeedOutIntermediatePoints, noDuplicates);
}

LineCurve::~LineCurve()
{
    delete mapper;
}

void LineCurve::drawLines(QPainter *painter,
                      const QwtScaleMap &xMap, const QwtScaleMap &yMap,
                      const QRectF &canvasRect,
                      int from, int to) const
{DD;
    //reevaluating from, to
    evaluateScale(from, to, xMap);

    if (from > to) return;

    const bool doAlign = QwtPainter::roundingAlignment( painter );

    QRectF clipRect;
    painter->save();

    //clip polygons
    qreal pw = qMax(qreal(1.0), painter->pen().widthF());
    clipRect = canvasRect.adjusted(-pw, -pw, pw, pw);

    mapper->setFlag(QwtPointMapper::RoundPoints, doAlign);
    mapper->setBoundingRect(canvasRect);

    const bool close = mapper->simplified && channel->type()==Descriptor::TimeResponse;
    QPolygonF polyline = mapper->getPolygon(xMap, yMap, dfddata, from, to);
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

QPointF LineCurve::samplePoint(int point) const
{
    return QwtPlotCurve::sample(point);
}

void LineCurve::attachTo(QwtPlot *plot)
{
    QwtPlotCurve::attach(plot);
}

QString LineCurve::title() const
{
    return QwtPlotCurve::title().text();
}

void LineCurve::setTitle(const QString &title)
{
    QwtPlotCurve::setTitle(title);
}

QwtAxisId LineCurve::yAxis() const
{
    return QwtPlotCurve::yAxis();
}

void LineCurve::setYAxis(QwtAxisId axis)
{
    QwtPlotCurve::setYAxis(axis);
    foreach (PointLabel *l, labels)
        l->setYAxis(axis);
}

QwtAxisId LineCurve::xAxis() const
{
    return QwtPlotCurve::xAxis();
}

void LineCurve::setXAxis(QwtAxisId axis)
{
    QwtPlotCurve::setXAxis(axis);
    foreach (PointLabel *l, labels)
        l->setXAxis(axis);
}

QPen LineCurve::pen() const
{
    return QwtPlotCurve::pen();
}

void LineCurve::setPen(const QPen &pen)
{
    QwtPlotCurve::setPen(pen);
}

QList<QwtLegendData> LineCurve::legendData() const
{
    QList<QwtLegendData> result = QwtPlotCurve::legendData();
    QwtLegendData &data = result[0];
    data.setValue(QwtLegendData::UserRole+3, pen().color());
    data.setValue(QwtLegendData::TitleRole, title());
    if (duplicate)
        data.setValue(QwtLegendData::UserRole+1, fileNumber);
    data.setValue(QwtLegendData::UserRole+2, highlighted);
    data.setValue(QwtLegendData::UserRole+4, fixed);

    return result;
}

void LineCurve::highlight()
{
    Curve::highlight();
    setZ(1000);
    plot()->updateLegend(this);
}

void LineCurve::resetHighlighting()
{
    Curve::resetHighlighting();
    setZ(20);
    plot()->updateLegend(this);
}


/** DfdData implementation */

DfdData::DfdData(DataHolder *data) : data(data)
{

}

QRectF DfdData::boundingRect() const
{
    if ( d_boundingRect.width() < 0 ) {
        d_boundingRect.setLeft( data->xMin() );
        d_boundingRect.setRight( data->xMax() );
        d_boundingRect.setTop( data->yMin() );
        d_boundingRect.setBottom( data->yMax() );
    }

    return d_boundingRect;
}

size_t DfdData::size() const
{
    return data->samplesCount();
}

QPointF DfdData::sample(size_t i) const
{
    return QPointF(data->xValue(i), data->yValue(i));
}

double DfdData::xStep() const
{
    return data->xStep();
}

double DfdData::xBegin() const
{
    return data->xMin();
}


int LineCurve::closest(const QPoint &pos, double *dist) const
{
    int index = -1;

    const size_t numSamples = channel->samplesCount();
    if ( numSamples <= 0 )
        return -1;

    const QwtScaleMap xMap = plot()->canvasMap( xAxis() );
    const QwtScaleMap yMap = plot()->canvasMap( yAxis() );

    int from = 0;
    int to = numSamples-1;
    evaluateScale(from, to, xMap);


    double dmin = 1.0e10;

    for ( int i = from; i <= to; i++ ) {
        const QPointF sample = samplePoint( i );

        const double cx = xMap.transform( sample.x() ) - pos.x();
        const double cy = yMap.transform( sample.y() ) - pos.y();

        const double f = cx*cx + cy*cy;
        if ( f < dmin ) {
            index = i;
            dmin = f;
        }
    }
    if ( dist )
        *dist = qSqrt( dmin );

    return index;
}
