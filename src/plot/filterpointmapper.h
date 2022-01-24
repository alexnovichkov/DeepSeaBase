#ifndef FILTERPOINTMAPPER_H
#define FILTERPOINTMAPPER_H

#include <qwt_point_mapper.h>
#include <QPolygonF>
#include <qwt_scale_map.h>

struct PointBlock
{
    double minX = 0;
    double maxX = 0;
    double minY = 0;
    double maxY = 0;
    int from = 0;
    int to = 0;
};



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
};

#endif // FILTERPOINTMAPPER_H
