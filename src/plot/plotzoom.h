#ifndef QMAINZOOMSVC_H
#define QMAINZOOMSVC_H

/**********************************************************/
/*                                                        */
/*                   Класс QMainZoomSvc                   */
/*                      Версия 1.0.1                      */
/*                                                        */
/* Поддерживает интерфейс синхронного масштабирования     */
/* графика как одну из основных функций класса            */
/* QwtChartZoom.                                          */
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

#include <QObject>

#include <QEvent>

#include <QColor>
#include <QCursor>

#include "chartzoom.h"

class QMouseEvent;
class QRubberBand;
class Plot;

class PlotZoom : public QObject
{
    Q_OBJECT

public:
    // конструктор
    explicit PlotZoom(Plot *plot);

    void startZoom(QMouseEvent *);
    void proceedZoom(QMouseEvent *);
    ChartZoom::zoomCoordinates endZoom(QMouseEvent *);
private:
    QRubberBand *rubberBand = nullptr;
    Plot *plot;

    int startingPosX = 0;
    int startingPosY = 0;

    void procKeyboardEvent(QEvent *event);
};

#endif // QMAINZOOMSVC_H
