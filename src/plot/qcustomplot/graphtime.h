#ifndef GRAPHTIME_H
#define GRAPHTIME_H

#include "graph2d.h"

class PointMapper;

class GraphTime : public Graph2D
{
    Q_OBJECT
public:
    GraphTime(const QString &title, Channel *channel, QCPAxis *keyAxis, QCPAxis *valueAxis);
    ~GraphTime();

    // Graph2D interface
protected:
    virtual void getOptimizedLineData(QVector<QCPGraphData> *lineData, const int begin, const int end) const override;
    virtual void draw(QCPPainter *painter) override;
};





#endif // GRAPHTIME_H
