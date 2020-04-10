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

class PlotZoom; // интерфейс масштабирования графика
class DragZoom; // интерфейс перемещения графика
class WheelZoom;
class AxisZoom;

class ChartZoom : public QObject
{
    Q_OBJECT

public:
    explicit ChartZoom(QwtPlot *plot);
    ~ChartZoom();

    // Значения типа текущего преобразования графика
    // ctNone - нет преобразования
    // ctZoom - изменение масштаба
    // ctDrag - перемещение графика
    // ctLeft - режим изменения левой границы
    // ctRight - режим изменения правой границы
    // ctBottom - режим изменения нижней границы
    // ctTop - режим изменения верхней границы
    enum ConvType {ctNone,ctZoom,ctDrag,
                    ctLeft, ctRight, ctBottom, ctTop};

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
        explicit ScaleBounds(QwtPlot *plot, QwtAxisId axis);

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

        QwtPlot *plot;          // опекаемый график

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
    QwtPlot *plot();

    void addZoom(const zoomCoordinates &coords, bool apply = false);
    void zoomBack();

    bool activated;
public slots:
    void setZoomEnabled(bool enabled);
signals:
    void updateTrackingCursor(double,bool);
    void contextMenuRequested(const QPoint &pos, QwtAxisId axis);
    void moveCursor(bool right);
protected:
    // обработчик всех событий
    bool eventFilter(QObject *,QEvent *);

private:


    QObject *mwin;          // Главное окно приложения
    QwtPlot *qwtPlot;          // Компонент QwtPlot, который отображает график

    // Интерфейс масштабирования графика
    PlotZoom *mainZoom;
    // Интерфейс перемещения графика
    DragZoom *dragZoom;

    WheelZoom *wheelZoom;
    AxisZoom *axisZoom;

    ConvType convType;     // Тип текущего преобразования графика

    // сохраняемый стэк масштабирования
    QStack<zoomCoordinates> zoomStack;

    // определение главного родителя
    QObject *generalParent(QObject *);
};


#endif // QWTCHARTZOOM_H
