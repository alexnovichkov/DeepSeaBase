#ifndef PLOTPICKER_H
#define PLOTPICKER_H

#include <qwt_plot_picker.h>
#include "plot.h"

class QwtPlotCurve;
class QwtPlotMarker;

class PlotPicker : public QwtPlotPicker
{
    Q_OBJECT
public:
    explicit PlotPicker(QWidget *canvas);
    void setMode(Plot::InteractionMode mode);
signals:
    
public slots:
    
private slots:
    void pointAppended(const QPoint &pos);
    void pointMoved(const QPoint &pos);

private:
    void highlightPoint(bool enable);
    void resetHighLighting();

    QwtPlot *plot;
    QwtPlotCurve *d_selectedCurve;
    QwtPlotMarker *marker;
    int d_selectedPoint;
    Plot::InteractionMode mode;

    // QwtPlotPicker interface
protected:
    virtual QwtText trackerTextF(const QPointF &pos) const;

};

#endif // PLOTPICKER_H
