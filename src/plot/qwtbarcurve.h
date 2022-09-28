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
    SamplePoint samplePoint(SelectedPoint point) const;

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

class QwtBarCurve : public QwtPlotHistogram, public Curve
{
public:
    QwtBarCurve(const QString &title, Channel *channel);

private:
    HistogramData *histogramdata;

    // Curve interface
public:
    virtual void attachTo(Plot *plot) override;
    virtual QString title() const override;
    virtual void setTitle(const QString &title) override;

    virtual Enums::AxisType yAxis() const override;
    virtual void setYAxis(Enums::AxisType axis) override;

    virtual Enums::AxisType xAxis() const override;
    virtual void setXAxis(Enums::AxisType axis) override;

    virtual QPen pen() const override;
    virtual void updatePen() override;
    virtual QList<QwtLegendData> legendData() const override;
    virtual void updateSelection(SelectedPoint point) override;
    virtual SamplePoint samplePoint(SelectedPoint point) const override;

    // Curve interface
public:
    virtual double xMin() const override;
    virtual double xMax() const override;

    // Curve interface
public:
    virtual SelectedPoint closest(const QPoint &pos, double *dist1, double *dist2) const override;
};

#endif // BARCURVE_H
