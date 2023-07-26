#ifndef QCPPOINTMARKER_H
#define QCPPOINTMARKER_H

#include "qcustomplot.h"
#include "plot/selectable.h"

class QCPPlot;
class Graph2D;
class PointLabel;
class Curve;
class QCPTracer;
class QCPPlot;

class PointLabel : public QCPItemText, public Selectable
{
public:
    enum class Mode {
        XValue,
        XYValue,
        YValue,
        XYZValue
    };
    explicit PointLabel(QCPPlot *plot, Curve *curve);
    ~PointLabel() {}

    void detachFrom(QCPPlot *plot);

    //shadows QCPLayerable::setVisible
    void setVisible(bool visible);

    void setMode(Mode mode);
    inline Mode mode() const {return m_mode;}
    Curve *curve() const {return m_curve;}

    SamplePoint getOrigin() const;
    SelectedPoint point() const {return m_point;}
    void setPoint(SelectedPoint point);
    void updateLabel();
    // Selectable interface
public:
    virtual bool draggable() const override;
    virtual void moveToPos(QPoint pos, QPoint startPos) override;
    virtual void cycle() override;
    virtual bool underMouse(const QPoint &pos, double *distanceX, double *distanceY, SelectedPoint *point) const override;
    virtual QList<QAction *> actions() override;
    virtual void remove() override;

protected:
    virtual void updateSelection(SelectedPoint point) override;
private:

    SelectedPoint m_point;
    Mode m_mode = Mode::XValue;
    QCPPlot *m_plot = nullptr;
    Curve *m_curve = nullptr;
    QCPTracer *m_tracer = nullptr;
};

#endif // QCPPOINTMARKER_H
