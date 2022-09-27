#ifndef QCPPLOT_H
#define QCPPLOT_H

#include "qcustomplot.h"
#include "enums.h"

class ZoomStack;
class PlotModel;

QCPAxis::AxisType toQcpAxis(Enums::AxisType type);

class QCPPlot : public QCustomPlot
{
    Q_OBJECT
public:
    QCPPlot(Enums::PlotType type, QWidget *parent = nullptr);
    inline Enums::PlotType type() const {return plotType;}

    void toggleAutoscale(Enums::AxisType axis, bool toggled);
    void autoscale(Enums::AxisType axis = Enums::AxisType::atInvalid);
    void setScale(Enums::AxisType id, double min, double max, double step = 0);
    void removeLabels();
    void cycleChannels(bool up);

    void deleteAllCurves(bool forceDeleteFixed = false);

    //virtual methods
    virtual void setRightScale(Enums::AxisType id, double min, double max);
protected:
    PlotModel *m = nullptr;
private:
    Enums::PlotType plotType = Enums::PlotType::General;
    ZoomStack *zoom = nullptr;
    bool sergeiMode = false;
};

#endif // QCPPLOT_H
