#ifndef SPECTROCURVE_H
#define SPECTROCURVE_H

#include <QObject>
#include <qwt_plot_spectrogram.h>
#include "curve.h"
#include "dataholder.h"

class SpectrogramData: public QwtRasterData
{
public:
    SpectrogramData(DataHolder *data);
    virtual QwtInterval interval( Qt::Axis axis ) const override;
    void setInterval(Qt::Axis axis, const QwtInterval &interval);
    virtual double value( double x, double y ) const override;
    SamplePoint samplePoint(SelectedPoint point) const;
private:
    QwtInterval m_intervals[3];
    DataHolder *m_data;
};

class SpectroCurve : public QwtPlotSpectrogram, public Curve
{
public:
    SpectroCurve(const QString &title, Channel *channel);
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

    virtual QList<QwtLegendData> legendData() const override;
    virtual SamplePoint samplePoint(SelectedPoint point) const override;
    virtual SelectedPoint closest(const QPoint &pos, double *dist1, double *dist2) const override;

    void setColorInterval(double min, double max);
    QwtInterval colorInterval() const;
protected:
    virtual void updatePen() override {}
};

#endif // SPECTROCURVE_H
