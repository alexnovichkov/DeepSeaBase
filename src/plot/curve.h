#ifndef CURVE_H
#define CURVE_H

#include <qwt_plot_curve.h>
#include <qwt_plot_marker.h>
#include <QPen>

class FileDescriptor;
class PointLabel;

class Channel;
class DataHolder;
class Plot;
class QwtScaleMap;
class PointMarker;

#include <qglobal.h>
#include "qwt_axis_id.h"
#include "selectable.h"

class Curve : public Selectable
{
public:
    enum class Type {
        Line,
        Spectrogram,
        Unknown
    };


    Curve(const QString &title, Channel *channel);
    virtual ~Curve();

    void attach(Plot *plot);

    virtual QString title() const = 0;
    virtual void setTitle(const QString &title) = 0;

    virtual QwtAxisId yAxis() const = 0;
    virtual void setYAxis(QwtAxisId axis) = 0;

    virtual QwtAxisId xAxis() const = 0;
    virtual void setXAxis(QwtAxisId axis) = 0;

    virtual QPen pen() const = 0;
    void setPen(const QPen &pen) {oldPen = pen; updatePen();}

    virtual QList<QwtLegendData> legendData() const = 0;

    virtual SamplePoint samplePoint(SelectedPoint point) const = 0;

    void setVisible(bool visible);

    void addLabel(PointLabel *label);
    void removeLabel(PointLabel *label);
    void removeLabels();
    /** find label by canvas position */
    PointLabel *findLabel(const QPoint &pos/*, QwtAxisId yAxis*/);
    /** find label by point on a curve */
    PointLabel *findLabel(SelectedPoint point);
    virtual SelectedPoint closest(const QPoint &pos, double *dist = nullptr, double *dist2 = nullptr) const = 0;

    virtual void moveToPos(QPoint pos, QPoint startPos = QPoint()) override;

    virtual double yMin() const;
    virtual double yMax() const;
    virtual double xMin() const;
    virtual double xMax() const;
    int samplesCount() const;

    void updateLabels();

    Channel *channel;
    QList<PointLabel*> labels;



    int fileNumber=0;
    bool duplicate;
    bool fixed = false;
    Type type = Type::Unknown;

public:
    QMap<int, QVariant> commonLegendData() const;
    void evaluateScale(int &from, int &to, const QwtScaleMap &xMap) const;
    void switchFixed();
    virtual void resetCashedData() {}

    // Selectable interface
public:
    virtual bool underMouse(const QPoint &pos, double *distanceX, double *distanceY,
                            SelectedPoint *point) const override;
    virtual void moveLeft(int count = 1) override;
    virtual void moveRight(int count = 1) override;
    virtual void moveUp(int count = 1) override;
    virtual void moveDown(int count = 1) override;
    virtual void fix() override;
    virtual void remove() override;
    virtual bool draggable() const override;
protected:
    virtual void attachTo(QwtPlot *plot) = 0;
    virtual void updateSelection(SelectedPoint point) override;
    inline virtual bool updateAnyway() const override {return true;}
    virtual void updatePen() = 0;
    Plot *m_plot = nullptr;
    mutable SelectedPoint selectedPoint;
    PointMarker *marker;
    QPen oldPen;
};





#endif // CURVE_H
