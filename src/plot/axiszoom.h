#ifndef QAXISZOOMSVC_H
#define QAXISZOOMSVC_H

#include "zoomstack.h"
#include "enums.h"
#include <QCursor>

class Plot;

class AxisZoom : public QObject
{
    Q_OBJECT

public:
    explicit AxisZoom(Plot *plot);
    void startVerticalAxisZoom(QMouseEvent *event, Enums::AxisType axis);
    void startHorizontalAxisZoom(QMouseEvent *event, Enums::AxisType axis);
    ZoomStack::zoomCoordinates proceedAxisZoom(QMouseEvent *, Enums::AxisType axis);
    ZoomStack::zoomCoordinates endAxisZoom(QMouseEvent *, Enums::AxisType axis);
signals:
    void axisClicked(const QPointF &value, bool second);
    void needsAutoscale(Enums::AxisType axis);
    void hover(Enums::AxisType axis, int hover); //0=none, 1=first half, 2 = second half

private:
    // Значения типа текущего преобразования графика
    // ctNone - нет преобразования
    // ctLeft - режим изменения левой границы
    // ctRight - режим изменения правой границы
    // ctBottom - режим изменения нижней границы
    // ctTop - режим изменения верхней границы
    enum class ConvType {ctNone,
                   ctLeft,
                   ctRight,
                   ctBottom,
                   ctTop};

    Plot *plot;

    QCursor cursor;        // Буфер для временного хранения курсора

    double currentLeftBorder, currentRightBorder;   // Текущие границы графика по оси x в момент начала преобразования
    double currentBottomBorder, currentTopBorder;   // Текущие границы графика по оси y в момент начала преобразования

    double currentWidth, currentHeight;   // Текущие ширина и высота графика в момент начала преобразования
    int currentLeftBorderInPixels, currentTopBorderInPixels;    // Текущие левое и верхнее смещение графика в момент начала преобразования
                            // (в пикселах относительно канвы)
    int currentPixelWidth, currentPixelHeight;      // Текущие ширина и высота графика в момент начала преобразования
                            // (в пикселах)
    int cursorPosX, cursorPosY;        // Положение курсора в момент начала преобразования
                            // (в пикселах относительно канвы графика за вычетом смещений графика)
    int currentLeftShiftInPixels, currentTopShiftInPixels;    // Текущие левое и верхнее смещение графика в момент начала преобразования
                            // (в пикселах относительно виджета шкалы)

    // ограничение нового размера шкалы
    double limitScale(double,double);
    ZoomStack::zoomCoordinates axisApplyMove(QPoint, Enums::AxisType);

    ConvType ct = ConvType::ctNone;
};

#endif // QAXISZOOMSVC_H
