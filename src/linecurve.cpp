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
    FilterPointMapper() : QwtPointMapper()  { }
    QPolygonF getPolygonF( const QwtScaleMap &xMap, const QwtScaleMap &yMap,
            const QwtSeriesData<QPointF> *series, int from, int to )
    {
        //number of visible points for current zoom
        const int pointCount = to - from + 1;
        if (pointCount <= 0)
            return QwtPointMapper::toPolygonF(xMap, yMap, series, from, to);

        //number of pixels
        //const int pixels = xMap.pDist();
        const int pixels = int(xMap.transform(series->sample(to).x()) - xMap.transform(series->sample(from).x()));
        if (pixels == 0)
            return QwtPointMapper::toPolygonF(xMap, yMap, series, from, to);

        //for each pixel two points - line begin and line end
        const int numberOfPlotPoints = pixels*2;

        //we have less than 5* more points than screen pixels - no need to use resample
        if (pointCount <= pixels*5) {
            return QwtPointMapper::toPolygonF(xMap, yMap, series, from, to);
        }
        const int width = pointCount/pixels;
//        qDebug()<<"pointCount"<<pointCount<<"pixels"<<pixels<<"width"<<width;



        //array that will be used to store calculated plot points in screen coordinates
        QPolygonF polyline(numberOfPlotPoints);
        QPointF *points = polyline.data();


        //iterate over pixels
        int start = from;
        PointBlock block;
        int end;

        for (int pixel=0; pixel<pixels; ++pixel) {
            if (end==to) {
//                qDebug()<<"pixel"<<pixel;
                break;
            }
            end = qRound(width * ((double)pixel + 1.0) + (double)from);
            if (end > to) end = to;

            QPointF sample1 = series->sample(start);
            QPointF sample2 = series->sample(start+1);

            //now find range [min;max] for current pixel
            //using search algorithm for comparison optimization (3n/2 instead of 2n)

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

            //add points to array
            points[pixel*2+0].setX(p1x);
            points[pixel*2+0].setY(p1y);

            points[pixel*2+1].setX(p2x);
            points[pixel*2+1].setY(p2y);
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

    mapper = new FilterPointMapper();
    // set filter points to true
    const bool noDuplicates = true;
    mapper->setFlag( QwtPointMapper::WeedOutPoints, noDuplicates );

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

    if ( from > to )
        return;

    const bool doAlign = QwtPainter::roundingAlignment( painter );

    QRectF clipRect;

    //clip polygons
    qreal pw = qMax( qreal( 1.0 ), painter->pen().widthF());
    clipRect = canvasRect.adjusted(-pw, -pw, pw, pw);


    mapper->setFlag( QwtPointMapper::RoundPoints, doAlign );
    mapper->setBoundingRect( canvasRect );

    QPolygonF polyline = mapper->getPolygonF(xMap, yMap, dfddata, from, to);

    // clip polygons
    QwtClipper::clipPolygonF(clipRect, polyline, false );

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

QwtAxisId LineCurve::yAxis() const
{
    return QwtPlotCurve::yAxis();
}

void LineCurve::setYAxis(QwtAxisId axis)
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
