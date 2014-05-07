#ifndef CURVE_H
#define CURVE_H

#include <qwt_plot_curve.h>
#include <qwt_plot_marker.h>

class DfdFileDescriptor;

class Marker
{
public:
    Marker(int point);
    void setValue(const QPointF &value);
    void moveTextTo(const QPointF &value);
    void detach();
    void attach(QwtPlot *plot);
    void setLabel(const QwtText &label);



    int d_selectedPoint;
    int type; // 0 = частота
              // 1 = частота + уровень
              // 2 = уровень
              // 3 = удаляем
    //QwtPlotMarker *symbol;
    QwtPlotMarker *text;
};

class Curve : public QwtPlotCurve
{
public:
    Curve(const QString &title, DfdFileDescriptor *dfd, int channel);
    Curve(const QString &title = QString::null);
    Curve(const QwtText &title);
    ~Curve();

    void setRawSamples(double x0, double xStep, const double *yData, int size);
    Marker *addMarker(int index);
    void removeMarker(int index);
    Marker *findMarker(int index);

    DfdFileDescriptor *dfd;
    int channel;
    QString legend;
    QList<Marker*> markers;
};

#endif // CURVE_H
