#ifndef POINTMARKER_H
#define POINTMARKER_H

#include <qwt_plot_marker.h>
#include <qwt_axis_id.h>

class PointMarker : public QwtPlotMarker
{
public:
    PointMarker(const QColor &color, QwtAxisId axis);
    void moveTo(const QPointF &val);
};

#endif // POINTMARKER_H
