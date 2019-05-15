#ifndef LINECURVE_H
#define LINECURVE_H

#include "curve.h"

class DfdData: public QwtSeriesData<QPointF>
{
public:
    DfdData(DataHolder *data);

    virtual QRectF boundingRect() const;

    virtual size_t size() const;

    virtual QPointF sample( size_t i ) const;

    virtual double xStep() const;
    virtual double xBegin() const;
private:
    DataHolder *data;
};

class LineCurve : public QwtPlotCurve, public Curve
{
public:
    LineCurve(const QString &title, FileDescriptor *descriptor, int channelIndex);

    // QwtPlotCurve interface
protected:
    virtual void drawLines(QPainter *painter, const QwtScaleMap &xMap, const QwtScaleMap &yMap, const QRectF &canvasRect, int from, int to) const;
private:
    DfdData *dfddata;

    // Curve interface
public:
    virtual void attachTo(QwtPlot *plot) override;
    virtual QString title() const override;
    virtual void setTitle(const QString &title) override;
    virtual int yAxis() const override;
    virtual void setYAxis(int axis) override;
    virtual QPen pen() const override;
    virtual void setPen(const QPen &pen) override;
    virtual QList<QwtLegendData> legendData() const override;
    virtual void highlight() override;
    virtual void resetHighlighting() override;
    virtual QPointF samplePoint(int point) const override;

    // Curve interface
public:
    virtual int closest(const QPoint &pos, double *dist) const override;
};

#endif // LINECURVE_H
