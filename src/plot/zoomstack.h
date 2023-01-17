#ifndef ZOOMSTACK_H
#define ZOOMSTACK_H

#include <QEvent>
#include <QMouseEvent>
#include <QTimer>
#include <QStack>
#include <QRectF>
#include <QtDebug>

#include "enums.h"

class Plot;

class ZoomStack : public QObject
{
    Q_OBJECT

public:
    explicit ZoomStack(Plot *m_plot);
    ~ZoomStack();

    class ScaleBounds
    {
    public:
        // конструктор
        explicit ScaleBounds(Plot *plot, Enums::AxisType axis);

         Enums::AxisType axis;   // основная шкала

        bool isFixed() const {return fixed;}
        void setFixed(bool fixed);
        // фиксация исходных границ шкалы
        void add(double min, double max, bool removePrevious = false);
        // установка заданных границ шкалы
        void set(double min, double max);
        // восстановление исходных границ шкалы
        void reset();
        void autoscale();
        void removeToAutoscale(double min, double max);
    private:
        QList<double> mins;
        QList<double> maxes;
        double min;
        double max;

        Plot *plot;          // опекаемый график

        bool fixed;             // признак фиксации границ
    };

    struct zoomCoordinates
    {
        QMap<Enums::AxisType, QPointF> coords;
    };

    ScaleBounds *scaleBounds(Enums::AxisType axis);

    void addZoom(zoomCoordinates coords, bool addToStack = false);
    void zoomBack();
    void moveToAxis(Enums::AxisType axis, double min, double max);
    void autoscale(Enums::AxisType axis);
signals:
    void replotNeeded();
private:
    Plot *m_plot;
    QStack<zoomCoordinates> m_zoomStack;
    QMap<Enums::AxisType, ScaleBounds*> m_scaleBounds;
};

inline QDebug operator<<(QDebug deb, ZoomStack::zoomCoordinates coords)
{
    deb << coords.coords;
    return deb;
}


#endif // ZOOMSTACK_H
