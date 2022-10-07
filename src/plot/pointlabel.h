#ifndef POINTLABEL_H
#define POINTLABEL_H

class Plot;
class Curve;

#include <QtCore>
#include "selectable.h"
#include "plotinterface.h"

class LabelImpl {
public:
    virtual ~LabelImpl() {}
    virtual void attachTo(Plot *plot) = 0;
    virtual void detachFrom(Plot *plot) = 0;
    virtual void setColor(const QColor &color) = 0;
    virtual void setBrush(const QBrush &brush) = 0;
    virtual void setXAxis(Enums::AxisType axis) = 0;
    virtual void setYAxis(Enums::AxisType axis) = 0;
    virtual void setLabel(const QString &label) = 0;
    virtual void setBorder(const QPen &pen) = 0;
    virtual void setVisible(bool visible) = 0;
    virtual QSizeF textSize() const = 0;
    virtual void update() = 0;
};

/**
 * @brief The PointLabel class
 * Is intended to draw a label with constant displacement relative to a curve point
 */

class PointLabel : public Selectable
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

    Curve *curve() const {return m_curve;}

//    SamplePoint origin() const;
//    void setOrigin(const SamplePoint &origin);

    SamplePoint getOrigin() const;
    QPoint getDisplacement() const;

    void setMode(Mode mode);
    inline Mode mode() const {return m_mode;}
    virtual void cycle() override;

    SelectedPoint point() const;

    void setPoint(SelectedPoint point);
    void setLabel(const QString& label);

    void setXAxis(Enums::AxisType axis);
    void setYAxis(Enums::AxisType axis);

    void setVisible(bool visible);

//    QString label() const;
    void attachTo(Plot *plot);
    void detachFrom(Plot *plot);

    virtual void updateSelection(SelectedPoint point) override;
    virtual bool underMouse(const QPoint &pos, double *distanceX, double *distanceY, SelectedPoint *point) const override;
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
    QPoint m_displacement; // displacement relative to invTransform(d_origin)
    Plot *m_plot;
    Curve *m_curve;
    LabelImpl *m_impl = nullptr;
};

#endif // POINTLABEL_H
