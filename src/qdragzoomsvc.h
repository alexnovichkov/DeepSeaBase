#ifndef QDRAGZOOMSVC_H
#define QDRAGZOOMSVC_H


#include <QObject>

/**********************************************************/
/*                                                        */
/*                   Класс QDragZoomSvc                   */
/*                      Версия 1.0.1                      */
/*                                                        */
/* Поддерживает интерфейс синхронного перемещения графика */
/* как одну из основных функций класса QwtChartZoom.      */
/* Выделен в отдельный класс, начиная с версии 1.4.0.     */
/*                                                        */
/* Разработал Мельников Сергей Андреевич,                 */
/* г. Каменск-Уральский Свердловской обл., 2012 г.,       */
/* при поддержке Ю. А. Роговского, г. Новосибирск.        */
/*                                                        */
/* Разрешается свободное использование и распространение. */
/* Упоминание автора обязательно.                         */
/*                                                        */
/**********************************************************/

class ChartZoom;
class QMouseEvent;
#include <QCursor>

class QDragZoomSvc : public QObject
{
    Q_OBJECT

public:
    explicit QDragZoomSvc();
    void attach(ChartZoom *);
protected:
    bool eventFilter(QObject *,QEvent *);
private:
    ChartZoom *zoom;     // Опекаемый менеджер масштабирования
    QCursor tCursor;        // Буфер для временного хранения курсора

    double minHorizontalBound, maxHorizontalBound;   // Текущие границы графика по горизонтальной оси
                            // в момент начала преобразования
    double minVerticalBound, maxVerticalBound;   // Текущие границы графика по главной вертикальной оси
                            // в момент начала преобразования
    double minVerticalBound1, maxVerticalBound1;   // Текущие границы графика по вспомогательной вертикальной оси
                            // в момент начала преобразования
    double horizontalFactor, verticalFactor, verticalFactor1;     // Текущие масштабирующие множители по обеим осям
                            // (изменение координаты при перемещении на один пиксел)
    int horizontalCursorPosition,verticalCursorPosition;        // Положение курсора в момент начала преобразования
                            // (в пикселах относительно канвы графика)
    double dx, dy, dy1; //Текущее смещение графика

    void applyDrag(QPoint mousePos, bool moveRightAxis);

    void dragMouseEvent(QEvent *);

    void startDrag(QMouseEvent *);
    void proceedDrag(QMouseEvent *);
    void endDrag(QMouseEvent *);
};

#endif // QDRAGZOOMSVC_H
