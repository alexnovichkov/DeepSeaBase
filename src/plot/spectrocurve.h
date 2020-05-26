#ifndef SPECTROCURVE_H
#define SPECTROCURVE_H

#include <QObject>
#include <qwt_plot_spectrogram.h>
#include "curve.h"
#include "dataholder.h"
#include <qwt_color_map.h>

class LinearColorMap: public QwtLinearColorMap
{
public:
    LinearColorMap():
        QwtLinearColorMap( Qt::black, Qt::white )
    {
        setFormat(QwtColorMap::RGB);

//        addColorStop( 0.1, Qt::cyan );
//        addColorStop( 0.6, Qt::green );
//        addColorStop( 0.95, Qt::yellow );
    }
};

class RGBColorMap: public QwtLinearColorMap
{
public:
    RGBColorMap():
        QwtLinearColorMap( QColor(2,0,174), QColor(217,53,43) )
    {
        setFormat(QwtColorMap::RGB);

        addColorStop( 0.0625, QColor(0,30,255));
        addColorStop( 0.125,  QColor(12,126,251));
        addColorStop( 0.1875, QColor(2,165,242));
        addColorStop( 0.25,   QColor(0,239,247));
        addColorStop( 0.3125, QColor(128,226,250));
        addColorStop( 0.375,  QColor(0,187,126));
        addColorStop( 0.4375, QColor(0,187,0));
        addColorStop( 0.5,    QColor(26,246,36));
        addColorStop( 0.5625, QColor(183,255,0));
        addColorStop( 0.625,  QColor(254,255,0));
        addColorStop( 0.6875, QColor(254,213,86));
        addColorStop( 0.75,   QColor(222,145,27));
        addColorStop( 0.8125, QColor(252,165,0));
        addColorStop( 0.875,  QColor(254,110,0));
    }
};

class HueColorMap: public QwtHueColorMap
{
public:
    HueColorMap() : QwtHueColorMap(QwtColorMap::RGB)
    {
        //setHueInterval( 240, 60 );
        //setHueInterval( 240, 420 );
        setHueInterval(0, 300);
        setSaturation(255);
        setValue(255);
    }
};

class SpectrogramData: public QwtRasterData
{
public:
    SpectrogramData(DataHolder *data) : m_data(data)
    {
        // some minor performance improvements when the spectrogram item
        // does not need to check for NaN values

        setAttribute( QwtRasterData::WithoutGaps, true );

        m_intervals[ Qt::XAxis ] = QwtInterval(m_data->xMin(), m_data->xMax());
        m_intervals[ Qt::YAxis ] = QwtInterval(m_data->zMin(), m_data->zMax());
        m_intervals[ Qt::ZAxis ] = QwtInterval(m_data->yMin(), m_data->yMax());
    }

    virtual QwtInterval interval( Qt::Axis axis ) const QWT_OVERRIDE
    {
        if ( axis >= 0 && axis <= 2 )
            return m_intervals[ axis ];

        return QwtInterval();
    }

    void setInterval(Qt::Axis axis, const QwtInterval &interval)
    {
        if ( axis >= 0 && axis <= 2 )
            m_intervals[ axis ] = interval;
    }

    virtual double value( double x, double y ) const QWT_OVERRIDE
    {
        // рисуем блоками - ищем ближайшее значение по Х и по Z
        int i = 0;
        int j = 0;

        if (m_data->xValuesFormat() == DataHolder::XValuesUniform) {
            i = qRound((x-m_data->xMin())/m_data->xStep());
        }

        if (m_data->zValuesFormat() == DataHolder::XValuesUniform) {
            j = qRound((y-m_data->zMin())/m_data->zStep());
        }

        if (i<0) i = 0;
        if (i>=m_data->samplesCount()) i = m_data->samplesCount()-1;

        if (j<0) j = 0;
        if (j>=m_data->blocksCount()) j = m_data->blocksCount()-1;

        return m_data->yValue(i, j);
    }

private:
    QwtInterval m_intervals[3];
    DataHolder *m_data;
};

class SpectroCurve : public QwtPlotSpectrogram, public Curve
{
public:
    SpectroCurve(const QString &title, FileDescriptor *descriptor, int channelIndex);
private:
    SpectrogramData *spectroData;

    // Curve interface
public:
    virtual void attachTo(QwtPlot *plot) override;
    virtual QString title() const override;
    virtual void setTitle(const QString &title) override;
    virtual QwtAxisId yAxis() const override;
    virtual void setYAxis(QwtAxisId axis) override;
    virtual QwtAxisId xAxis() const override;
    virtual void setXAxis(QwtAxisId axis) override;
    virtual QPen pen() const override;
    virtual void setPen(const QPen &pen) override;
    virtual QList<QwtLegendData> legendData() const override;
    virtual QPointF samplePoint(int point) const override;
    virtual int closest(const QPoint &pos, double *dist) const override;

    void setColorInterval(double min, double max);
};

#endif // SPECTROCURVE_H
