#ifndef PLOTPICKER_H
#define PLOTPICKER_H

#include <qwt_plot_picker.h>

#include "plot.h"
#include "curve.h"

class QwtPlotCurve;
class PointLabel;
class QwtPlotMarker;

class PlotPicker : public QwtPlotPicker
{
    Q_OBJECT
public:
    explicit PlotPicker(QWidget *canvas);
    ~PlotPicker();
    void setMode(Plot::InteractionMode mode);
protected:
    virtual void widgetKeyReleaseEvent(QKeyEvent *e);
    virtual void widgetKeyPressEvent(QKeyEvent *e);
signals:
    void cursorMovedTo(double newValue);
    void moveCursor(bool moveToRight);
    void cursorSelected(QwtPlotMarker *cursor);
    void setZoomEnabled(bool);
private slots:
    void pointAppended(const QPoint &pos);
    void pointMoved(const QPoint &pos);

private:
    void highlightPoint(bool enable);
    void resetHighLighting();
    Curve *findClosestPoint(const QPoint &pos, int &index) const;

    PointLabel *findLabel();
    QwtPlotMarker *findCursor(const QPoint &pos);

    int d_selectedPoint;
    QwtPlot *plot;
    Curve *d_selectedCurve;
    QwtPlotMarker *marker;

    Plot::InteractionMode mode;

    PointLabel *d_selectedLabel;

    QPoint d_currentPos;

    QwtPlotMarker *d_selectedCursor;
protected:
    virtual QwtText trackerTextF(const QPointF &pos) const;
};

#endif // PLOTPICKER_H
