#ifndef QAXISZOOMSVC_H
#define QAXISZOOMSVC_H

#include "chartzoom.h"

class Plot;

class AxisZoom : public QObject
{
    Q_OBJECT

public:
    explicit AxisZoom(Plot *plot);
    void startVerticalAxisZoom(QMouseEvent *event, QwtAxisId axis);
    void startHorizontalAxisZoom(QMouseEvent *event, QwtAxisId axis);
    ChartZoom::zoomCoordinates proceedAxisZoom(QMouseEvent *, QwtAxisId axis);
    ChartZoom::zoomCoordinates endAxisZoom(QMouseEvent *, QwtAxisId axis);
signals:
    void xAxisClicked(double xValue, bool second);
    void yAxisClicked(double xValue, bool second);
    void needsAutoscale(QwtAxisId axis);
    void hover(QwtAxisId axis, int hover); //0=none, 1=first half, 2 = second half

private:
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
    ChartZoom::zoomCoordinates axisApplyMove(QPoint, QwtAxisId);

    ChartZoom::ConvType ct = ChartZoom::ctNone;
};

#endif // QAXISZOOMSVC_H
