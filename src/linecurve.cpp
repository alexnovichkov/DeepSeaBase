#include "linecurve.h"

#include "qwt_point_mapper.h"
#include "qwt_scale_map.h"
#include "qwt_painter.h"
#include "qwt_clipper.h"
#include "logging.h"
#include "filedescriptor.h"
#include "qwt_plot.h"

class FilterPointMapper : public QwtPointMapper
{
public:
    FilterPointMapper() : QwtPointMapper()  { }
    QPolygonF getPolygonF( const QwtScaleMap &xMap, const QwtScaleMap &yMap,
            const QwtSeriesData<QPointF> *series, int from, int to ) const
    {
        //number of visible points for current zoom
        int realpoints = to - from + 1;

        //number of pixels
        int pixels = xMap.pDist();
        if (pixels == 0) return QwtPointMapper::toPolygonF(xMap, yMap, series, from, to);

        //we have less than twice more points than screen pixels - no need to use resample
        if (realpoints <= 2 * pixels) {
            return QwtPointMapper::toPolygonF(xMap, yMap, series, from, to);
        }

        int numberOfPlotPoints = pixels*2;

        //array that will be used to store calculated plot points in screen coordinates
        QPolygonF polyline(numberOfPlotPoints);
        QPointF *points = polyline.data();

        /*
            iterate through pixels - need to draw vertical line
            corresponding to value range change for current pixel
        */
        int pointCount = to - from + 1;
        if ( pointCount <= 0.0 )
            return QwtPointMapper::toPolygonF(xMap, yMap, series, from, to);

        //iterate over pixels
        int start = from;

        int startPixel = qRound( xMap.transform( series->sample(start).x() ) );

        for(int pixel=0;pixel<pixels;++pixel) {
            //now find range [min;max] for current pixel
            //using search algorithm for comparison optimization (3n/2 instead of 2n)
            double minY = 0.0;
            double maxY = 0.0;
            double minX = 0.0;
            double maxX = 0.0;

            if(series->sample(start).y() < series->sample(start + 1).y()) {
                minY = series->sample(start).y();
                maxY = series->sample(start+1).y();
                minX = series->sample(start).x();
                maxX = series->sample(start+1).x();
            }
            else {
                minY = series->sample(start+1).y();
                maxY = series->sample(start).y();
                minX = series->sample(start+1).x();
                maxX = series->sample(start).x();
            }

            int end = (((double)pixel+1.0)/pixels)*pointCount + from;
            if(end>to) end = to;

            // finding end
            for(int k=start+1; ; ++k) {
                if (k > to) break;
                int endPixel = qRound( xMap.transform( series->sample(k).x() ) );
                if (endPixel != startPixel) break;
                else end = k;
            }

            //compare pairs
            for(int k=start+2; k<end; k+=2) {
                if(series->sample(k).y() > series->sample(k+1).y()) {
                    if(series->sample(k).y() > maxY) {
                        maxY = series->sample(k).y();
                        maxX = series->sample(k).x();
                    }

                    if(series->sample(k+1).y()<minY) {
                        minY = series->sample(k+1).y();
                        minX = series->sample(k+1).x();
                    }
                }
                else {
                    if(series->sample(k+1).y()>maxY) {
                        maxY = series->sample(k+1).y();
                        maxX = series->sample(k+1).x();
                    }

                    if(series->sample(k).y()<minY) {
                        minY = series->sample(k).y();
                        minX = series->sample(k).x();
                    }
                }
            }

            //new start for next iteration
            start = end;
            double p1x = 0.0, p2x = 0.0, p1y = 0.0, p2y = 0.0;

            if(minX<maxX) {
                //rising function, push points in direct order
                p1x = xMap.transform(minX);
                p2x = xMap.transform(maxX);
                p1y = yMap.transform(minY);
                p2y = yMap.transform(maxY);
            }
            else {
                //falling function, push points in reverse order
                p2x = xMap.transform(minX);
                p1x = xMap.transform(maxX);
                p2y = yMap.transform(minY);
                p1y = yMap.transform(maxY);
            }

            //add points to array
            points[pixel*2+0].setX(qRound(p1x));
            points[pixel*2+0].setY(qRound(p1y));

            points[pixel*2+1].setX(qRound(p2x));
            points[pixel*2+1].setY(qRound(p2y));
        }

        return polyline;
    }
};

LineCurve::LineCurve(const QString &title, FileDescriptor *descriptor, int channelIndex) :  QwtPlotCurve(title),
    Curve(title, descriptor, channelIndex)
{DD;
    setPaintAttribute(QwtPlotCurve::ClipPolygons);
    setPaintAttribute(QwtPlotCurve::FilterPoints);
    setRenderHint(QwtPlotItem::RenderAntialiased);

    setLegendIconSize(QSize(16,8));

    dfddata = new DfdData(this->channel->data());
    setData(dfddata);
}

//LineCurve::~LineCurve()
//{

//}

void LineCurve::drawLines(QPainter *painter,
                      const QwtScaleMap &xMap, const QwtScaleMap &yMap,
                      const QRectF &canvasRect,
                      int from, int to) const
{
    //reevaluating from, to
    evaluateScale(from, to, xMap);

    if ( from > to )
        return;

    const bool doAlign = QwtPainter::roundingAlignment( painter );

    QRectF clipRect;

    //clip polygons
    qreal pw = qMax( qreal( 1.0 ), painter->pen().widthF());
    clipRect = canvasRect.adjusted(-pw, -pw, pw, pw);

    // set filter points to true
    const bool noDuplicates = true;

    FilterPointMapper mapper;
    mapper.setFlag( QwtPointMapper::RoundPoints, doAlign );
    mapper.setFlag( QwtPointMapper::WeedOutPoints, noDuplicates );
    mapper.setBoundingRect( canvasRect );

    QPolygonF polyline = mapper.getPolygonF(xMap, yMap, dfddata, from, to);

    // clip polygons
    polyline = QwtClipper::clipPolygonF(clipRect, polyline, false );

    QwtPainter::drawPolyline( painter, polyline );
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

int LineCurve::yAxis() const
{
    return QwtPlotCurve::yAxis();
}

void LineCurve::setYAxis(int axis)
{
    QwtPlotCurve::setYAxis(axis);
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

        const double f = qwtSqr( cx ) + qwtSqr( cy );
        if ( f < dmin ) {
            index = i;
            dmin = f;
        }
    }
    if ( dist )
        *dist = qSqrt( dmin );

    return index;
}
