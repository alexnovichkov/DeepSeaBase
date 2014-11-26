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

class Curve : public QwtPlotCurve
{
public:
    Curve(const QString &title, FileDescriptor *descriptor, int channelIndex);
    Curve(const QString &title = QString::null);
    Curve(const QwtText &title);
    ~Curve();

    void setRawSamples(double x0, double xStep, const double *yData, int size);
    void addLabel(PointLabel *label);
    void removeLabel(PointLabel *label);
    /** find label by canvas position */
    PointLabel *findLabel(const QPoint &pos, int yAxis);
    /** find label by point on a curve */
    PointLabel *findLabel(const int point);

    FileDescriptor *descriptor;
    int channelIndex;
    Channel *channel;
    QString legend;
    QList<PointLabel*> labels;
    QPen oldPen;
};

#endif // CURVE_H
