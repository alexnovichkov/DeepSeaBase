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

#include "chartzoom.h"

class WheelZoom : public QObject
{
    Q_OBJECT
public:
    explicit WheelZoom();
    void attach(ChartZoom *);
protected:
    bool eventFilter(QObject *,QEvent *);
private:
    ChartZoom *zoom;
    void applyWheel(QEvent *, QwtAxisId axis);
    QPointF getCoords(QwtAxisId axis, int pos, double factor);
};

#endif // QWHEELZOOMSVC_H
