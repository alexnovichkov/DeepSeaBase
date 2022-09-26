/**********************************************************/
/*                                                        */
/*                   Класс QwtChartZoom                   */
/*                      Версия 1.5.2                      */
/*                                                        */
/* Обеспечивает интерфейс изменения масштаба и границ 	  */
/* графика QwtPlot в стиле компонента TChart (Delphi,     */
/* C++Builder).                                           */
/*                                                        */
/* Разработал Мельников Сергей Андреевич,                 */
/* г. Каменск-Уральский Свердловской обл., 2012 г.,       */
/* при поддержке Ю. А. Роговского, г. Новосибирск.        */
/*                                                        */
/* Разрешается свободное использование и распространение. */
/* Упоминание автора обязательно.                         */
/*                                                        */
/**********************************************************/

#ifndef QWTCHARTZOOM_H
#define QWTCHARTZOOM_H

#include <QEvent>
#include <QMouseEvent>
#include <QTimer>
#include <QStack>
#include <QRectF>

#include "enums.h"

class Plot;

class ZoomStack : public QObject
{
    Q_OBJECT

public:
    explicit ZoomStack(Plot *plot);
    ~ZoomStack();

    /**************************************************/
    /*               Класс QScaleBounds               */
    /*                  Версия 1.0.1                  */
    /*                                                */
    /* Содержит исходные границы основной шкалы и     */
    /* соотношение между основной и дополнительной    */
    /* шкалой.                                        */

    class ScaleBounds
    {
    public:
        // конструктор
        explicit ScaleBounds(Plot *plot, Enums::AxisType axis);

         Enums::AxisType axis;   // основная шкала

        bool isFixed() const {return fixed;}
        void setFixed(bool fixed);
        // фиксация исходных границ шкалы
        void add(double min, double max);
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


    ScaleBounds *horizontalScaleBounds,*verticalScaleBounds;
    ScaleBounds *verticalScaleBoundsSlave;

    void addZoom(zoomCoordinates coords, bool addToStack = false);
    void zoomBack();
    void moveToAxis(Enums::AxisType axis, double min, double max);
    void autoscale(Enums::AxisType axis, bool spectrogram);
private:
    Plot *plot;
    QStack<zoomCoordinates> zoomStack;
};


#endif // QWTCHARTZOOM_H
