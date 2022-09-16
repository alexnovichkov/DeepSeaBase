#ifndef POINTLABEL_H
#define POINTLABEL_H

#include "qwt_plot_item.h"

#include "qwt_text.h"
class QwtPlot;
class Curve;
#include <QtCore>
#include "selectable.h"

/**
 * @brief The PointLabel class
 * Is intended to draw a label with constant displacement relative to a curve point
 */

class PointLabel : public QwtPlotItem, public Selectable
{
public:
    explicit PointLabel(QwtPlot *parent, Curve *curve);

    virtual ~PointLabel();

    virtual int rtti() const override;

    QPointF origin() const;

    void setOrigin(const QPointF &origin);

    void setMode(int mode);
    virtual void cycle() override;
    int mode() const {return d_mode;}

    int point() const;

    void setPoint(int point);
    void setLabel(const QwtText& label);

    QwtText label() const;

    virtual void updateSelection() override;
    virtual bool underMouse(const QPoint &pos, double *distanceX = nullptr, double *distanceY = nullptr) const override;

    virtual void draw(QPainter *painter, const QwtScaleMap &xMap,
                      const QwtScaleMap &yMap, const QRectF &canvasRect) const override;
    virtual void remove() override;
    virtual void moveToPos(QPoint pos, QPoint startPos = QPoint()) override;
    virtual QList<QAction*> actions() override;
    virtual bool draggable() const override {return true;}

    bool contains(const QPoint &pos);
private:
    void updateLabel();
    int d_mode = 0;
    int d_point;
    QPointF d_origin; // origin of Label, equivalent to value of PlotMarker
    QPoint d_displacement; // displacement relative to invTransform(d_origin)
    QwtText d_label;
    QwtPlot *plot;
public:
    Curve *curve;
};

#endif // POINTLABEL_H
