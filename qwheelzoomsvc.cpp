/**********************************************************/
/*                                                        */
/*            Реализация класса QWheelZoomSvc             */
/*                      Версия 1.0.3                      */
/*                                                        */
/* Разработал Мельников Сергей Андреевич,                 */
/* г. Каменск-Уральский Свердловской обл., 2012 г.,       */
/* при поддержке Ю. А. Роговского, г. Новосибирск.        */
/*                                                        */
/* Разрешается свободное использование и распространение. */
/* Упоминание автора обязательно.                         */
/*                                                        */
/**********************************************************/

#include "qwheelzoomsvc.h"
#include <QApplication>


// Конструктор
QWheelZoomSvc::QWheelZoomSvc() :
    QObject()
{
    // назначаем коэффициент, определяющий изменение масштаба графика
    // при вращении колеса мыши
    sfact = 1.2;
}

// Прикрепление интерфейса к менеджеру масштабирования
void QWheelZoomSvc::attach(QwtChartZoom *zm)
{
    // запоминаем указатель на менеджер масштабирования
    zoom = zm;
    // назначаем для графика обработчик событий (фильтр событий)
    zoom->plot()->installEventFilter(this);
}

// Задание коэффициента масштабирования графика
// при вращении колеса мыши (по умолчанию он равен 1.2)
void QWheelZoomSvc::setWheelFactor(double fact) {
    sfact = fact;
}

// Обработчик всех событий
bool QWheelZoomSvc::eventFilter(QObject *target, QEvent *event)
{
    //  if (zoom->activated)
    if (target == zoom->plot()) {
        if (event->type() == QEvent::Wheel) {
            procWheel(event);
            zoom->updatePlot();
        }
    }

    return QObject::eventFilter(target,event);
}


// Применение изменений по вращении колеса мыши
void QWheelZoomSvc::applyWheel(QEvent *event, bool wheelHorizontally, bool wheelVertically)
{
    // приводим тип QEvent к QWheelEvent
    QWheelEvent *wEvent = static_cast<QWheelEvent *>(event);

    if (wEvent->orientation() != Qt::Vertical) return;

    // определяем угол поворота колеса мыши
    // (значение 120 соответствует углу поворота 15°)
    const int wheelDelta = wEvent->delta();

    if (wheelDelta != 0) {   // если колесо вращалось, то
        // фиксируем исходные границы графика (если этого еще не было сделано)
        zoom->fixBounds();
        // получаем указатель на график
        QwtPlot *plot = zoom->plot();

        double factor;
        double wheelSteps = wheelDelta/120.0;
        factor = pow(0.85, wheelSteps);

        double horPos = plot->invTransform(zoom->masterH(), wEvent->pos().x());
        double verPos = plot->invTransform(zoom->masterV(), wEvent->pos().y());

        if (wheelHorizontally) {// если задано масштабирование по горизонтали
            // получаем карту основной горизонтальной шкалы
            QwtScaleMap sm = plot->canvasMap(zoom->masterH());

            double lower = (sm.s1()-horPos)*factor + horPos;
            double upper = (sm.s2()-horPos)*factor + horPos;
            zoom->horizontalScaleBounds->set(lower, upper,-1);
        }
        if (wheelVertically) // если задано масштабирование по вертикали
        {
            // получаем карту основной вертикальной шкалы
            QwtScaleMap sm = plot->canvasMap(zoom->masterV());
            double lower = (sm.s1()-verPos)*factor + verPos;
            double upper = (sm.s2()-verPos)*factor + verPos;
            zoom->verticalScaleBounds->set(lower, upper,-1);
        }
        // перестраиваем график (синхронно с остальными)
        plot->replot();
    }
}

// Обработчик вращения колеса мыши
void QWheelZoomSvc::procWheel(QEvent *event)
{
    // в зависимости от включенного режима вызываем
    // масштабирование с соответствующими параметрами
    QwtChartZoom::QConvType regime = QwtChartZoom::ctWheel;
    if (qApp->keyboardModifiers() & Qt::ControlModifier)
        regime = QwtChartZoom::ctHorWheel;
    if (qApp->keyboardModifiers() & Qt::ShiftModifier)
        regime = QwtChartZoom::ctVerWheel;

    switch (regime)
    {
            // масштабирование по обеим осям
    case QwtChartZoom::ctWheel: applyWheel(event,true,true); break;
            // масштабирование только по вертикальной оси
    case QwtChartZoom::ctVerWheel: applyWheel(event,false,true); break;
            // масштабирование только по горизонтальной оси
    case QwtChartZoom::ctHorWheel: applyWheel(event,true,false); break;
        // для прочих режимов ничего не делаем
    default: ;
    }
}
