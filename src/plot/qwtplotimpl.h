#ifndef QWTPLOTIMPL_H
#define QWTPLOTIMPL_H

#include "plotinterface.h"
#include <qwt_plot.h>

#include "enums.h"

class QwtPlotCanvas;
class PlotInfoOverlay;
class AxisOverlay;
class Grid;
class Curve;
class Plot;
class DragZoom;
class WheelZoom;
class AxisZoom;
class PlotZoom;
class CanvasEventFilter;
class Picker;
class PlotTracker;

Enums::AxisType toAxisType(QwtAxisId id);
QwtAxisId toQwtAxisType(Enums::AxisType type);

class QwtPlotImpl : public QwtPlot, public PlotInterface
{
    Q_OBJECT
public:
    QwtPlotImpl(Plot *plot, QWidget *parent = nullptr);
    ~QwtPlotImpl();

    virtual void createLegend() override;
    virtual void setEventFilter(CanvasEventFilter *filter) override;
    virtual QObject* eventTargetAxis(QEvent *event, QObject *target) override;
    virtual double screenToPlotCoordinates(Enums::AxisType axis, double value) const override;
    virtual double plotToScreenCoordinates(Enums::AxisType axis, double value) const override;
    virtual Range plotRange(Enums::AxisType axis) const override;
    virtual Range screenRange(Enums::AxisType axis) const override;
    virtual void replot() override;
    virtual void updateAxes() override;
    virtual void updateLegend() override;
    virtual QPoint localCursorPosition(const QPoint &globalCursorPosition) const override;
    virtual void setAxisScale(Enums::AxisType axis, Enums::AxisScale scale) override;
    virtual void setAxisRange(Enums::AxisType axis, double min, double max, double step) override;
    virtual void setInfoVisible(bool visible) override;
    virtual void enableAxis(Enums::AxisType axis, bool enable) override;
    virtual void enableColorBar(Enums::AxisType axis, bool enable) override;
    virtual void setColorMap(int colorMap, Curve *curve) override;
    virtual bool axisEnabled(Enums::AxisType axis) override;
    virtual void setAxisTitle(Enums::AxisType axis, const QString &title) override;
    virtual QString axisTitle(Enums::AxisType axis) const override;
    virtual void importPlot(const QString &fileName, const QSize &size, int resolution) override;
    virtual void importPlot(QPrinter &printer, const QSize &size, int resolution) override;
    virtual void setInteractionMode(Enums::InteractionMode mode) override;
    virtual Curve *createCurve(const QString &legendName, Channel *channel, Enums::AxisType xAxis, Enums::AxisType yAxis) override;
    virtual Selected findObject(QPoint pos) const override;
    virtual void deselect() override;

    Plot *parent = nullptr;
signals:
    void curveMarkedToMove(Curve*);
    void canvasDoubleClicked(QPoint);
private slots:
    void editLegendItem(QwtPlotItem *item);
    void deleteCurveFromLegend(QwtPlotItem *item);
    void fixCurve(QwtPlotItem* curve);
private:
    void hoverAxis(Enums::AxisType axis, int hover);

    QwtPlotCanvas *_canvas = nullptr;
    PlotInfoOverlay *infoOverlay = nullptr;
    AxisOverlay *leftOverlay = nullptr;
    AxisOverlay *rightOverlay = nullptr;
    Grid *grid = nullptr;

    DragZoom *dragZoom = nullptr;
    WheelZoom *wheelZoom = nullptr;
    AxisZoom *axisZoom = nullptr;
    PlotZoom *plotZoom = nullptr;
    CanvasEventFilter *canvasFilter = nullptr;
    PlotTracker *tracker = nullptr;

    // QWidget interface
public:
    void dropEvent(QDropEvent *event) override;
protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
};

#endif // QWTPLOTIMPL_H
