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
    setLegendIconSize(QSize(16,8));

    setRenderThreadCount(0); // use system specific thread count
    setCachePolicy(QwtPlotRasterItem::PaintCache);
    setDisplayMode(QwtPlotSpectrogram::ImageMode, true);
    setDisplayMode(QwtPlotSpectrogram::ContourMode, true);

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

void SpectroCurve::setPen(const QPen &pen)
{
    Q_UNUSED(pen);
}

QList<QwtLegendData> SpectroCurve::legendData() const
{
    QList<QwtLegendData> result = QwtPlotSpectrogram::legendData();
    QwtLegendData &data = result[0];
    data.setValue(QwtLegendData::UserRole+3, pen().color());
    data.setValue(QwtLegendData::TitleRole, title());
    if (duplicate)
        data.setValue(QwtLegendData::UserRole+1, fileNumber);
    data.setValue(QwtLegendData::UserRole+2, highlighted);
    data.setValue(QwtLegendData::UserRole+4, fixed);

    return result;
}

QPointF SpectroCurve::samplePoint(int) const
{
    return QPointF();
    //return spectroData->samplePoint(point);
}

int SpectroCurve::closest(const QPoint &, double *) const
{
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
