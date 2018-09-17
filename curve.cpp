#include "curve.h"
#include "qwt_symbol.h"
#include "pointlabel.h"

#include "filedescriptor.h"
#include <qwt_curve_fitter.h>
#include "logging.h"
#include "psimpl.h"

//class DfdData: public QwtSeriesData<QPointF>
//{
//public:
//    DfdData(double x0, double xStep, const double *y, size_t size) :
//        d_x0(x0), d_xStep(xStep), d_y(y), d_x(0), d_size(size)
//    {}
//    DfdData(const double *x, const double *y, size_t size) :
//        d_x0(x[0]), d_xStep(x[1]-x[0]), d_y(y), d_x(x), d_size(size)
//    {}

//    virtual QRectF boundingRect() const
//    {
//        if ( d_boundingRect.width() < 0 )
//            d_boundingRect = qwtBoundingRect( *this );

//        return d_boundingRect;
//    }

//    virtual size_t size() const
//    {
//        return d_size;
//    }

//    virtual QPointF sample( size_t i ) const
//    {
//        if (d_x) return QPointF(d_x[int(i)], d_y[int(i)]);
//        return QPointF( d_x0 + i * d_xStep, d_y[int( i )] );
//    }
//private:
//    double d_x0;
//    double d_xStep;
//    const double *d_y;
//    const double *d_x;
//    size_t d_size;
//};

Curve::Curve(const QwtText &title) :
    QwtPlotCurve(title), descriptor(0), channelIndex(-1), channel(0), m_simplified(false),
    xMin(0), xMax(0), yMin(0), yMax(0), samplesCount(0)
{DD;
    setPaintAttribute(QwtPlotCurve::ClipPolygons);
    setPaintAttribute(QwtPlotCurve::FilterPoints);
    setRenderHint(QwtPlotItem::RenderAntialiased);


    //setCurveAttribute(Fitted, true);
    //auto fitter = new QwtWeedingCurveFitter();
    //fitter->setChunkSize(20);
    //setCurveFitter(fitter);

    setLegendIconSize(QSize(16,8));
}

Curve::Curve(const QString &title) : Curve(QwtText(title))
{DD;

}

Curve::Curve(const QString &title, FileDescriptor *descriptor, int channelIndex) :
    Curve(title)
{DD;
    this->descriptor = descriptor;
    this->channelIndex = channelIndex;
    channel = descriptor->channel(channelIndex);

    setRawSamples();
}





Curve::~Curve()
{DD;
    foreach(PointLabel *l, labels) l->detach();
    qDeleteAll(labels);
}

void Curve::setRawSamples()
{
    if (!descriptor || !channel) return;

    if (channel->xValues().isEmpty())
        setRawSamples(channel->xBegin(), descriptor->xStep(), channel->yValues().data(), channel->samplesCount());
    else
        setRawSamples(channel->xValues().data(), channel->yValues().data(), channel->samplesCount());
}

void Curve::setRawSamples(double x0, double xStep, const double *yData, int size)
{DD;
    if (size > 1048576) {
        const int factor = 10;
        // try to filter out original data

        xDataSimplified.clear();
        yDataSimplified.clear();
        QVector<double> coords(size*2);
        for (int i=0; i<size; ++i) {
            coords[i*2] = x0+i*xStep;
            coords[i*2+1] = yData[i];
        } //qDebug()<<"coords"<<coords.mid(0,40);
        QVector<double> simplified(size / factor * 2); qDebug()<<"simplified size"<<simplified.size();

        typedef QVector<double>::iterator it;
        psimpl::simplify_douglas_peucker_n<2, it, it>
                (coords.begin(), coords.end(), size / factor, simplified.begin());

        //retrieving x data and y data
        for (int i=0; i < simplified.size()-1; i+=2) {
            xDataSimplified << simplified[i];
            yDataSimplified << simplified[i+1];
        }

        // freeing unused memory
        xDataSimplified.squeeze();
        yDataSimplified.squeeze();

        setData(new DfdData(xDataSimplified.data(), yDataSimplified.data(), xDataSimplified.size()));
        m_simplified = true;
        xMin = xDataSimplified.first();
        xMax = xDataSimplified.last();
        auto minmax = std::minmax_element(yDataSimplified.begin(), yDataSimplified.end());
        yMin = *(minmax.first);
        yMax = *(minmax.second);
        samplesCount = xDataSimplified.size();
    }
    else {
        setData(new DfdData(x0, xStep, yData, size));
        xMin = x0;
        xMax = x0 + xStep*size;
        auto minmax = std::minmax_element(yData, yData+size);
        yMin = *(minmax.first);
        yMax = *(minmax.second);
        samplesCount = size;
    }
}

