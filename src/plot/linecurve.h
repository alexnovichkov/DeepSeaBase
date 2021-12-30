#ifndef LINECURVE_H
#define LINECURVE_H

#include "curve.h"
#include "qwt_axis_id.h"
#include <QPolygonF>

class FilterPointMapper;

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
    LineCurve(const QString &title, Channel *channel);
    virtual ~LineCurve();

    // QwtPlotCurve interface
protected:
    virtual void setMapper();
    virtual void drawLines(QPainter *painter, const QwtScaleMap &xMap,
                           const QwtScaleMap &yMap, const QRectF &canvasRect,
                           int from, int to) const override;
    FilterPointMapper *mapper;
    DfdData *dfddata;

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
    virtual void highlight() override;
    virtual void resetHighlighting() override;
    virtual QPointF samplePoint(int point) const override;

    virtual void resetCashedData() override;

    // Curve interface
public:
    virtual int closest(const QPoint &pos, double *dist) const override;

    // QwtPlotItem interface
public:
    virtual void setVisible(bool visible) override;
};

class TimeCurve : public LineCurve
{
public:
    TimeCurve(const QString &title, Channel *channel);
protected:
    virtual void setMapper() override;
    virtual void drawLines(QPainter *painter, const QwtScaleMap &xMap,
                           const QwtScaleMap &yMap, const QRectF &canvasRect,
                           int from, int to) const override;
};

#endif // LINECURVE_H
