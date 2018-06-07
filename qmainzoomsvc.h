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

class QwtChartZoom;
class QMouseEvent;
class QRubberBand;

class QMainZoomSvc : public QObject
{
    Q_OBJECT

public:
    // конструктор
    explicit QMainZoomSvc();

    // прикрепление интерфейса к менеджеру масштабирования
    void attach(QwtChartZoom *);
    void detach();
signals:
    void xAxisClicked(double xValue, bool second);
protected:
    // обработчик всех событий
    bool eventFilter(QObject *,QEvent *);

private:
    QwtChartZoom *zoom;     // Опекаемый менеджер масштабирования
    QRubberBand *zwid;
    QCursor tCursor;        // Буфер для временного хранения курсора

    int scp_x,scp_y;        // Положение курсора в момент начала преобразования
                            // (в пикселах относительно канвы графика)

    // обработчик обычных событий от мыши
    void procMouseEvent(QEvent *);

    void procKeyboardEvent(QEvent *event);

    // обработчик нажатия на кнопку мыши
    // (включение изменения масштаба)
    void startZoom(QMouseEvent *);
    // обработчик перемещения мыши
    // (выделение новых границ графика)
    void selectZoomRect(QMouseEvent *);
    // обработчик отпускания кнопки мыши
    // (выполнение изменения масштаба)
    void procZoom(QMouseEvent *);
};

#endif // QMAINZOOMSVC_H
