#include "qmainzoomsvc.h"

/**********************************************************/
/*                                                        */
/*             Реализация класса QMainZoomSvc             */
/*                      Версия 1.0.1                      */
/*                                                        */
/* Разработал Мельников Сергей Андреевич,                 */
/* г. Каменск-Уральский Свердловской обл., 2012 г.,       */
/* при поддержке Ю. А. Роговского, г. Новосибирск.        */
/*                                                        */
/* Разрешается свободное использование и распространение. */
/* Упоминание автора обязательно.                         */
/*                                                        */
/**********************************************************/

#include "logging.h"
#include "chartzoom.h"
#include <QRubberBand>

// Конструктор
QMainZoomSvc::QMainZoomSvc() :
    QObject()
{DD;
    // очищаем виджет, отвечающий за отображение выделенной области
    rubberBand = 0;
}

// Прикрепление интерфейса к менеджеру масштабирования
void QMainZoomSvc::attach(ChartZoom *zm)
{DD;
    // запоминаем указатель на менеджер масштабирования
    zoom = zm;
    // назначаем для графика обработчик событий (фильтр событий)
    zoom->plot()->canvas()->installEventFilter(this);
}

void QMainZoomSvc::detach()
{
    if (zoom) zoom->plot()->canvas()->removeEventFilter(this);
}

// Обработчик всех событий
bool QMainZoomSvc::eventFilter(QObject *target,QEvent *event)
{
    if (zoom->activated) {
        // если событие произошло для графика, то
        if (target == zoom->plot()->canvas()) {
            // если произошло одно из событий от мыши, то
            if (event->type() == QEvent::MouseButtonPress ||
                event->type() == QEvent::MouseMove ||
                event->type() == QEvent::MouseButtonRelease ||
                event->type() == QEvent::MouseButtonDblClick)
                procMouseEvent(event);
            else if (event->type() == QEvent::KeyPress) {
                procKeyboardEvent(event);
            }
        }
    }
    // передаем управление стандартному обработчику событий
    return QObject::eventFilter(target,event);
}

// Обработчик обычных событий от мыши
void QMainZoomSvc::procMouseEvent(QEvent *event)
{DD;
    // создаем указатель на событие от мыши
    QMouseEvent *mEvent = static_cast<QMouseEvent *>(event);
    // в зависимости от типа события вызываем соответствующий обработчик
    switch (event->type())
    {
        // нажата кнопка мыши
        case QEvent::MouseButtonPress:
            startZoom(mEvent);
            break;
            // перемещение мыши
        case QEvent::MouseMove:
            selectZoomRect(mEvent);
            break;
            // отпущена кнопка мыши
        case QEvent::MouseButtonRelease:
            finishZoom(mEvent);
            break;
            // для прочих событий ничего не делаем
        default: ;
    }
}

void QMainZoomSvc::procKeyboardEvent(QEvent *event)
{DD;
    QKeyEvent *kEvent = static_cast<QKeyEvent*>(event);
    switch (kEvent->key()) {
        case Qt::Key_Backspace: {
            zoom->zoomBack();
            break;
        }
        case Qt::Key_Escape: {
            if (zoom->regim() == ChartZoom::ctZoom) {
                // восстанавливаем курсор
//                zoom->plot()->canvas()->setCursor(tCursor);
                // удаляем виджет, отображающий выделенную область
                if (rubberBand) rubberBand->hide();
                zoom->setRegime(ChartZoom::ctNone);
            }
            break;
        }
        default: break;
    }
}

