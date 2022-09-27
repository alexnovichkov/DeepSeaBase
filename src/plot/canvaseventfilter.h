#ifndef CANVASEVENTFILTER_H
#define CANVASEVENTFILTER_H

#include <QObject>
#include <QPoint>
#include "enums.h"
#include <QtDebug>

class QwtPlotImpl;
class QMouseEvent;
class ZoomStack;
class DragZoom;
class WheelZoom;
class AxisZoom;
class PlotZoom;
class Picker;

#include "enums.h"

class CanvasEventFilter : public QObject
{
    Q_OBJECT
public:
    // Значения типа текущего преобразования графика
    // None - нет преобразования
    // Zoom - изменение масштаба
    // Drag - перемещение графика
    // Left - режим изменения левой границы
    // Right - режим изменения правой границы
    // Bottom - режим изменения нижней границы
    // Top - режим изменения верхней границы
    // Pick - перемещение объекта по графику
    enum class ActionType {None,
                           Zoom, // выделение
                           Drag, //перетаскивание
                           Axis, //на осях
                           Pick //перетаскивание объектов
                          };


    explicit CanvasEventFilter(QwtPlotImpl *parent);
    void setZoom(ZoomStack *zoom) {this->zoomStack = zoom;}
    void setDragZoom(DragZoom *zoom) {dragZoom = zoom;}
    void setWheelZoom(WheelZoom *zoom) {wheelZoom = zoom;}
    void setAxisZoom(AxisZoom *zoom) {axisZoom = zoom;}
    void setPicker(Picker *picker) {this->picker = picker;}
    void setPlotZoom(PlotZoom *zoom) {plotZoom = zoom;}
signals:
    void hover(Enums::AxisType axis, int hover); //0=none, 1=first half, 2 = second half
    void contextMenuRequested(const QPoint &pos, Enums::AxisType axis);
//    void moveCursor(Enums::Direction direction);
    void canvasDoubleClicked(const QPoint &pos);
protected:
    bool eventFilter(QObject *target, QEvent *event);
private:
    void procMouseEvent(QEvent *event);
    void procKeyboardEvent(QEvent *event);
    void procWheelEvent(Enums::AxisType axis, QEvent *event);
    void procAxisEvent(Enums::AxisType axis, QEvent *event);

    void mousePress(QMouseEvent *event);
    void mouseMove(QMouseEvent *event);
    void mouseRelease(QMouseEvent *event);
    void mouseDoubleClick(QMouseEvent *event);
    void applyWheel(QEvent *event, Enums::AxisType axis);

    QwtPlotImpl *plot;
    ZoomStack *zoomStack; //отвечает за запоминание изменений масштаба
    DragZoom *dragZoom; //отвечает за перетаскивание графика
    WheelZoom *wheelZoom; //отвечает за зум графика колесиком
    AxisZoom *axisZoom; //отвечает за изменение масштаба на осях
    PlotZoom *plotZoom; //отвечает за изменение масштаба выделением
    Picker *picker;

    ActionType actionType = ActionType::None;
    bool enabled = true;

    QPoint currentPosition;
};

QDebug operator<<(QDebug debug, const CanvasEventFilter::ActionType &c);

#endif // CANVASEVENTFILTER_H
