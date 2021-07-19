#ifndef QDRAGZOOMSVC_H
#define QDRAGZOOMSVC_H


#include <QObject>

#include "zoomstack.h"
class QMouseEvent;
class Plot;

#include <QCursor>

class DragZoom : public QObject
{
    Q_OBJECT

public:
    explicit DragZoom(Plot *plot);
    void startDrag(QMouseEvent *);
    ZoomStack::zoomCoordinates proceedDrag(QMouseEvent *);
    ZoomStack::zoomCoordinates endDrag(QMouseEvent *);
private:
    Plot *plot;
    QCursor tCursor;        // Буфер для временного хранения курсора

    double minHorizontalBound, maxHorizontalBound;   // Текущие границы графика по горизонтальной оси
                            // в момент начала преобразования
    double minVerticalBound, maxVerticalBound;   // Текущие границы графика по главной вертикальной оси
                            // в момент начала преобразования
    double minVerticalBound1, maxVerticalBound1;   // Текущие границы графика по вспомогательной вертикальной оси
                            // в момент начала преобразования
    QPoint position;        // Положение курсора в момент начала преобразования
                            // (в пикселах относительно канвы графика)
    double dx, dy, dy1; //Текущее смещение графика
};

#endif // QDRAGZOOMSVC_H