// Обработчик нажатия на кнопку мыши
// (включение изменения масштаба)
void QMainZoomSvc::startZoom(QMouseEvent *mEvent)
{DD;
    // если в данный момент еще не включен ни один из режимов
    if (zoom->regim() == ChartZoom::ctNone)
    {
        // получаем геометрию канвы графика
        QRect cg = zoom->plot()->canvas()->geometry();
        // определяем текущее положение курсора (относительно канвы графика)
        startingPosX = mEvent->pos().x();
        startingPosY = mEvent->pos().y();
        // если курсор находится над канвой графика
        if (startingPosX >= 0 && startingPosX < cg.width() &&
            startingPosY >= 0 && startingPosY < cg.height()) {
            // если нажата левая кнопка мыши, то
            if (mEvent->button() == Qt::LeftButton)
            {
                // прописываем соответствующий признак режима
                zoom->setRegime(ChartZoom::ctZoom);
                // создаем виджет, который будет отображать выделенную область
                // (он будет прорисовываться на том же виджете, что и график)
                if (!rubberBand) {
                    rubberBand = new QRubberBand(QRubberBand::Rectangle, zoom->plot()->canvas());
                }
            }
        }
    }
}

// Обработчик перемещения мыши
// (выделение новых границ графика)
void QMainZoomSvc::selectZoomRect(QMouseEvent *mEvent)
{DD;
    // если включен режим изменения масштаба, то
    if (zoom->regim() == ChartZoom::ctZoom)
    {
        int dx = mEvent->pos().x() - startingPosX;
        if (dx == 0) dx = 1;

        int dy = mEvent->pos().y() - startingPosY;
        if (dy == 0) dy = 1;
        // отображаем выделенную область
        if (rubberBand) {
            rubberBand->setGeometry(QRect(startingPosX, startingPosY, dx,dy).normalized());
            rubberBand->show();
        }
    }
}

// Обработчик отпускания кнопки мыши
// (выполнение изменения масштаба)
void QMainZoomSvc::finishZoom(QMouseEvent *mEvent)
{DD;
    // если включен режим изменения масштаба или режим перемещения графика
    if (zoom->regim() == ChartZoom::ctZoom) {
        // если отпущена левая кнопка мыши, то
        if (mEvent->button() == Qt::LeftButton)
        {
            QwtPlot *plt = zoom->plot();
            // удаляем виджет, отображающий выделенную область
            if (rubberBand) rubberBand->hide();
            // определяем положение курсора, т.е. координаты xp и yp
            // конечной точки выделенной области (в пикселах относительно канвы QwtPlot)
            int xp = mEvent->pos().x();
            int yp = mEvent->pos().y();

            // если был одинарный щелчок мышью, то трактуем как установку курсора
            if (xp-startingPosX==0 && yp-startingPosY==0) {
                emit xAxisClicked(zoom->plot()->canvasMap(QwtPlot::xBottom).invTransform(startingPosX),
                                  mEvent->modifiers() & Qt::ControlModifier);
            }

            if (qAbs(xp - startingPosX) >= 8 && qAbs(yp - startingPosY) >= 8)
            {
                int leftmostX = qMin(xp, startingPosX); int rightmostX = qMax(xp, startingPosX);
                int leftmostY = qMin(yp, startingPosY); int rightmostY = qMax(yp, startingPosY);
                QwtPlot::Axis mX = zoom->masterH();
                const double xMin = plt->invTransform(mX,leftmostX);
                const double xMax = plt->invTransform(mX,rightmostX);

                QwtPlot::Axis mY = zoom->masterV();
                double yMin = plt->invTransform(mY,rightmostY);
                double yMax = plt->invTransform(mY,leftmostY);

                mY = zoom->slaveV();
                double ySMin = plt->invTransform(mY,rightmostY);
                double ySMax = plt->invTransform(mY,leftmostY);

                ChartZoom::zoomCoordinates coords;
                coords.coords.insert(zoom->masterH(), {xMin, xMax});
                coords.coords.insert(zoom->masterV(), {yMin, yMax});
                coords.coords.insert(zoom->slaveV(), {ySMin, ySMax});
                zoom->addZoom(coords, true);
            }
            // очищаем признак режима
            zoom->setRegime(ChartZoom::ctNone);
        }
    }
}

