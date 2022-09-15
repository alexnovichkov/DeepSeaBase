#ifndef LINECURVE_H
#define LINECURVE_H

#include "curve.h"
#include "qwt_axis_id.h"
#include <QPolygonF>

class FilterPointMapper;

class LineData: public QwtSeriesData<QPointF>
{
public:
    LineData(DataHolder *data);

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
    LineData *dfddata;
    virtual bool doCloseLine() const;

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

    virtual void resetCashedData() override;

    // Curve interface
public:
    virtual SelectedPoint closest(const QPoint &pos, double *dist1, double *dist2) const override;

    // QwtPlotItem interface
public:
    virtual void setVisible(bool visible) override;
};

class TimeCurve : public LineCurve
{
public:
    TimeCurve(const QString &title, Channel *channel);
protected:
    virtual void setMapper() override; //always sets the mapper
    virtual bool doCloseLine() const override; //always closes the contour
};

#endif // LINECURVE_H
