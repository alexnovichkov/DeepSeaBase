#ifndef CURVE_H
#define CURVE_H

#include <qwt_plot_curve.h>
#include <qwt_plot_marker.h>

class FileDescriptor;
class PointLabel;

class Channel;
class DataHolder;
class QPen;
class Plot;
class QwtScaleMap;

#include <qglobal.h>

class Curve
{
public:
    Curve(const QString &title, FileDescriptor *descriptor, int channelIndex);
    virtual ~Curve();

    virtual void attachTo(QwtPlot *plot) = 0;

    virtual QString title() const = 0;
    virtual void setTitle(const QString &title) = 0;

    virtual int yAxis() const = 0;
    virtual void setYAxis(int axis) = 0;

    virtual QPen pen() const = 0;
    virtual void setPen(const QPen &pen) = 0;

    virtual QList<QwtLegendData> legendData() const = 0;

    virtual QPointF samplePoint(int point) const = 0;

    void setVisible(bool visible);

    void addLabel(PointLabel *label);
    void removeLabel(PointLabel *label);
    void removeLabels();
    /** find label by canvas position */
    PointLabel *findLabel(const QPoint &pos, int yAxis);
    /** find label by point on a curve */
    PointLabel *findLabel(const int point);
    virtual void resetHighlighting();
    virtual void highlight();
    virtual int closest(const QPoint &pos, double *dist = NULL) const = 0;

    double yMin() const;
    double yMax() const;
    virtual double xMin() const;
    virtual double xMax() const;
    int samplesCount() const;

    FileDescriptor *descriptor;
    int channelIndex;
    Channel *channel;
    QList<PointLabel*> labels;
    QPen oldPen;


    int fileNumber;
    bool duplicate;
    bool highlighted;
    bool fixed = false;
public:
    void evaluateScale(int &from, int &to, const QwtScaleMap &xMap) const;
    void switchFixed();
};





#endif // CURVE_H
