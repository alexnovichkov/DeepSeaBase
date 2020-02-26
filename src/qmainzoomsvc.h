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

class ChartZoom;
class QMouseEvent;
class QRubberBand;

class QMainZoomSvc : public QObject
{
    Q_OBJECT

public:
    // конструктор
    explicit QMainZoomSvc();

    // прикрепление интерфейса к менеджеру масштабирования
    void attach(ChartZoom *);
signals:
    void xAxisClicked(double xValue, bool second);
protected:
    bool eventFilter(QObject *,QEvent *);

private:
    ChartZoom *zoom;
    QRubberBand *rubberBand;

    int startingPosX,startingPosY;        // Положение курсора в момент начала преобразования
                            // (в пикселах относительно канвы графика)

    void procMouseEvent(QEvent *);
    void procKeyboardEvent(QEvent *event);

    void startZoom(QMouseEvent *);
    void proceedZoom(QMouseEvent *);
    void endZoom(QMouseEvent *);
};

#endif // QMAINZOOMSVC_H
