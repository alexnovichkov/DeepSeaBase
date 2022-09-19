#include "spectrocurve.h"
#include "dataholder.h"
#include "qwt_text.h"
#include "pointlabel.h"
#include "qwt_legend_data.h"
#include "qwt_plot.h"
#include "qwt_scale_map.h"
#include "fileformats/filedescriptor.h"
#include "logging.h"

SpectroCurve::SpectroCurve(const QString &title, Channel *channel)
    :  QwtPlotSpectrogram(title), Curve(title, channel)
{DDD;
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
{DDD;
    QwtPlotSpectrogram::attach(plot);
}

QString SpectroCurve::title() const
{DDD;
    return QwtPlotSpectrogram::title().text();
}

void SpectroCurve::setTitle(const QString &title)
{DDD;
    QwtPlotSpectrogram::setTitle(title);
}

QwtAxisId SpectroCurve::yAxis() const
{DDD;
    return QwtPlotSpectrogram::yAxis();
}

void SpectroCurve::setYAxis(QwtAxisId axis)
{DDD;
    QwtPlotSpectrogram::setYAxis(axis);
    foreach (PointLabel *l, labels)
        l->setYAxis(axis);
}

QwtAxisId SpectroCurve::xAxis() const
{DDD;
    return QwtPlotSpectrogram::xAxis();
}

void SpectroCurve::setXAxis(QwtAxisId axis)
{DDD;
    QwtPlotSpectrogram::setXAxis(axis);
    foreach (PointLabel *l, labels)
        l->setXAxis(axis);
}

QPen SpectroCurve::pen() const
{DDD;
    return QPen();
}

QList<QwtLegendData> SpectroCurve::legendData() const
{DDD;
    QList<QwtLegendData> result = QwtPlotSpectrogram::legendData();
    QwtLegendData &data = result[0];
    data.setValues(commonLegendData());
    return result;
}

SamplePoint SpectroCurve::samplePoint(SelectedPoint point) const
{DDD;
    return spectroData->samplePoint(point);
}

SelectedPoint SpectroCurve::closest(const QPoint &pos, double *dist1, double *dist2) const
{DDD;
    Q_UNUSED(dist1);
    Q_UNUSED(dist2);

    const QwtScaleMap xMap = plot()->canvasMap( xAxis() );
    const QwtScaleMap yMap = plot()->canvasMap( yAxis() );

    int indexX = channel->data()->nearest(xMap.invTransform(pos.x()));
    int indexZ = channel->data()->nearestZ(yMap.invTransform(pos.y()));

    if (dist1 && indexX != -1) *dist1 = qAbs(xMap.transform(channel->data()->xValue(indexX)) - pos.x());
    if (dist2 && indexZ != -1) *dist2 = qAbs(yMap.transform(channel->data()->zValue(indexZ)) - pos.y());

    return {indexX, indexZ};
}

void SpectroCurve::setColorInterval(double min, double max)
{DDD;
    spectroData->setInterval(Qt::ZAxis, QwtInterval(min, max));
}

QwtInterval SpectroCurve::colorInterval() const
{DDD;
    return spectroData->interval(Qt::ZAxis);
}




SpectrogramData::SpectrogramData(DataHolder *data) : m_data(data)
{DDD;
    // some minor performance improvements when the spectrogram item
    // does not need to check for NaN values

    setAttribute( QwtRasterData::WithoutGaps, true );

    m_intervals[ Qt::XAxis ] = QwtInterval(m_data->xMin(), m_data->xMax());
    m_intervals[ Qt::YAxis ] = QwtInterval(m_data->zMin(), m_data->zMax());
    m_intervals[ Qt::ZAxis ] = QwtInterval(m_data->yMin(), m_data->yMax());
}

QwtInterval SpectrogramData::interval(Qt::Axis axis) const
{DDD;
    if ( axis >= 0 && axis <= 2 )
        return m_intervals[ axis ];

    return QwtInterval();
}

void SpectrogramData::setInterval(Qt::Axis axis, const QwtInterval &interval)
{DDD;
    if ( axis >= 0 && axis <= 2 )
        m_intervals[ axis ] = interval;
}

double SpectrogramData::value(double x, double y) const
{DDD;
    int j = m_data->nearestZ(y);
    if (j < 0) j = 0;

    int i = m_data->nearest(x);
    if (i < 0) i = 0;
    return m_data->yValue(i, j);
}

SamplePoint SpectrogramData::samplePoint(SelectedPoint point) const
{
    return {m_data->xValue(point.x), m_data->yValue(point.x, point.z), m_data->zValue(point.z)};
}
