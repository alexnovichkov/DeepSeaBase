#ifndef QWHEELZOOMSVC_H
#define QWHEELZOOMSVC_H

class Plot;
#include <QObject>
#include "zoomstack.h"
#include "enums.h"

class WheelZoom : public QObject
{
    Q_OBJECT
public:
    explicit WheelZoom(Plot *plot);
    ZoomStack::zoomCoordinates applyWheel(QEvent *, Enums::AxisType axis);
private:
    Plot *plot;
    QPointF getCoords(Enums::AxisType axis, int pos, double factor);
};

#endif // QWHEELZOOMSVC_H
