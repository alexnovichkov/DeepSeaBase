#ifndef FILTERPOINTMAPPER_H
#define FILTERPOINTMAPPER_H

#include <qwt_point_mapper.h>
#include <QPolygonF>
#include <qwt_scale_map.h>
#include "enums.h"




typedef QList<PointBlock> PointBlocks;

class FilterPointMapper : public QwtPointMapper
{
public:
    FilterPointMapper(bool createPolygon);

    QPolygonF getPolygon( const QwtScaleMap &xMap, const QwtScaleMap &yMap,
            const QwtSeriesData<QPointF> *series, int from, int to);

    bool simplified = false;
    bool polygon = false;
    QPolygonF cashedPolyline;
    int oldFrom = 0;
    int oldTo = 0;
    QwtScaleMap oldXMap;
    QwtScaleMap oldYMap;

    QVector<PointBlock> cashedBlocks;
};

#endif // FILTERPOINTMAPPER_H
