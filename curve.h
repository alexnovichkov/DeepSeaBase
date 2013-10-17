#ifndef CURVE_H
#define CURVE_H

#include <qwt_plot_curve.h>
class DfdFileDescriptor;

class Curve : public QwtPlotCurve
{
public:
    Curve(const QString &title, DfdFileDescriptor *dfd, int channel);
    Curve(const QString &title = QString::null);
    Curve(const QwtText &title);
    void setRawSamples(double x0, double xStep, const double *yData, int size);

    DfdFileDescriptor *dfd;
    int channel;
    QString legend;
};

#endif // CURVE_H
