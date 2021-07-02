#include "plotzoom.h"

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
#include "qwt_scale_map.h"
#include <QtDebug>
#include "plot.h"

constexpr int MinimumZoom = 8;

PlotZoom::PlotZoom()
{DD;
    rubberBand = nullptr;
}

void PlotZoom::attach(ChartZoom *zm)
{DD;
    zoom = zm;
    zoom->plot()->canvas()->installEventFilter(this);
}

bool PlotZoom::eventFilter(QObject *target,QEvent *event)
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

void PlotZoom::procMouseEvent(QEvent *event)
{
    auto mEvent = dynamic_cast<QMouseEvent *>(event);

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

void PlotZoom::procKeyboardEvent(QEvent *event)
{
    QKeyEvent *kEvent = dynamic_cast<QKeyEvent*>(event);
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

void PlotZoom::startZoom(QMouseEvent *mEvent)
{
    if (zoom->regime() == ChartZoom::ctNone) {
        QRect cg = zoom->plot()->canvas()->geometry();

        startingPosX = mEvent->pos().x();
        startingPosY = mEvent->pos().y();

        if (startingPosX >= 0 && startingPosX < cg.width() &&
            startingPosY >= 0 && startingPosY < cg.height()) {
            if (mEvent->button() == Qt::LeftButton) {
                zoom->setRegime(ChartZoom::ctZoom);
                if (!rubberBand) rubberBand = new QRubberBand(QRubberBand::Rectangle, zoom->plot()->canvas());
            }
        }
    }
}

void PlotZoom::proceedZoom(QMouseEvent *mEvent)
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

void PlotZoom::endZoom(QMouseEvent *mEvent)
{
    if (zoom->regime() != ChartZoom::ctZoom) return;

    if (mEvent->button() == Qt::LeftButton) {
        QwtPlot *plt = zoom->plot();

        if (rubberBand) rubberBand->hide();
        // определяем положение курсора, т.е. координаты xp и yp
        // конечной точки выделенной области (в пикселах относительно канвы QwtPlot)
        int xp = mEvent->pos().x();
        int yp = mEvent->pos().y();

        if (qAbs(xp - startingPosX) >= MinimumZoom && qAbs(yp - startingPosY) >= MinimumZoom) {
            int leftmostX = qMin(xp, startingPosX); int rightmostX = qMax(xp, startingPosX);
            int leftmostY = qMin(yp, startingPosY); int rightmostY = qMax(yp, startingPosY);
            QwtAxis::Position pos = QwtAxis::xBottom;
            const double xMin = plt->invTransform(pos,leftmostX);
            const double xMax = plt->invTransform(pos,rightmostX);

            pos = QwtAxis::yLeft;
            double yMin = plt->invTransform(pos,rightmostY);
            double yMax = plt->invTransform(pos,leftmostY);

            pos = QwtAxis::yRight;
            double ySMin = plt->invTransform(pos,rightmostY);
            double ySMax = plt->invTransform(pos,leftmostY);

            ChartZoom::zoomCoordinates coords;
            coords.coords.insert(QwtAxis::xBottom, {xMin, xMax});
            coords.coords.insert(QwtAxis::yLeft, {yMin, yMax});
            if (!zoom->plot()->spectrogram)
                coords.coords.insert(QwtAxis::yRight, {ySMin, ySMax});
            zoom->addZoom(coords, true);
        }
        // закончили масштабирование
        zoom->setRegime(ChartZoom::ctNone);
    }
}

