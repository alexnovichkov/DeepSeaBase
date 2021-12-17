#ifndef SCALEDRAW_H
#define SCALEDRAW_H

#include <qwt_painter.h>
#include <qwt_scale_draw.h>
#include <qwt_scale_widget.h>

#include <QWidget>

class ScaleDraw : public QwtScaleDraw
{
public:
    ScaleDraw();
    int hover = 0; //0=none, 1=first, 2=second

protected:
    virtual void drawBackbone(QPainter *painter) const override;
};

class Plot;

class AxisOverlay : public QWidget
{
    Q_OBJECT
public:
    explicit AxisOverlay(Plot *parent);
    void setVisible(bool visible) override;
    void paintEvent(QPaintEvent *event) override;
    Plot *plot();
protected:
    virtual void setGeom() = 0;
private:
    Plot *m_plot;
};

class LeftAxisOverlay : public AxisOverlay
{
    Q_OBJECT
public:
    explicit LeftAxisOverlay(Plot *parent);
protected:
    void setGeom() override;
};

class RightAxisOverlay : public AxisOverlay
{
    Q_OBJECT
public:
    explicit RightAxisOverlay(Plot *parent);
protected:
    void setGeom() override;
};

#endif // SCALEDRAW_H
