#ifndef CANVASEVENTFILTER_H
#define CANVASEVENTFILTER_H

#include <QObject>
#include <QPoint>
#include "qwt_axis_id.h"

class Plot;
class QMouseEvent;
class ChartZoom;
class DragZoom;
class WheelZoom;
class AxisZoom;

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
                           Zoom,
                           Drag,
                           Left,
                           Right,
                           Bottom,
                           Top,
                           Pick};


    explicit CanvasEventFilter(Plot *parent);
    void setZoom(ChartZoom *zoom) {this->zoom = zoom;}
    void setDragZoom(DragZoom *zoom) {this->dragZoom = zoom;}
    void setWheelZoom(WheelZoom *zoom) {this->wheelZoom = zoom;}
    void setAxisZoom(AxisZoom *zoom) {this->axisZoom = zoom;}
signals:
    void hover(QwtAxisId axis, int hover); //0=none, 1=first half, 2 = second half
    void contextMenuRequested(const QPoint &pos, QwtAxisId axis);
protected:
    bool eventFilter(QObject *target, QEvent *event);
private:
    void procMouseEvent(QEvent *event);
    void procKeyboardEvent(QEvent *event);
    void procWheelEvent(QwtAxisId axis, QEvent *event);
    void procAxisEvent(QwtAxisId axis, QEvent *event);

    void startPress(QMouseEvent *event);
    void moveMouse(QMouseEvent *event);
    void endPress(QMouseEvent *event);
    void applyWheel(QEvent *event, QwtAxisId axis);

    Plot *plot;
    ChartZoom *zoom;
    DragZoom *dragZoom;
    WheelZoom *wheelZoom;
    AxisZoom *axisZoom;

    ActionType actionType = ActionType::None;
    bool enabled = true;

    QPoint currentPosition;
};

#endif // CANVASEVENTFILTER_H
