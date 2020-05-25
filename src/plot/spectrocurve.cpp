#include "spectrocurve.h"
#include "dataholder.h"
#include "qwt_text.h"
#include "pointlabel.h"
#include "qwt_legend_data.h"
#include "qwt_plot.h"
#include "qwt_scale_map.h"
#include "fileformats/filedescriptor.h"
#include "qwt_color_map.h"

SpectroCurve::SpectroCurve(const QString &title, FileDescriptor *descriptor, int channelIndex)
    :  QwtPlotSpectrogram(title), Curve(title, descriptor, channelIndex)
{
    setLegendIconSize(QSize(16,8));

    setRenderThreadCount( 0 ); // use system specific thread count
    setCachePolicy( QwtPlotRasterItem::PaintCache );
    setDisplayMode( QwtPlotSpectrogram::ImageMode, true );

    spectroData = new SpectrogramData(this->channel->data());
    setData(spectroData);

    auto color = new QwtLinearColorMap(/*Qt::darkCyan, Qt::red*/);
    color->addColorStop( 0.1, Qt::cyan );
    color->addColorStop( 0.6, Qt::green );
    color->addColorStop( 0.95, Qt::yellow );

    setColorMap(color);


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

QPointF SpectroCurve::samplePoint(int point) const
{
    return QPointF();
    //return spectroData->samplePoint(point);
}

int SpectroCurve::closest(const QPoint &pos, double *dist) const
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
