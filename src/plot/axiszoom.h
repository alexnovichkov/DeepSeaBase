/**********************************************************/
/*                                                        */
/*                   Класс QAxisZoomSvc                   */
/*                      Версия 1.2.2                      */
/*                                                        */
/* Поддерживает интерфейс изменения одной из границ       */
/* графика, является дополнением к классу QwtChartZoom,   */
/* начиная с версии 1.5.0.                                */
/*                                                        */
/* Разработал Мельников Сергей Андреевич,                 */
/* г. Каменск-Уральский Свердловской обл., 2012 г.,       */
/* при поддержке Ю. А. Роговского, г. Новосибирск.        */
/*                                                        */
/* Разрешается свободное использование и распространение. */
/* Упоминание автора обязательно.                         */
/*                                                        */
/**********************************************************/

#ifndef QAXISZOOMSVC_H
#define QAXISZOOMSVC_H

#include "chartzoom.h"

class AxisZoom : public QObject
{
    Q_OBJECT

public:
    explicit AxisZoom();
    void attach(ChartZoom *);
signals:
    void xAxisClicked(double xValue, bool second);
    void contextMenuRequested(const QPoint &pos, QwtAxisId axis);
    void needsAutoscale(QwtAxisId axis);
    void moveCursor(bool right);
    void hover(QwtAxisId axis, int hover); //0=none, 1=first half, 2 = second half
protected:
    bool eventFilter(QObject *,QEvent *);

private:
    ChartZoom *zoom;     // Опекаемый менеджер масштабирования

    QCursor cursor;        // Буфер для временного хранения курсора

    double currentLeftBorder, currentRightBorder;   // Текущие границы графика по оси x в момент начала преобразования
    double currentBottomBorder, currentTopBorder;   // Текущие границы графика по оси y в момент начала преобразования

    double currentWidth, currentHeight;   // Текущие ширина и высота графика в момент начала преобразования
    int scb_pxl, scb_pyt;    // Текущие левое и верхнее смещение графика в момент начала преобразования
                            // (в пикселах относительно канвы)
    int currentPixelWidth, currentPixelHeight;      // Текущие ширина и высота графика в момент начала преобразования
                            // (в пикселах)
    int cursorPosX, cursorPosY;        // Положение курсора в момент начала преобразования
                            // (в пикселах относительно канвы графика за вычетом смещений графика)
    int sab_pxl, sab_pyt;    // Текущие левое и верхнее смещение графика в момент начала преобразования
                            // (в пикселах относительно виджета шкалы)

    // ограничение нового размера шкалы
    double limitScale(double,double);
    void axisApplyMove(QPoint, QwtAxisId);

    void axisMouseEvent(QEvent *event, QwtAxisId axis);
    void procKeyboardEvent(QEvent *event);

    void startVerticalAxisZoom(QMouseEvent *event, QwtAxisId axis);
    void startHorizontalAxisZoom(QMouseEvent *event, QwtAxisId axis);
    void proceedAxisZoom(QMouseEvent *, QwtAxisId axis);
    void endAxisZoom(QMouseEvent *, QwtAxisId axis);

};

#endif // QAXISZOOMSVC_H
