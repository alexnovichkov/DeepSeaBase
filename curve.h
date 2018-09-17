#ifndef CURVE_H
#define CURVE_H

#include <qwt_plot_curve.h>
#include <qwt_plot_marker.h>

class FileDescriptor;
class PointLabel;

class Channel;

//#include "qwt_plot_item.h"
//#include "qwt_scale_map.h"
#include <qglobal.h>

class DfdData: public QwtSeriesData<QPointF>
{
public:
    DfdData(double x0, double xStep, const double *y, size_t size);
    DfdData(const double *x, const double *y, size_t size);

    virtual QRectF boundingRect() const;

    virtual size_t size() const;

    virtual QPointF sample( size_t i ) const;

    virtual double xStep() const;
    virtual double xBegin() const;
private:
    double d_x0;
    double d_xStep;
    const double *d_y;
    const double *d_x;
    size_t d_size;
};

//class QwtArrayPlotItem : public QwtPlotItem
//{

//public:
//    QwtArrayPlotItem(const QwtText &title = QwtText());
//    ~QwtArrayPlotItem();

//    virtual int rtti() const
//    {
//        return QwtPlotItem::Rtti_PlotUserItem+1;
//    }

//    void draw( QPainter *painter,
//        const QwtScaleMap &xMap, const QwtScaleMap &yMap,
//        const QRectF &canvasRect ) const;
//    QRectF boundingRect() const;

//    void setData(double* data, quint32 size, qreal dt = 1.0);
//    void setColor(QColor &color){m_plotColor = color;}
//    QColor color(){return m_plotColor;}


//private:
//    quint32 m_size;
//    double* m_data;
//    double m_dt;


//    QColor m_plotColor;
//    mutable QRectF m_boundingRect;

//};

class Curve : public QwtPlotCurve
{
public:
    Curve();
    Curve(const QString &title, FileDescriptor *descriptor, int channelIndex);
    Curve(const QString &title = QString::null);
    Curve(const QwtText &title);
    ~Curve();

    void setRawSamples();

    void addLabel(PointLabel *label);
    void removeLabel(PointLabel *label);
    /** find label by canvas position */
    PointLabel *findLabel(const QPoint &pos, int yAxis);
    /** find label by point on a curve */
    PointLabel *findLabel(const int point);

    bool isSimplified() const;

    FileDescriptor *descriptor;
    int channelIndex;
    Channel *channel;
    QList<PointLabel*> labels;
    QPen oldPen;
    double xMin, xMax;
    double yMin, yMax;
    int samplesCount;

private:
    void setRawSamples(double x0, double xStep, const double *yData, int size);
    void setRawSamples(const double *xData, const double *yData, int size);

    QVector<double> xDataSimplified;
    QVector<double> yDataSimplified;
    bool m_simplified;


};

#endif // CURVE_H
