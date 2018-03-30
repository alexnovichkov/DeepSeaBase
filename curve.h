#ifndef CURVE_H
#define CURVE_H

#include <qwt_plot_curve.h>
#include <qwt_plot_marker.h>

class FileDescriptor;
class PointLabel;

//class Marker
//{
//public:
//    Marker(int point);
//    void setValue(const QPointF &value);
//    void moveTextTo(const QPointF &value);
//    void detach();
//    void attach(QwtPlot *plot);
//    void setLabel(const QwtText &label);



//    int d_selectedPoint;
//    int type; // 0 = частота
//              // 1 = частота + уровень
//              // 2 = уровень
//              // 3 = удаляем
//    //QwtPlotMarker *symbol;
//    QwtPlotMarker *text;
//};

class Channel;

#include "qwt_plot_item.h"
#include "qwt_scale_map.h"
#include <qglobal.h>

class QwtArrayPlotItem : public QwtPlotItem
{

public:
    QwtArrayPlotItem(const QwtText &title = QwtText());
    ~QwtArrayPlotItem();

    virtual int rtti() const
    {
        return QwtPlotItem::Rtti_PlotUserItem+1;
    }

    void draw( QPainter *painter,
        const QwtScaleMap &xMap, const QwtScaleMap &yMap,
        const QRectF &canvasRect ) const;
    QRectF boundingRect() const;

    void setData(double* data, quint32 size, qreal dt = 1.0);
    void setColor(QColor &color){m_plotColor = color;}
    QColor color(){return m_plotColor;}


private:
    quint32 m_size;
    double* m_data;
    double m_dt;


    QColor m_plotColor;
    mutable QRectF m_boundingRect;

};

class Curve : public QwtPlotCurve
{
public:
    Curve(const QString &title, FileDescriptor *descriptor, int channelIndex);
    Curve(const QString &title = QString::null);
    Curve(const QwtText &title);
    ~Curve();

    void setRawSamples(double x0, double xStep, const double *yData, int size);
    void setRawSamples(const double *xData, const double *yData, int size);
    void addLabel(PointLabel *label);
    void removeLabel(PointLabel *label);
    /** find label by canvas position */
    PointLabel *findLabel(const QPoint &pos, int yAxis);
    /** find label by point on a curve */
    PointLabel *findLabel(const int point);

    FileDescriptor *descriptor;
    int channelIndex;
    Channel *channel;
    QList<PointLabel*> labels;
    QPen oldPen;
};

#endif // CURVE_H
