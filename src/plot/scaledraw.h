#ifndef SCALEDRAW_H
#define SCALEDRAW_H

#include <qwt_painter.h>
#include <qwt_scale_draw.h>
#include <qwt_scale_widget.h>

#include <qwt_plot_zoneitem.h>

class ScaleDraw : public QwtScaleDraw
{
public:
    ScaleDraw();
    int hover = 0; //0=none, 1=first, 2=second

protected:
    virtual void drawBackbone(QPainter *painter) const override;
};

class QwtPlot;

class AxisOverlay : public QwtPlotZoneItem
{
public:
    explicit AxisOverlay(QwtPlot *parent);
    void setVisibility(bool visible);
    QwtPlot *plot();
protected:
    virtual void setGeom() = 0;
    void setColor();
private:
    QwtPlot *m_plot;
};

class LeftAxisOverlay : public AxisOverlay
{
public:
    explicit LeftAxisOverlay(QwtPlot *parent);
protected:
    void setGeom() override;
};

class RightAxisOverlay : public AxisOverlay
{
public:
    explicit RightAxisOverlay(QwtPlot *parent);
protected:
    void setGeom() override;
};

#endif // SCALEDRAW_H
