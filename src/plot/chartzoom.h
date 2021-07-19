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

#include <qwt_plot.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_canvas.h>
#include <qwt_scale_draw.h>
#include <qwt_scale_map.h>
#include "enums.h"

class PlotZoom; // интерфейс масштабирования графика
class DragZoom; // интерфейс перемещения графика
class WheelZoom;
class AxisZoom;
class Plot;

class ChartZoom : public QObject
{
    Q_OBJECT

public:
    explicit ChartZoom(Plot *plot);
    ~ChartZoom();

    // Значения типа текущего преобразования графика
    // ctNone - нет преобразования
    // ctZoom - изменение масштаба
    // ctDrag - перемещение графика
    // ctLeft - режим изменения левой границы
    // ctRight - режим изменения правой границы
    // ctBottom - режим изменения нижней границы
    // ctTop - режим изменения верхней границы
    // ctPick - перемещение объекта по графику
    enum ConvType {ctNone,
                         ctZoom,
                         ctDrag,
                         ctLeft,
                         ctRight,
                         ctBottom,
                         ctTop,
                         ctPick};

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
        explicit ScaleBounds(Plot *plot, QwtAxisId axis);

        QwtAxisId axis;   // основная шкала

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
        QMap<int, QPointF> coords;
    };

    /**************************************************/

    // Контейнеры границ шкалы
    // (вертикальной и горизонтальной)
    ScaleBounds *horizontalScaleBounds,*verticalScaleBounds;
    ScaleBounds *verticalScaleBoundsSlave;

    // текущий режим масштабирования
    ConvType regime();
    // переключение режима масштабирования
    void setRegime(ConvType);

    // указатель на опекаемый компонент QwtPlot
    Plot *plot();

    void addZoom(zoomCoordinates coords, bool addToStack = false);
    void zoomBack();
    void moveToAxis(int axis, double min, double max);
    void autoscale(int axis, bool spectrogram);

    bool activated;

//    DragZoom *dragZoom = nullptr;
public slots:
    void setZoomEnabled(bool enabled);
signals:
    void updateTrackingCursorX(double,bool);
    void updateTrackingCursorY(double,bool);
    void contextMenuRequested(const QPoint &pos, QwtAxisId axis);
    void moveCursor(Enums::Direction direction);
    void hover(QwtAxisId axis, int hover);
    void setPickerEnabled(bool);
private:
    Plot *qwtPlot;

    PlotZoom *mainZoom;

//    WheelZoom *wheelZoom;
//    AxisZoom *axisZoom;

    ConvType convType;     // Тип текущего преобразования графика

    // сохраняемый стэк масштабирования
    QStack<zoomCoordinates> zoomStack;
};


#endif // QWTCHARTZOOM_H
