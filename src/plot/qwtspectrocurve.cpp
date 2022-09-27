#include "qwtspectrocurve.h"
#include "dataholder.h"
#include "qwt_text.h"
#include "pointlabel.h"
#include "qwt_legend_data.h"
#include "plot.h"
#include "qwt_scale_map.h"
#include "fileformats/filedescriptor.h"
#include "logging.h"
#include "qwtplotimpl.h"

QwtSpectroCurve::QwtSpectroCurve(const QString &title, Channel *channel)
    :  QwtPlotSpectrogram(title), Curve(title, channel)
{DDD;
    type = Type::Spectrogram;
    setLegendIconSize(QSize(16,8));

    setRenderThreadCount(0); // use system specific thread count
    setCachePolicy(QwtPlotRasterItem::PaintCache);
  //  setDisplayMode(QwtPlotSpectrogram::ImageMode, true);
    setDisplayMode(QwtPlotSpectrogram::ContourMode, true);

    spectroData = new SpectrogramData(this->channel->data());
    setData(spectroData);
}

void QwtSpectroCurve::attachTo(QwtPlot *plot)
{DDD;
    QwtPlotSpectrogram::attach(plot);
}

QString QwtSpectroCurve::title() const
{DDD;
    return QwtPlotSpectrogram::title().text();
}

void QwtSpectroCurve::setTitle(const QString &title)
{DDD;
    QwtPlotSpectrogram::setTitle(title);
}

Enums::AxisType QwtSpectroCurve::yAxis() const
{DDD;
    return toAxisType(QwtPlotSpectrogram::yAxis());
}

void QwtSpectroCurve::setYAxis(Enums::AxisType axis)
{DDD;
    QwtPlotSpectrogram::setYAxis(toQwtAxisType(axis));
    foreach (PointLabel *l, labels)
        l->setYAxis(toQwtAxisType(axis));
}

Enums::AxisType QwtSpectroCurve::xAxis() const
{DDD;
    return toAxisType(QwtPlotSpectrogram::xAxis());
}

void QwtSpectroCurve::setXAxis(Enums::AxisType axis)
{DDD;
    QwtPlotSpectrogram::setXAxis(toQwtAxisType(axis));
    foreach (PointLabel *l, labels)
        l->setXAxis(toQwtAxisType(axis));
}

QPen QwtSpectroCurve::pen() const
{DDD;
    return QPen();
}

QList<QwtLegendData> QwtSpectroCurve::legendData() const
{DDD;
    QList<QwtLegendData> result = QwtPlotSpectrogram::legendData();
    QwtLegendData &data = result[0];
    data.setValues(commonLegendData());
    return result;
}

SamplePoint QwtSpectroCurve::samplePoint(SelectedPoint point) const
{DDD;
    return spectroData->samplePoint(point);
}

SelectedPoint QwtSpectroCurve::closest(const QPoint &pos, double *dist1, double *dist2) const
{DDD;
    int indexX = channel->data()->nearest(m_plot->screenToPlotCoordinates(xAxis(), pos.x()));
    int indexZ = channel->data()->nearestZ(m_plot->screenToPlotCoordinates(yAxis(), pos.y()));

    if (dist1 && indexX != -1) *dist1 = qAbs(m_plot->plotToScreenCoordinates(xAxis(), channel->data()->xValue(indexX)) - pos.x());
    if (dist2 && indexZ != -1) *dist2 = qAbs(m_plot->plotToScreenCoordinates(yAxis(), channel->data()->zValue(indexZ)) - pos.y());

    return {indexX, indexZ};
}

void QwtSpectroCurve::setColorInterval(double min, double max)
{DDD;
    spectroData->setInterval(Qt::ZAxis, QwtInterval(min, max));
}

QwtInterval QwtSpectroCurve::colorInterval() const
{DDD;
    return spectroData->interval(Qt::ZAxis);
}




SpectrogramData::SpectrogramData(DataHolder *data) : m_data(data)
{DDD;
    // some minor performance improvements when the spectrogram item
    // does not need to check for NaN values

    setAttribute( QwtRasterData::WithoutGaps, false );

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
    const QwtInterval xInterval = interval( Qt::XAxis );
    const QwtInterval yInterval = interval( Qt::YAxis );

    if ( !( xInterval.contains(x) && yInterval.contains(y) ) )
        return qQNaN();

    int j = m_data->nearestZ(y);
    if (j < 0) return qQNaN();

    int i = m_data->nearest(x);
    if (i < 0) return qQNaN();

    return m_data->yValue(i, j);
}

SamplePoint SpectrogramData::samplePoint(SelectedPoint point) const
{
    return {m_data->xValue(point.x), m_data->yValue(point.x, point.z), m_data->zValue(point.z)};
}
