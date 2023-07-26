#ifndef CANVASEVENTFILTER_H
#define CANVASEVENTFILTER_H

#include <QObject>
#include <QPoint>
#include "enums.h"
#include <QtDebug>

class QMouseEvent;
class ZoomStack;
class Picker;
class QWheelEvent;
class QCPAxis;
class QCPPlot;

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


    explicit CanvasEventFilter(QCPPlot *parent);
    void setZoom(ZoomStack *zoom) {this->zoomStack = zoom;}
    void setPicker(Picker *picker) {this->picker = picker;}
signals:
    void hover(Enums::AxisType axis, int hover); //0=none, 1=first half, 2 = second half
    void contextMenuRequested(const QPoint &pos, Enums::AxisType axis);
    void canvasDoubleClicked(const QPoint &pos);
    void axisDoubleClicked(QCPAxis* axis);
protected:
    bool eventFilter(QObject *target, QEvent *event);
private:
    void procMouseEvent(QMouseEvent *event);
    void procKeyboardEvent(QEvent *event);

    void mousePress(QMouseEvent *event);
    void mouseMove(QMouseEvent *event);
    void mouseRelease(QMouseEvent *event);
    void mouseDoubleClick(QMouseEvent *event);
    void mouseLeave(QMouseEvent *event);
    void applyWheel(QEvent *event, Enums::AxisType axis);

    QCPPlot *plot = nullptr;
    ZoomStack *zoomStack = nullptr; //отвечает за запоминание изменений масштаба
    Picker *picker = nullptr;

    ActionType actionType = ActionType::None;
    bool enabled = true;

    QPoint startPosition;
    QCPAxis* currentAxis = nullptr;
};

QDebug operator<<(QDebug debug, const CanvasEventFilter::ActionType &c);

#endif // CANVASEVENTFILTER_H
