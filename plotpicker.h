#ifndef PLOTPICKER_H
#define PLOTPICKER_H

#include <qwt_plot_picker.h>

#include "plot.h"
#include "curve.h"

class QwtPlotCurve;

class PlotPicker : public QwtPlotPicker
{
    Q_OBJECT
public:
    explicit PlotPicker(QWidget *canvas);
    void setMode(Plot::InteractionMode mode);
//    virtual bool eventFilter(QObject *target, QEvent *);
    virtual void widgetKeyReleaseEvent(QKeyEvent *e);
//    virtual bool event(QEvent *);
signals:
    
public slots:
    
private slots:
    void pointAppended(const QPoint &pos);
    void pointMoved(const QPoint &pos);

private:
    void highlightPoint(bool enable);
    void resetHighLighting();
    Curve *findClosestPoint(const QPoint &pos, int &index) const;
    Marker * findMarker();

    QwtPlot *plot;
    Curve *d_selectedCurve;
    QwtPlotMarker *marker;
    int d_selectedPoint;
    Plot::InteractionMode mode;

    Marker *selectedMarker;

protected:
    virtual QwtText trackerTextF(const QPointF &pos) const;
};

#endif // PLOTPICKER_H