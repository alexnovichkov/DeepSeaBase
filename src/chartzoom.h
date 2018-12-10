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

class QMainZoomSvc; // интерфейс масштабирования графика
class QDragZoomSvc; // интерфейс перемещения графика
class QWheelZoomSvc;
class QAxisZoomSvc;

class ChartZoom : public QObject
{
    Q_OBJECT

public:
    explicit ChartZoom(QwtPlot *);
    ~ChartZoom();

    // Значения типа текущего преобразования графика
    // ctNone - нет преобразования
    // ctZoom - изменение масштаба
    // ctDrag - перемещение графика
    // ctWheel - режим Wheel (изменение масштаба по обеим осям
        // вращением колеса мыши при нажатой клавише Ctrl)
    // ctVerWheel - режим VerWheel (изменение масштаба по вертикальной оси
        // вращением колеса мыши при нажатой левой клавише Shift)
    // ctHorWheel - режим HorWheel (изменение масштаба по горизонтальной оси
        // вращением колеса мыши при нажатой правой клавише Shift)
    // ctAxisHL - режим изменения левой границы
    // ctAxisHR - режим изменения правой границы
    // ctAxisVB - режим изменения нижней границы
    // ctAxisVT - режим изменения верхней границы
    // (значение, отличающееся от ctNone действует только пока
    // нажата левая или правая кнопка мыши, клавиша Ctrl или Shift)
    enum QConvType {ctNone,ctZoom,ctDrag,ctWheel,ctVerWheel,ctHorWheel,
                    ctAxisHL,ctAxisHR,ctAxisVB,ctAxisVT};

//    enum ZoomMode {
//        zoomBySelection = 1,
//        moveByWheel = 2,
//        moveByMouse = 4,
//        moveByAxis = 8
//    };

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
        explicit ScaleBounds(QwtPlot *,QwtPlot::Axis);

        QwtPlot::Axis axis;   // основная шкала

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
    QConvType regim();
    // переключение режима масштабирования
    void setRegime(QConvType);

    // указатель на опекаемый компонент QwtPlot
    QwtPlot *plot();
    // основная горизонтальная шкала
    QwtPlot::Axis masterH() const;
    // дополнительная горизонтальная шкала
    QwtPlot::Axis slaveH() const;
    // основная вертикальная шкала
    QwtPlot::Axis masterV() const;
    // дополнительная вертикальная шкала
    QwtPlot::Axis slaveV() const;

    void updatePlot();  // обновление графика

    void setZoomEnabled(bool enabled);

    void addZoom(const zoomCoordinates &coords, bool apply = false);
    void zoomBack();

    bool activated;
public slots:
    void labelSelected(bool selected);
signals:
    void updateTrackingCursor(double,bool);
    void contextMenuRequested(const QPoint &pos, const int axis);
protected:
    // обработчик всех событий
    bool eventFilter(QObject *,QEvent *);

private:


    QObject *mwin;          // Главное окно приложения
    QwtPlot *qwtPlot;          // Компонент QwtPlot, который отображает график

    // горизонтальная шкала
    QwtPlot::Axis masterX;  // основная
    QwtPlot::Axis slaveX;   // дополнительная
    // вертикальная шкала
    QwtPlot::Axis masterY;  // основная
    QwtPlot::Axis slaveY;   // дополнительная

    // Интерфейс масштабирования графика
    QMainZoomSvc *mainZoom;
    // Интерфейс перемещения графика
    QDragZoomSvc *dragZoom;

    QWheelZoomSvc *wheelZoom;
    QAxisZoomSvc *axisZoom;

    QConvType convType;     // Тип текущего преобразования графика

    // сохраняемый стэк масштабирования
    QStack<zoomCoordinates> zoomStack;

    // определение главного родителя
    QObject *generalParent(QObject *);
};


#endif // QWTCHARTZOOM_H
