#ifndef BARCURVE_H
#define BARCURVE_H

#include "qwt_plot_histogram.h"
#include "curve.h"

class HistogramData: public QwtSeriesData<QwtIntervalSample>
{
public:
    HistogramData(DataHolder *data);

    virtual QRectF boundingRect() const;

    virtual size_t size() const;

    virtual QwtIntervalSample sample( size_t i ) const;
    //возвращает не интервал, а точку из данных
    QPointF samplePoint(Curve::SelectedPoint point) const;

//    virtual double xStep() const;
//    virtual double xBegin() const;
    DataHolder *data;
    enum OctaveType {
        OctaveUnknown=0,
        Octave3=3,
        Octave1=1,
        Octave2=2,
        Octave6=6,
        Octave12=12,
        Octave24=24
    };
    int octaveType = 0;
    double factor = 1.0;
};

class BarCurve : public QwtPlotHistogram, public Curve
{
public:
    BarCurve(const QString &title, Channel *channel);

private:
    HistogramData *histogramdata;

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
    virtual void updatePen() override;
    virtual QList<QwtLegendData> legendData() const override;
    virtual void updateSelection() override;
    virtual QPointF samplePoint(SelectedPoint point) const override;

    // Curve interface
public:
    virtual double xMin() const override;
    virtual double xMax() const override;

    // Curve interface
public:
    virtual SelectedPoint closest(const QPoint &pos, double *dist1, double *dist2) const override;
};

#endif // BARCURVE_H
