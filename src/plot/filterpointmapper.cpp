#include "filterpointmapper.h"

#include <QtDebug>
#include <qwt_series_data.h>
#include "logging.h"

bool scaleMapEquals(const QwtScaleMap &map1, const QwtScaleMap &map2)
{
    return (qFuzzyCompare(map1.s1()+1.0, map2.s1()+1.0) &&
            qFuzzyCompare(map1.s2()+1.0, map2.s2()+1.0) &&
            qFuzzyCompare(map1.p1()+1.0, map2.p1()+1.0) &&
            qFuzzyCompare(map1.p2()+1.0, map2.p2()+1.0));
}

QVector<PointBlock> getBlocks(int from, int to, const QwtSeriesData<QPointF> *series, int pixels, int pointCount)
{
    QVector<PointBlock> blocks;

    PointBlock block;

    //iterate over pixels
    int start = from;
    for (int pixel=0; pixel<pixels; ++pixel) {
        if (start == to) {
            //qDebug()<<"reached end";
            break;
        }

        int end = (((double)pixel+1.0)/pixels)*pointCount + from;
        if (end > to) end = to;

        //first two samples
        QPointF sample1 = series->sample(start);
        QPointF sample2 = series->sample(start+1);

        //now find range [min;max] for current pixel

        //first two points
        if(sample1.y() < sample2.y()) {
            block.minY = sample1.y();
            block.maxY = sample2.y();
            block.minX = sample1.x();
            block.maxX = sample2.x();
        }
        else {
            block.minY = sample2.y();
            block.maxY = sample1.y();
            block.minX = sample2.x();
            block.maxX = sample1.x();
        }

        //rest of points
        for(int k=start+2; k<end; k+=2) {
            sample1 = series->sample(k);
            sample2 = series->sample(k+1);
            if(sample1.y() > sample2.y()) {
                if(sample1.y() > block.maxY) {
                    block.maxY = sample1.y();
                    block.maxX = sample1.x();
                }

                if(sample2.y()<block.minY) {
                    block.minY = sample2.y();
                    block.minX = sample2.x();
                }
            }
            else {
                if(sample2.y()>block.maxY) {
                    block.maxY = sample2.y();
                    block.maxX = sample2.x();
                }

                if(sample1.y()<block.minY) {
                    block.minY = sample1.y();
                    block.minX = sample1.x();
                }
            }
            block.from = start;
            block.to = end;
        }

        blocks << block;

        //new start for next iteration
        start = end;
    }
    return blocks;
}

QVector<PointBlock> getCashedBlocks(const QVector<PointBlock> &cashedBlocks, int from, int to)
{
    int left = 0;
    int right = 0;
    for (int i=0; i<cashedBlocks.size(); ++i) {
        if (from >= cashedBlocks.at(i).from && from <= cashedBlocks.at(i).to) {
            left = i;
            break;
        }
    }
    for (int i=cashedBlocks.size()-1; i>=0; i--) {
        if (to >= cashedBlocks.at(i).from && to <= cashedBlocks.at(i).to) {
            right = i;
            break;
        }
    }
    return cashedBlocks.mid(left, right-left+1);
}

QDebug operator<<(QDebug debug, const PointBlock &c)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << '(' << c.from << ", " << c.to << ") - " << c.minX << c.maxX << c.minY << c.maxY;

    return debug;
}

FilterPointMapper::FilterPointMapper(bool createPolygon) : QwtPointMapper(), polygon(createPolygon)
{ }

QPolygonF FilterPointMapper::getPolygon(const QwtScaleMap &xMap, const QwtScaleMap &yMap, const QwtSeriesData<QPointF> *series, int from, int to)
{DD0;
    //if horizontal distance is null
    if (qFuzzyIsNull(xMap.sDist())) return QPolygonF();

    if (from == oldFrom && to == oldTo &&
        scaleMapEquals(oldXMap, xMap) &&
        scaleMapEquals(oldYMap, yMap)) {
        if (!cashedPolyline.isEmpty()) return cashedPolyline;
    }

    //number of visible points for current zoom
    const int pointCount = to - from + 1;
    //number of pixels
    const int pixels = int(xMap.transform(series->sample(to).x()) - xMap.transform(series->sample(from).x()));
    //we have less than 5* more points than screen pixels - no need to use resample
    if (pointCount < 5 || pixels == 0 || pointCount <= pixels*5) {
        simplified = false;
        cashedPolyline = QwtPointMapper::toPolygonF(xMap, yMap, series, from, to);
        oldFrom = from;
        oldTo = to;
        return cashedPolyline;
    }

    simplified = true;

    //array that will be used to store calculated plot points in screen coordinates
    QPolygonF polyline(pixels*2);
    QPointF *points = polyline.data();

    QVector<PointBlock> blocks;

//#define NEW_BLOCKS

#ifdef NEW_BLOCKS
    if (qFuzzyIsNull(oldXMap.sDist()) || !qFuzzyCompare(xMap.pDist()/xMap.sDist(), oldXMap.pDist()/oldXMap.sDist()))
#endif
        blocks = getBlocks(from, to, series, pixels, pointCount);
#ifdef NEW_BLOCKS
    else {
        //1. не пересекаются
        if (oldFrom > to || from > oldTo || (oldFrom==0 && oldTo==0))

            blocks = getBlocks(from, to, series, pixels, pointCount);
        //2. пересекаются
        else {
            int from1 = qMax(from, oldFrom);
            int to1 = qMin(to, oldTo);

            //левый кусок
            if (from < oldFrom)
                blocks = getBlocks(from, oldFrom-1, series, pixels, pointCount);

            //общий кусок
            blocks.append(getCashedBlocks(cashedBlocks,from1, to1));

            //правый кусок
            if (to > oldTo)
                blocks.append(getBlocks(oldTo+1, to, series, pixels, pointCount));
        }

        if (blocks.size() != pixels)
            qDebug()<<"Error: blocks count != pixels count:"<<blocks.size()<<pixels;
        blocks.resize(pixels);
    }
#endif

    cashedBlocks = blocks;

    for (int pixel=0; pixel<pixels; ++pixel) {
        auto &block = blocks[pixel];
        if (polygon) {
            QPointF &minValue = points[pixel];
            QPointF &maxValue = points[2 * pixels - 1 - pixel];
            minValue.rx() = (xMap.transform(block.minX)+xMap.transform(block.maxX))/2.0;
            minValue.ry() = yMap.transform(block.minY);
            maxValue.rx() = minValue.x();
            maxValue.ry() = yMap.transform(block.maxY);
        }
        else {
            double p1x = 0.0, p2x = 0.0, p1y = 0.0, p2y = 0.0;

            if(block.minX < block.maxX) {
                //rising function, push points in direct order
                p1x = xMap.transform(block.minX);
                p2x = xMap.transform(block.maxX);
                p1y = yMap.transform(block.minY);
                p2y = yMap.transform(block.maxY);
            }
            else {
                //falling function, push points in reverse order
                p2x = xMap.transform(block.minX);
                p1x = xMap.transform(block.maxX);
                p2y = yMap.transform(block.minY);
                p1y = yMap.transform(block.maxY);
            }
            points[pixel*2+0].setX(p1x);
            points[pixel*2+0].setY(p1y);
            points[pixel*2+1].setX(p2x);
            points[pixel*2+1].setY(p2y);
        }
    }
    oldFrom = from;
    oldTo = to;
    oldXMap = xMap;
    oldYMap = yMap;
    cashedPolyline = polyline;
    return cashedPolyline;
}
