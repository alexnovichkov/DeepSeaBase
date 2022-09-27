#ifndef QWTPLOTIMPL_H
#define QWTPLOTIMPL_H

#include "plotinterface.h"
#include <qwt_plot.h>

#include "enums.h"

class QwtPlotCanvas;

Enums::AxisType toAxisType(QwtAxisId id);
QwtAxisId toQwtAxisType(Enums::AxisType type);

class QwtPlotImpl : public PlotInterface, public QwtPlot
{
    Q_OBJECT
public:
    QwtPlotImpl();
private slots:
    void editLegendItem(QwtPlotItem *item);
    void deleteCurveFromLegend(QwtPlotItem *item);
    void fixCurve(QwtPlotItem* curve);
private:
    QwtPlotCanvas *_canvas = nullptr;
};

#endif // QWTPLOTIMPL_H
