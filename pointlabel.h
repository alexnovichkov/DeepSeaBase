#ifndef POINTLABEL_H
#define POINTLABEL_H

#include "qwt_plot_item.h"

class QwtText;
class QwtPlot;

/**
 * @brief The PointLabel class
 * Is intended to draw a label with constant displacement relative to a curve point
 */

class PointLabel : public QwtPlotItem
{
public:
    explicit PointLabel(QwtPlot *parent);

    virtual ~PointLabel();

    virtual int rtti() const;

    QPointF origin() const;

    void setOrigin(const QPointF &origin);

    void setMode(int mode);
    void cycleMode();
    int mode() const {return d_mode;}

    int point() const;

    void setPoint(int point);

    QPoint displacement() const;

    void setDisplacement(const QPoint &displacement);
    void setDisplacement(int dx, int dy);

    void setLabel(const QwtText& label);

    QwtText label() const;

    bool selected() const;

    void setSelected(bool selected);

    virtual void draw(QPainter *painter, const QwtScaleMap &xMap, const QwtScaleMap &yMap, const QRectF &canvasRect) const;

    void moveBy(const QPoint &pos);

    bool contains(const QPoint &pos, int yAxis);
private:
    int d_mode;
    int d_point;
    QPointF d_origin; // origin of Label, equivalent to value of PlotMarker
    QPoint d_displacement; // displacement relative to invTransform(d_origin)
    QwtText d_label;
    QwtPlot *plot;
    bool d_selected;
};

#endif // POINTLABEL_H
