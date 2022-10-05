#ifndef QCPPLOT_H
#define QCPPLOT_H

#include "qcustomplot.h"
#include "enums.h"
#include "plot/plotinterface.h"

class Plot;
class CanvasEventFilter;
class QCPCheckableLegend;
class MouseCoordinates;
class QCPAxisOverlay;

QCPAxis::AxisType toQcpAxis(Enums::AxisType type);

class QCPPlot : public QCustomPlot, public PlotInterface
{
    friend  class MouseCoordinates;
    Q_OBJECT
public:
    QCPPlot(Plot *plot, QWidget *parent = nullptr);
    ~QCPPlot();

    QCPCheckableLegend *checkableLegend = nullptr;
private:
    Plot *parent = nullptr;
    CanvasEventFilter *canvasFilter = nullptr;
    QSharedPointer<QCPAxisTicker> linTicker;
    QSharedPointer<QCPAxisTickerLog> logTicker;
    QSharedPointer<QCPAxisTicker> octaveTicker;
    QCursor oldCursor;
    MouseCoordinates *mouseCoordinates;

    QCPAxisOverlay *leftOverlay = nullptr;
    QCPAxisOverlay *rightOverlay = nullptr;

    QCPColorScale *colorScale = nullptr;

    // PlotInterface interface
public:
    virtual void setEventFilter(CanvasEventFilter *filter) override;
    virtual Enums::AxisType eventTargetAxis(QEvent *event, QObject *target) override;
    virtual void createLegend() override;
    virtual double screenToPlotCoordinates(Enums::AxisType axisType, double value) const override;
    virtual double plotToScreenCoordinates(Enums::AxisType axisType, double value) const override;
    virtual Range plotRange(Enums::AxisType axisType) const override;
    virtual Range screenRange(Enums::AxisType axisType) const override;
    virtual void replot() override;
    virtual void updateAxes() override;
    virtual void updateLegend() override;
    virtual QPoint localCursorPosition(const QPoint &globalCursorPosition) const override;
    virtual void setAxisScale(Enums::AxisType axisType, Enums::AxisScale scale) override;
    virtual void setAxisRange(Enums::AxisType axis, double min, double max, double step) override;
    virtual void setInfoVisible(bool visible) override;
    virtual void enableAxis(Enums::AxisType axis, bool enable) override;
    virtual bool axisEnabled(Enums::AxisType axis) override;
    virtual void setAxisTitle(Enums::AxisType axis, const QString &title) override;
    virtual QString axisTitle(Enums::AxisType axis) const override;
    virtual void enableColorBar(Enums::AxisType axis, bool enable) override;
    virtual void setColorMap(Enums::AxisType axis, Range range, int colorMap, Curve *curve) override;
    virtual void setColorMap(int colorMap, Curve *curve) override;
    virtual void importPlot(const QString &fileName, const QSize &size, int resolution) override;
    virtual void importPlot(QPrinter &printer, const QSize &size, int resolution) override;
    virtual void setInteractionMode(Enums::InteractionMode mode) override;
    virtual Curve *createCurve(const QString &legendName, Channel *channel, Enums::AxisType xAxis, Enums::AxisType yAxis) override;
    virtual Selected findObject(QPoint pos) const override;
    virtual void deselect() override;
private:
    QCPAxis *axis(Enums::AxisType axis) const;
    void addZoom();

protected:
    virtual void keyPressEvent(QKeyEvent *event) override;
    virtual void dragEnterEvent(QDragEnterEvent *event) override;
    virtual void dragMoveEvent(QDragMoveEvent *event) override;
    virtual void dragLeaveEvent(QDragLeaveEvent *event) override;
    virtual void dropEvent(QDropEvent *event) override;
};

#endif // QCPPLOT_H
