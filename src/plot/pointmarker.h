#ifndef POINTMARKER_H
#define POINTMARKER_H

#include <qwt_plot_marker.h>
#include <qwt_axis_id.h>
class Curve;

class PointMarker : public QwtPlotMarker
{
public:
    PointMarker(const QColor &color, QwtAxisId axis);
    PointMarker(Curve *parent = nullptr);
    void moveTo(const QPointF &val);
private:
    Curve *curve = nullptr;
};

#endif // POINTMARKER_H
