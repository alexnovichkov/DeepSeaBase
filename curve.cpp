#include "curve.h"
#include "qwt_symbol.h"
#include "pointlabel.h"

#include "filedescriptor.h"
#include <qwt_curve_fitter.h>
#include "logging.h"
#include "psimpl.h"
#include "qwt_scale_map.h"
#include "qwt_painter.h"
#include "qwt_point_mapper.h"
#include "qwt_clipper.h"
#include "dataholder.h"

class FilterPointMapper : public QwtPointMapper
{
public:
    FilterPointMapper() : QwtPointMapper()
    { }
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

        for(int pixel=0;pixel<pixels;++pixel)
        {
            //now find range [min;max] for current pixel

            //using search algorithm for comparison optimization (3n/2 instead of 2n)
            double minY = 0.0;
            double maxY = 0.0;
            double minX = 0.0;
            double maxX = 0.0;

            if(series->sample(start).y() < series->sample(start + 1).y())
            {
                minY = series->sample(start).y();
                maxY = series->sample(start+1).y();
                minX = series->sample(start).x();
                maxX = series->sample(start+1).x();
            }
            else
            {
                minY = series->sample(start+1).y();
                maxY = series->sample(start).y();
                minX = series->sample(start+1).x();
                maxX = series->sample(start).x();
            }

            int end = (((double)pixel+1.0)/pixels)*pointCount + from;
            if(end>to)
                end = to;

            // finding end
            for(int k=start+1; ; ++k) {
                if (k > to) break;
                int endPixel = qRound( xMap.transform( series->sample(k).x() ) );
                if (endPixel != startPixel) break;
                else end = k;
            }

            //compare pairs
            for(int k=start+2; k<end; k+=2)
            {
                if(series->sample(k).y() > series->sample(k+1).y())
                {
                    if(series->sample(k).y() > maxY)
                    {
                        maxY = series->sample(k).y();
                        maxX = series->sample(k).x();
                    }

                    if(series->sample(k+1).y()<minY)
                    {
                        minY = series->sample(k+1).y();
                        minX = series->sample(k+1).x();

                    }
                }
                else
                {
                    if(series->sample(k+1).y()>maxY)
                    {
                        maxY = series->sample(k+1).y();
                        maxX = series->sample(k+1).x();

                    }

                    if(series->sample(k).y()<minY)
                    {
                        minY = series->sample(k).y();
                        minX = series->sample(k).x();
                    }
                }
            }

            //new start for next iteration
            start = end;
            double p1x = 0.0, p2x = 0.0, p1y = 0.0, p2y = 0.0;

            if(minX<maxX)
            {
                //rising function, push points in direct order
                p1x = xMap.transform(minX);
                p2x = xMap.transform(maxX);
                p1y = yMap.transform(minY);
                p2y = yMap.transform(maxY);
            }
            else
            {
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

Curve::Curve(const QwtText &title) :
    QwtPlotCurve(title), descriptor(0), channelIndex(-1), channel(0)
{DD;
    setPaintAttribute(QwtPlotCurve::ClipPolygons);
    setPaintAttribute(QwtPlotCurve::FilterPoints);
    setRenderHint(QwtPlotItem::RenderAntialiased);
    duplicate = false;
    data = 0;

    //setCurveAttribute(Fitted, true);
    //auto fitter = new QwtWeedingCurveFitter();
    //fitter->setChunkSize(20);
    //setCurveFitter(fitter);

    setLegendIconSize(QSize(16,8));
}

Curve::Curve(const QString &title) : Curve(QwtText(title))
{DD;
    data = 0;
}

Curve::Curve(const QString &title, FileDescriptor *descriptor, int channelIndex) :
    Curve(title)
{DD;
    this->descriptor = descriptor;
    this->channelIndex = channelIndex;
    channel = descriptor->channel(channelIndex);

    data = new DfdData(this->channel->data());
    setData(data);
}

Curve::~Curve()
{DD;
    foreach(PointLabel *l, labels) l->detach();
    qDeleteAll(labels);
}

void Curve::drawLines(QPainter *painter,
                      const QwtScaleMap &xMap, const QwtScaleMap &yMap,
                      const QRectF &canvasRect,
                      int from, int to) const
{
    double startX = xMap.s1();
    double endX = xMap.s2();
    for (int i=0; i<to; ++i) {
        if (data->sample(i).x() >= startX) {
            from = i-1;
            break;
        }
    }
    for (int i=to; i>=from; --i) {
        if (data->sample(i).x() <= endX) {
            to = i+1;
            break;
        }
    }
    if (from < 0) from = 0;
    if (to >= int(data->size())) to = data->size()-1;

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

    QPolygonF polyline = mapper.getPolygonF(xMap, yMap, data, from, to);

    // clip polygons
    polyline = QwtClipper::clipPolygonF(clipRect, polyline, false );

    QwtPainter::drawPolyline( painter, polyline );
}

void Curve::addLabel(PointLabel *label)
{DD;
    labels << label;
}

void Curve::removeLabel(PointLabel *label)
{DD;
    if (labels.contains(label)) {
        labels.removeAll(label);
        label->detach();
        delete label;
    }
}

PointLabel *Curve::findLabel(const QPoint &pos, int yAxis)
{DD;
    foreach (PointLabel *l, labels)
        if (l->contains(pos, yAxis))
            return l;

    return 0;
}

PointLabel *Curve::findLabel(const int point)
{DD;
    foreach (PointLabel *l, labels)
        if (l->point() == point)
            return l;

    return 0;
}

double Curve::yMin() const
{
    return channel->data()->yMin();
}

double Curve::yMax() const
{
    return channel->data()->yMax();
}

double Curve::xMin() const
{
    return channel->data()->xMin();
}

double Curve::xMax() const
{
    return channel->data()->xMax();
}

int Curve::samplesCount() const
{
    return channel->data()->samplesCount();
}

QList<QwtLegendData> Curve::legendData() const
{
    //adding file number to legend data generated by the default implementation
    QList<QwtLegendData> result = QwtPlotCurve::legendData();
    if (duplicate)
        result.first().setValue(QwtLegendData::UserRole+1, this->fileNumber);
    return result;
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
