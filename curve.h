#ifndef CURVE_H
#define CURVE_H

#include <qwt_plot_curve.h>
#include <qwt_plot_marker.h>

class FileDescriptor;
class PointLabel;

class Channel;
class DataHolder;

//#include "qwt_plot_item.h"
//#include "qwt_scale_map.h"
#include <qglobal.h>

class DfdData: public QwtSeriesData<QPointF>
{
public:
    DfdData(DataHolder *data);

    virtual QRectF boundingRect() const;

    virtual size_t size() const;

    virtual QPointF sample( size_t i ) const;

    virtual double xStep() const;
    virtual double xBegin() const;
private:
    DataHolder *data;
};

class Curve : public QwtPlotCurve
{
public:
    Curve();
    Curve(const QString &title, FileDescriptor *descriptor, int channelIndex);
    Curve(const QString &title = QString::null);
    Curve(const QwtText &title);
    ~Curve();

    void addLabel(PointLabel *label);
    void removeLabel(PointLabel *label);
    /** find label by canvas position */
    PointLabel *findLabel(const QPoint &pos, int yAxis);
    /** find label by point on a curve */
    PointLabel *findLabel(const int point);

    double yMin() const;
    double yMax() const;
    double xMin() const;
    double xMax() const;
    int samplesCount() const;

    FileDescriptor *descriptor;
    int channelIndex;
    Channel *channel;
    QList<PointLabel*> labels;
    QPen oldPen;


    int fileNumber;
    bool duplicate;
    DfdData *data;

    // QwtPlotCurve interface
protected:
    virtual void drawLines(QPainter *painter, const QwtScaleMap &xMap, const QwtScaleMap &yMap, const QRectF &canvasRect, int from, int to) const;

    // QwtPlotItem interface
public:
    virtual QList<QwtLegendData> legendData() const;
};

#endif // CURVE_H
