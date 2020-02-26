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

QMainZoomSvc::QMainZoomSvc() : QObject()
{DD;
    rubberBand = 0;
}

// Прикрепление интерфейса к менеджеру масштабирования
void QMainZoomSvc::attach(ChartZoom *zm)
{DD;
    zoom = zm;
    zoom->plot()->canvas()->installEventFilter(this);
}

// Обработчик всех событий
bool QMainZoomSvc::eventFilter(QObject *target,QEvent *event)
{
    if (zoom->activated) {
        if (target == zoom->plot()->canvas()) {
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
    return QObject::eventFilter(target,event);
}

void QMainZoomSvc::procMouseEvent(QEvent *event)
{
    QMouseEvent *mEvent = static_cast<QMouseEvent *>(event);
    switch (event->type())  {
        case QEvent::MouseButtonPress:
            startZoom(mEvent);
            break;
        case QEvent::MouseMove:
            proceedZoom(mEvent);
            break;
        case QEvent::MouseButtonRelease:
            endZoom(mEvent);
            break;
        default: ;
    }
}

void QMainZoomSvc::procKeyboardEvent(QEvent *event)
{
    QKeyEvent *kEvent = static_cast<QKeyEvent*>(event);
    switch (kEvent->key()) {
        case Qt::Key_Backspace: {
            zoom->zoomBack();
            break;
        }
        case Qt::Key_Escape: {//прерывание выделения
            if (zoom->regime() == ChartZoom::ctZoom) {
                if (rubberBand) rubberBand->hide();
                zoom->setRegime(ChartZoom::ctNone);
            }
            break;
        }
        default: break;
    }
}

void QMainZoomSvc::startZoom(QMouseEvent *mEvent)
{
    if (zoom->regime() == ChartZoom::ctNone) {
        QRect cg = zoom->plot()->canvas()->geometry();

        startingPosX = mEvent->pos().x();
        startingPosY = mEvent->pos().y();

        if (startingPosX >= 0 && startingPosX < cg.width() &&
            startingPosY >= 0 && startingPosY < cg.height()) {
            if (mEvent->button() == Qt::LeftButton) {
                zoom->setRegime(ChartZoom::ctZoom);
                if (!rubberBand)
                    rubberBand = new QRubberBand(QRubberBand::Rectangle, zoom->plot()->canvas());
            }
        }
    }
}

void QMainZoomSvc::proceedZoom(QMouseEvent *mEvent)
{
    if (zoom->regime() != ChartZoom::ctZoom) return;

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

void QMainZoomSvc::endZoom(QMouseEvent *mEvent)
{
    if (zoom->regime() != ChartZoom::ctZoom) return;

    if (mEvent->button() == Qt::LeftButton) {
        QwtPlot *plt = zoom->plot();

        if (rubberBand) rubberBand->hide();
        // определяем положение курсора, т.е. координаты xp и yp
        // конечной точки выделенной области (в пикселах относительно канвы QwtPlot)
        int xp = mEvent->pos().x();
        int yp = mEvent->pos().y();

        // если был одинарный щелчок мышью, то трактуем как установку курсора
        if (xp-startingPosX==0 && yp-startingPosY==0) {
            emit xAxisClicked(plt->canvasMap(QwtPlot::xBottom).invTransform(startingPosX),
                              mEvent->modifiers() & Qt::ControlModifier);
        }

        if (qAbs(xp - startingPosX) >= 8 && qAbs(yp - startingPosY) >= 8) {
            int leftmostX = qMin(xp, startingPosX); int rightmostX = qMax(xp, startingPosX);
            int leftmostY = qMin(yp, startingPosY); int rightmostY = qMax(yp, startingPosY);
            QwtPlot::Axis mX = QwtPlot::xBottom;
            const double xMin = plt->invTransform(mX,leftmostX);
            const double xMax = plt->invTransform(mX,rightmostX);

            QwtPlot::Axis mY = QwtPlot::yLeft;
            double yMin = plt->invTransform(mY,rightmostY);
            double yMax = plt->invTransform(mY,leftmostY);

            mY = QwtPlot::yRight;
            double ySMin = plt->invTransform(mY,rightmostY);
            double ySMax = plt->invTransform(mY,leftmostY);

            ChartZoom::zoomCoordinates coords;
            coords.coords.insert(QwtPlot::xBottom, {xMin, xMax});
            coords.coords.insert(QwtPlot::yLeft, {yMin, yMax});
            coords.coords.insert(QwtPlot::yRight, {ySMin, ySMax});
            zoom->addZoom(coords, true);
        }
        // закончили масштабирование
        zoom->setRegime(ChartZoom::ctNone);
    }
}

