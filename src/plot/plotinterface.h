#ifndef PLOTINTERFACE_H
#define PLOTINTERFACE_H

#include "enums.h"
#include <QPoint>
#include <QSize>
#include <QPrinter>
#include "selectable.h"

class Curve;
class Channel;
class QMouseEvent;
class CanvasEventFilter;
class QCPAxis;

class PlotInterface
{
public:
    virtual ~PlotInterface() {}
    virtual void setEventFilter(CanvasEventFilter *filter) = 0;
    virtual void createLegend() = 0;
    virtual double screenToPlotCoordinates(Enums::AxisType axis, double value) const = 0;
    virtual double plotToScreenCoordinates(Enums::AxisType axis, double value) const = 0;
    virtual Range plotRange(Enums::AxisType axis) const = 0;
    virtual Range screenRange(Enums::AxisType axis) const = 0;
    virtual void replot() = 0;
    virtual void updateAxes() = 0;
    virtual void updateLegend() = 0;
    virtual QPoint localCursorPosition(const QPoint &globalCursorPosition) const = 0;

    virtual void setAxisScale(Enums::AxisType axis, Enums::AxisScale scale) = 0;
    virtual void setAxisRange(Enums::AxisType axis, double min, double max, double step) = 0;
    virtual void setInfoVisible(bool visible) = 0;
    virtual void enableAxis(Enums::AxisType axis, bool enable) = 0;
    virtual bool axisEnabled(Enums::AxisType axis) = 0;
    virtual void setAxisTitle(Enums::AxisType axis, const QString &title) = 0;
    virtual QString axisTitle(Enums::AxisType axis) const = 0;
    virtual void enableColorBar(Enums::AxisType axis, bool enable) = 0;
    virtual void setColorBarTitle(const QString &title) {}
    virtual void setColorMap(int colorMap, Curve *curve) = 0;

    virtual void importPlot(const QString &fileName, const QSize &size, int resolution) = 0;
    virtual void importPlot(QPrinter &printer, const QSize &size, int resolution) = 0;
    virtual Curve* createCurve(const QString &legendName, Channel *channel, Enums::AxisType xAxis, Enums::AxisType yAxis) = 0;

    virtual Selected findObject(QPoint pos) const = 0;
    virtual void deselect() = 0;

    virtual QObject* eventTargetAxis(QEvent *event, QObject *target) = 0;
};

#endif // PLOTINTERFACE_H
