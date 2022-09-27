#ifndef POINTLABEL_H
#define POINTLABEL_H

#include "qwt_plot_item.h"

#include "qwt_text.h"
#include "qwt_symbol.h"
class Plot;
class Curve;

#include <QtCore>
#include "selectable.h"

#include "plotinterface.h"

/**
 * @brief The PointLabel class
 * Is intended to draw a label with constant displacement relative to a curve point
 */

class PointLabel : public QwtPlotItem, public Selectable
{
public:
    enum class Mode {
        XValue,
        XYValue,
        YValue,
        XYZValue
    };
    explicit PointLabel(Plot *parent, Curve *m_curve);

    virtual ~PointLabel();

    virtual int rtti() const override;

//    SamplePoint origin() const;
//    void setOrigin(const SamplePoint &origin);

    SamplePoint getOrigin() const;

    void setMode(Mode mode);
    inline Mode mode() const {return m_mode;}
    virtual void cycle() override;

    SelectedPoint point() const;

    void setPoint(SelectedPoint point);
    void setLabel(const QwtText& label);

    QwtText label() const;
    void attach(PlotInterface *impl);

    virtual void updateSelection(SelectedPoint point) override;
    virtual bool underMouse(const QPoint &pos, double *distanceX, double *distanceY, SelectedPoint *point) const override;

    virtual void draw(QPainter *painter, const QwtScaleMap &xMap,
                      const QwtScaleMap &yMap, const QRectF &canvasRect) const override;
    virtual void remove() override;
    virtual void moveToPos(QPoint pos, QPoint startPos = QPoint()) override;
    virtual QList<QAction*> actions() override;
    virtual bool draggable() const override {return true;}

    bool contains(const QPoint &pos);
    static SamplePoint check(SamplePoint point);

    void updateLabel();
private:
    Mode m_mode = Mode::XValue;
    SelectedPoint m_point;
//    SamplePoint m_origin; // origin of Label, equivalent to value of PlotMarker
    QPoint m_displacement; // displacement relative to invTransform(d_origin)
    QwtText m_label;
    QwtSymbol m_marker;
    Plot *m_plot;
    Curve *m_curve;
};

#endif // POINTLABEL_H
