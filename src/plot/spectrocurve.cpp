#include "spectrocurve.h"
#include "dataholder.h"
#include "qwt_text.h"
#include "pointlabel.h"
#include "qwt_legend_data.h"
#include "qwt_plot.h"
#include "qwt_scale_map.h"
#include "fileformats/filedescriptor.h"

SpectroCurve::SpectroCurve(const QString &title, Channel *channel)
    :  QwtPlotSpectrogram(title), Curve(title, channel)
{
    type = Type::Spectrogram;
    setLegendIconSize(QSize(16,8));

    setRenderThreadCount(0); // use system specific thread count
    setCachePolicy(QwtPlotRasterItem::PaintCache);
    setDisplayMode(QwtPlotSpectrogram::ImageMode, true);
//    setDisplayMode(QwtPlotSpectrogram::ContourMode, true);

    spectroData = new SpectrogramData(this->channel->data());
    setData(spectroData);
}

void SpectroCurve::attachTo(QwtPlot *plot)
{
    QwtPlotSpectrogram::attach(plot);
}

QString SpectroCurve::title() const
{
    return QwtPlotSpectrogram::title().text();
}

void SpectroCurve::setTitle(const QString &title)
{
    QwtPlotSpectrogram::setTitle(title);
}

QwtAxisId SpectroCurve::yAxis() const
{
    return QwtPlotSpectrogram::yAxis();
}

void SpectroCurve::setYAxis(QwtAxisId axis)
{
    QwtPlotSpectrogram::setYAxis(axis);
    foreach (PointLabel *l, labels)
        l->setYAxis(axis);
}

QwtAxisId SpectroCurve::xAxis() const
{
    return QwtPlotSpectrogram::xAxis();
}

void SpectroCurve::setXAxis(QwtAxisId axis)
{
    QwtPlotSpectrogram::setXAxis(axis);
    foreach (PointLabel *l, labels)
        l->setXAxis(axis);
}

QPen SpectroCurve::pen() const
{
    return QPen();
}

//void SpectroCurve::setPen(const QPen &pen)
//{
//    Curve::setPen(pen);
//}

QList<QwtLegendData> SpectroCurve::legendData() const
{
    QList<QwtLegendData> result = QwtPlotSpectrogram::legendData();
    QwtLegendData &data = result[0];
    data.setValues(commonLegendData());
    return result;
}

QPointF SpectroCurve::samplePoint(int) const
{
    return QPointF();
    //return spectroData->samplePoint(point);
}

int SpectroCurve::closest(const QPoint &, double *dist1, double *dist2) const
{
    Q_UNUSED(dist1);
    Q_UNUSED(dist2);

    int index = -1;

//    const size_t numSamples = channel->samplesCount();
//    if ( numSamples <= 0 )
//        return -1;

//    const QwtScaleMap xMap = plot()->canvasMap( xAxis() );
//    const QwtScaleMap yMap = plot()->canvasMap( yAxis() );

//    int from = 0;
//    int to = numSamples-1;
//    evaluateScale(from, to, xMap);


//    double dmin = 1.0e10;

//    for ( int i = from; i <= to; i++ ) {
//        const QPointF sample = samplePoint( i );

//        const double cx = xMap.transform( sample.x() ) - pos.x();
//        const double cy = yMap.transform( sample.y() ) - pos.y();

//        const double f = cx*cx + cy*cy;
//        if ( f < dmin ) {
//            index = i;
//            dmin = f;
//        }
//    }
//    if ( dist )
//        *dist = qSqrt( dmin );

    return index;
}

void SpectroCurve::setColorInterval(double min, double max)
{
    spectroData->setInterval(Qt::ZAxis, QwtInterval(min, max));
}

QwtInterval SpectroCurve::colorInterval() const
{
    return spectroData->interval(Qt::ZAxis);
}

//double SpectroCurve::yMin() const
//{
//    return channel->data()->zMin();
//}

//double SpectroCurve::yMax() const
//{
//    return channel->data()->zMax();
//}

SpectrogramData::SpectrogramData(DataHolder *data) : m_data(data)
{
    // some minor performance improvements when the spectrogram item
    // does not need to check for NaN values

    setAttribute( QwtRasterData::WithoutGaps, true );

    m_intervals[ Qt::XAxis ] = QwtInterval(m_data->xMin(), m_data->xMax());
    m_intervals[ Qt::YAxis ] = QwtInterval(m_data->zMin(), m_data->zMax());
    m_intervals[ Qt::ZAxis ] = QwtInterval(m_data->yMin(), m_data->yMax());
}

QwtInterval SpectrogramData::interval(Qt::Axis axis) const
{
    if ( axis >= 0 && axis <= 2 )
        return m_intervals[ axis ];

    return QwtInterval();
}

void SpectrogramData::setInterval(Qt::Axis axis, const QwtInterval &interval)
{
    if ( axis >= 0 && axis <= 2 )
        m_intervals[ axis ] = interval;
}

double SpectrogramData::value(double x, double y) const
{
    int j = m_data->nearestZ(y);
    if (j < 0) j = 0;

    int i = m_data->nearest(x);
    if (i < 0) i = 0;
    return m_data->yValue(i, j);
}
