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
    bool harmonics() const {return _showHarmonics;}
protected:
    virtual void widgetKeyReleaseEvent(QKeyEvent *e);
    virtual void widgetKeyPressEvent(QKeyEvent *e);
signals:
    void updateTrackingCursor(double xVal, bool second);
    void cursorMovedTo(QwtPlotMarker *cursor, double newValue);
    void labelSelected(bool);
public slots:
    void showHarmonics(bool show) {_showHarmonics = show;}
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
    QCursor defaultCursor;

    Plot::InteractionMode mode;

//    QList<PointLabel*> labels;
    PointLabel *selectedLabel;

    QPoint d_currentPos;

    bool _showHarmonics;
    QList<QwtPlotMarker *> _harmonics;

    QList<QwtPlotMarker *> cursors;
    QwtPlotMarker *selectedCursor;
protected:
    virtual QwtText trackerTextF(const QPointF &pos) const;
};

#endif // PLOTPICKER_H