void Curve::setRawSamples(const double *xData, const double *yData, int size)
{DD;
    setData(new DfdData(xData, yData, size));

    auto minmax = std::minmax_element(xData, xData+size);
    xMin = *(minmax.first);
    xMax = *(minmax.second);
    minmax = std::minmax_element(yData, yData+size);
    yMin = *(minmax.first);
    yMax = *(minmax.second);
    samplesCount = size;
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

bool Curve::isSimplified() const
{
    return m_simplified;
}





//QwtArrayPlotItem::QwtArrayPlotItem(const QwtText &title):
//    QwtPlotItem(title), m_dt(1.0), m_size(0), m_data(0), m_plotColor(Qt::red)
//{
//    setItemAttribute(QwtPlotItem::AutoScale, true);
//    setRenderHint(QwtPlotItem::RenderAntialiased,true);
//}

//QwtArrayPlotItem::~QwtArrayPlotItem()
//{

//}

//void QwtArrayPlotItem::draw( QPainter *painter,
//        const QwtScaleMap &xMap, const QwtScaleMap &yMap,
//        const QRectF &canvasRect ) const
//{
//    //some data checks first
//    if(!m_data)
//        return;

//    if(m_size<2)
//        return;

//    if(qFuzzyCompare(m_dt,0.0))
//        return;


//    //array that will be used to store calculated plot points in screen coordinates
//    QPointF* points=0;
//    //number of points in array (will be calculated later)
//    quint32 numberOfPlotPoints = 0;

//    //number of visible points for current zoom
//    quint32 realpoints = xMap.sDist()/m_dt;
//    if(realpoints>m_size) //wrong axes, just use standart value then
//        realpoints = m_size;


//    //number of pixels
//    int pixels = xMap.pDist();
//    if(pixels == 0)
//        return;

//    if(realpoints>2*pixels) //we have twice more points then screen pixels - need to use resample
//    {
//        /*
//            iterate through pixels - need to draw vertical line
//            corresponding to value range change for current pixel
//        */

//        //only use points from visible range
//        double startPoint = xMap.s1()/m_dt;
//        if(startPoint>m_size)
//            startPoint = m_size;
//        else if(startPoint<0)
//            startPoint = 0;

//        double endPoint = xMap.s2()/m_dt;
//        if(endPoint>m_size)
//            endPoint = m_size;
//        else if(endPoint<0)
//            endPoint = 0;

//        double pointSize = endPoint - startPoint;
//        if ( pointSize <= 0.0 )
//            return;


//        //allocate memory
//        numberOfPlotPoints = pixels*2;
//        points = new QPointF[numberOfPlotPoints];

//        //iterate over pixels
//        int start = startPoint;
//        for(int pixel=0;pixel<pixels;++pixel)
//        {

//            int end = (((double)pixel+1.0)/pixels)*pointSize + startPoint;
//            if(end>endPoint)
//                end = endPoint;

//            //now find range [min;max] for current pixel

//            //using search algorithm for comparison optimization (3n/2 instead of 2n)
//            double min = 0.0;
//            double max = 0.0;
//            int minIndex = 0;
//            int maxIndex = 0;

//            if(m_data[start]<m_data[start+1])
//            {
//                min = m_data[start];
//                max = m_data[start+1];
//                minIndex = start;
//                maxIndex = start+1;
//            }
//            else
//            {
//                min = m_data[start+1];
//                max = m_data[start];
//                minIndex = start+1;
//                maxIndex = start;
//            }

//            //compare pairs
//            for(int k=start+2;k<end-2;k+=2)
//            {
//                if(m_data[k]>m_data[k+1])
//                {
//                    if(m_data[k]>max)
//                    {
//                        max = m_data[k];
//                        maxIndex = k;
//                    }

//                    if(m_data[k+1]<min)
//                    {
//                        min = m_data[k+1];
//                        minIndex = k+1;

//                    }
//                }
//                else
//                {
//                    if(m_data[k+1]>max)
//                    {
//                        max = m_data[k+1];
//                        maxIndex = k+1;

//                    }

//                    if(m_data[k]<min)
//                    {
//                        min = m_data[k];
//                        minIndex = k;
//                    }
//                }
//            }

//            //new start for next iteration
//            start = end;
//            double p1x = 0.0, p2x = 0.0, p1y = 0.0, p2y = 0.0;

//            if(minIndex<maxIndex)
//            {
//                //rising function, push points in direct order
//                p1x = xMap.transform(minIndex*m_dt);
//                p2x = xMap.transform(maxIndex*m_dt);
//                p1y = yMap.transform( min );
//                p2y = yMap.transform( max );
//            }
//            else
//            {
//                //falling function, push points in reverse order
//                p2x = xMap.transform(minIndex*m_dt);
//                p1x = xMap.transform(maxIndex*m_dt);
//                p2y = yMap.transform( min );
//                p1y = yMap.transform( max );
//            }

//            //add points to array
//            points[pixel*2+0].setX(p1x);
//            points[pixel*2+0].setY(p1y);

//            points[pixel*2+1].setX(p2x);
//            points[pixel*2+1].setY(p2y);
//        }
//    }
//    else	//normal draw, not using resample
//    {

//        //only use points from visible range
//        quint32 startPoint = xMap.s1()/m_dt;

//        if(startPoint>m_size)
//            startPoint = m_size;

//        int endPoint = xMap.s2()/m_dt;
//        endPoint+=2;
//        if(endPoint>m_size)
//            endPoint = m_size;

//        int pointSize = endPoint - startPoint;
//        if ( pointSize <= 0 )
//            return;

//        //allocate array for points
//        numberOfPlotPoints = pointSize;
//        points =  new QPointF[numberOfPlotPoints];

//        for ( int i = startPoint; i < endPoint; i++ )
//        {
//            double x = xMap.transform( i*m_dt );
//            double y = yMap.transform( m_data[i]);

//            points[i - startPoint].setX(x);
//            points[i - startPoint].setY(y);
//        }
//    }


//    //draw plot
//    painter->setPen(m_plotColor);
//    painter->drawPolyline(points, numberOfPlotPoints);

//    //free memory
//    delete[] points;
//}

//void QwtArrayPlotItem::setData(double* data, quint32 size, qreal dt)
//{
//    if(!data)
//    {
//        qCritical()<<"QwtArrayPlotItem::setData: data == 0!";
//        return;
//    }
//    if(size<2)
//    {
//        qCritical()<<"QwtArrayPlotItem::setData: wrong size!";
//        return;
//    }
//    if(qFuzzyCompare(dt,0.0))
//    {
//        qCritical()<<"QwtArrayPlotItem::setData: dt == 0.0!";
//        return;
//    }
//    m_dt = dt;
//    m_size = size;




//    m_boundingRect = QRectF(0.0, 0.0, -1.0, -1.0);//set invalid to recalculate

//    m_data = data;
//}

//QRectF QwtArrayPlotItem::boundingRect() const
//{
//    //if we have valid rect, return it
//    if( m_boundingRect.isValid())
//        return m_boundingRect;
//    //need to calculate
//    else if(m_data != 0 && m_size>1)
//    {
//        double min =0.0;
//        double max =0.0;


//        if(m_data[0]<m_data[1])
//        {
//            min = m_data[0];
//            max = m_data[1];
//        }
//        else
//        {
//            min = m_data[1];
//            max = m_data[0];
//        }

//        //compare pairs
//        for(int k=2;k<m_size-2;k+=2)
//        {
//            if(m_data[k]>m_data[k+1])
//            {
//                if(m_data[k]>max)
//                {
//                    max = m_data[k];
//                }

//                if(m_data[k+1]<min)
//                {
//                    min = m_data[k+1];
//                }
//            }
//            else
//            {
//                if(m_data[k+1]>max)
//                {
//                    max = m_data[k+1];
//                }
//                if(m_data[k]<min)
//                {
//                    min = m_data[k];
//                }
//            }
//        }

//        m_boundingRect = QRectF(0.0,(double)min, m_size*m_dt, (double)(max-min));
//        return m_boundingRect;

//    }
//    return  QRectF( 1.0, 1.0, -2.0, -2.0 );
//}

DfdData::DfdData(double x0, double xStep, const double *y, size_t size)
    : d_x0(x0), d_xStep(xStep), d_y(y), d_x(0), d_size(size)
{

}

DfdData::DfdData(const double *x, const double *y, size_t size)
    : d_x0(x[0]), d_xStep(0), d_y(y), d_x(x), d_size(size)
{

}

QRectF DfdData::boundingRect() const
{
    if ( d_boundingRect.width() < 0 )
        d_boundingRect = qwtBoundingRect( *this );

    return d_boundingRect;
}

size_t DfdData::size() const
{
    return d_size;
}

QPointF DfdData::sample(size_t i) const
{
    if (d_x) return QPointF(d_x[int(i)], d_y[int(i)]);
    return QPointF( d_x0 + i * d_xStep, d_y[int( i )] );
}

double DfdData::xStep() const
{
    return d_xStep;
}

double DfdData::xBegin() const
{
    return d_x0;
}
