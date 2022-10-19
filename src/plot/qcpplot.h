#ifndef QCPPLOT_H
#define QCPPLOT_H

#include "qcustomplot.h"
#include "enums.h"

class Plot;
class CanvasEventFilter;
class QCPCheckableLegend;
class MouseCoordinates;
class QCPAxisOverlay;
class QCPInfoOverlay;
class Curve;
class Channel;
class Selected;

QCPAxis::AxisType toQcpAxis(Enums::AxisType type);
Enums::AxisType fromQcpAxis(QCPAxis::AxisType);

class QCPPlot : public QCustomPlot
{
    friend  class MouseCoordinates;
    Q_OBJECT
public:
    QCPPlot(Plot *plot, QWidget *parent = nullptr);
    ~QCPPlot();

    void startZoom(QMouseEvent *event);
    void proceedZoom(QMouseEvent *event);
    void endZoom(QMouseEvent *event);
    void cancelZoom();

    void updateSecondaryPlots(const QPointF &value);

    QCPCheckableLegend *checkableLegend = nullptr;
signals:
    void canvasDoubleClicked(QPoint);

private:
    Plot *parent = nullptr;
    CanvasEventFilter *canvasFilter = nullptr;
    QSharedPointer<QCPAxisTicker> linTicker;
    QSharedPointer<QCPAxisTickerLog> logTicker;
    QSharedPointer<QCPAxisTicker> octaveTicker;
    QCursor oldCursor;
    MouseCoordinates *mouseCoordinates;
    QCPInfoOverlay *infoOverlay = nullptr;
    QCPAxisOverlay *leftOverlay = nullptr;
    QCPAxisOverlay *rightOverlay = nullptr;
    QCPColorScale *colorScale = nullptr;
    QCPAxisRect *spectreRect = nullptr;
    QCPAxisRect *throughRect = nullptr;
    QCPGraph *spectreGraph = nullptr;
    QCPGraph *throughGraph = nullptr;
    QCPTextElement *spectreTitle = nullptr;
    QCPTextElement *throughTitle = nullptr;


    // PlotInterface interface
public:
    void setEventFilter(CanvasEventFilter *filter);
    QCPAxis *eventTargetAxis(QEvent *event, QObject *target);
    void createLegend();
    double screenToPlotCoordinates(Enums::AxisType axisType, double value) const;
    double plotToScreenCoordinates(Enums::AxisType axisType, double value) const;
    Range plotRange(Enums::AxisType axisType) const;
    Range screenRange(Enums::AxisType axisType) const;
    void replot();
    void updateAxes();
    void updateLegend();
    QPoint localCursorPosition(const QPoint &globalCursorPosition) const;
    void setAxisScale(Enums::AxisType axisType, Enums::AxisScale scale);
    void setAxisRange(Enums::AxisType axis, double min, double max, double step);
    void setInfoVisible(bool visible);
    void enableAxis(Enums::AxisType axis, bool enable);
    bool axisEnabled(Enums::AxisType axis);
    void setAxisTitle(Enums::AxisType axis, const QString &title);
    QString axisTitle(Enums::AxisType axis) const;
    void enableColorBar(Enums::AxisType axis, bool enable);
    void setColorMap(int colorMap, Curve *curve);
    void setColorBarTitle(const QString &title);
    void importPlot(const QString &fileName, const QSize &size, int resolution);
    void importPlot(QPrinter &printer, const QSize &size, int resolution);
    Curve *createCurve(const QString &legendName, Channel *channel, Enums::AxisType xAxis, Enums::AxisType yAxis);
    Selected findObject(QPoint pos) const;
    void deselect();
private:
    QCPAxis *axis(Enums::AxisType axis) const;
    void addZoom();

protected:
//    virtual void keyPressEvent(QKeyEvent *event) override;
    virtual void dragEnterEvent(QDragEnterEvent *event) override;
    virtual void dragMoveEvent(QDragMoveEvent *event) override;
    virtual void dragLeaveEvent(QDragLeaveEvent *event) override;
    virtual void dropEvent(QDropEvent *event) override;
};

#endif // QCPPLOT_H
