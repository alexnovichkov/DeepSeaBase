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

class QAxisZoomSvc : public QObject
{
    Q_OBJECT

public:
    // конструктор
    explicit QAxisZoomSvc();

    // прикрепление интерфейса к менеджеру масштабирования
    void attach(ChartZoom *);
signals:
    void xAxisClicked(double xValue, bool second);
    void contextMenuRequested(const QPoint &pos, const int axis);
    void needsAutoscale(int axis);
protected:
    // обработчик всех событий
    bool eventFilter(QObject *,QEvent *);

private:
    ChartZoom *zoom;     // Опекаемый менеджер масштабирования

    QCursor cursor;        // Буфер для временного хранения курсора

    double currentLeftBorder, currentRightBorder;   // Текущие границы графика по оси x в момент начала преобразования
    double currentBottomBorder, currentTopBorder;   // Текущие границы графика по оси y в момент начала преобразования
    double currentBottomBorder1, currentTopBorder1;

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
    // применение результатов перемещения границы шкалы
    void axisApplyMove(QPoint,int);

    // обработчик событий от мыши для шкалы
    void axisMouseEvent(QEvent *event, int axis);

    void procKeyboardEvent(QEvent *event);

    // обработчик нажатия на кнопку мыши над шкалой
    // (включение изменения масштаба шкалы)
    void startVerticalAxisZoom(QMouseEvent *event, int axis);
    void startHorizontalAxisZoom(QMouseEvent *event, int axis);
    // обработчик перемещения мыши
    // (выделение новых границ шкалы)
    void proceedAxisZoom(QMouseEvent *,int);
    // обработчик отпускания кнопки мыши
    // (выполнение изменения масштаба шкалы)
    void endAxisZoom(QMouseEvent *,int);

};

#endif // QAXISZOOMSVC_H
