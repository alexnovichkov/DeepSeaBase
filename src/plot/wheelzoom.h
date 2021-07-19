/**********************************************************/
/*                                                        */
/*                  Класс QWheelZoomSvc                   */
/*                      Версия 1.0.3                      */
/*                                                        */
/* Поддерживает интерфейс синхронного изменения масштаба  */
/* графиков вращением колеса мыши, является дополнением   */
/* к классу QwtChartZoom, начиная с версии 1.5.0.         */
/*                                                        */
/* Разработал Мельников Сергей Андреевич,                 */
/* г. Каменск-Уральский Свердловской обл., 2012 г.,       */
/* при поддержке Ю. А. Роговского, г. Новосибирск.        */
/*                                                        */
/* Разрешается свободное использование и распространение. */
/* Упоминание автора обязательно.                         */
/*                                                        */
/**********************************************************/

#ifndef QWHEELZOOMSVC_H
#define QWHEELZOOMSVC_H

class Plot;
#include <QObject>
#include "qwt_axis_id.h"
#include "zoomstack.h"

class WheelZoom : public QObject
{
    Q_OBJECT
public:
    explicit WheelZoom(Plot *plot);
    ZoomStack::zoomCoordinates applyWheel(QEvent *, QwtAxisId axis);
private:
    Plot *plot;
    QPointF getCoords(QwtAxisId axis, int pos, double factor);
};

#endif // QWHEELZOOMSVC_H
