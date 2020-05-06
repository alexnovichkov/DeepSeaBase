/**********************************************************/
/*                                                        */
/*             Реализация класса QAxisZoomSvc             */
/*                      Версия 1.2.2                      */
/*                                                        */
/* Разработал Мельников Сергей Андреевич,                 */
/* г. Каменск-Уральский Свердловской обл., 2012 г.,       */
/* при поддержке Ю. А. Роговского, г. Новосибирск.        */
/*                                                        */
/* Разрешается свободное использование и распространение. */
/* Упоминание автора обязательно.                         */
/*                                                        */
/**********************************************************/

#include "axiszoom.h"
#include <QMenu>
#include <qwt_scale_widget.h>
#include "axisboundsdialog.h"
#include "logging.h"
#include <QtMath>

AxisZoom::AxisZoom() :  QObject()
{
}

void AxisZoom::attach(ChartZoom *zm)
{DD;
    zoom = zm;
    for (int ax = 0; ax < QwtAxis::PosCount; ax++) {
        for (int j = 0; j < zoom->plot()->axesCount(ax); ++j)
            zoom->plot()->axisWidget(QwtAxisId(ax,j))->installEventFilter(this);
    }
}

bool AxisZoom::eventFilter(QObject *target,QEvent *event)
{
    if (event->type() == QEvent::KeyPress)
        procKeyboardEvent(event);

    int ax = -1;
    int jx = -1;
    for (int a = 0; a < QwtAxis::PosCount; a++) {
        for (int j = 0; j < zoom->plot()->axesCount(a); ++j) {
            if (target == zoom->plot()->axisWidget(QwtAxisId(a,j))) {
                ax = a;
                jx = j;
                break;
            }
        }
    }

    if (ax >= 0 && jx >= 0) {
        if (event->type() == QEvent::MouseButtonPress ||
            event->type() == QEvent::MouseMove ||
            event->type() == QEvent::MouseButtonRelease ||
            event->type() == QEvent::MouseButtonDblClick)
            axisMouseEvent(event, QwtAxisId(ax,jx));
    }

    return QObject::eventFilter(target,event);
}

void AxisZoom::axisMouseEvent(QEvent *event,QwtAxisId axis)
{DD;
    QMouseEvent *mEvent = static_cast<QMouseEvent *>(event);
    switch (event->type())
    {
        case QEvent::MouseButtonPress:
            if (mEvent->button()==Qt::RightButton) {
                emit contextMenuRequested(mEvent->globalPos(), axis);
            }
            else if (mEvent->button()==Qt::LeftButton) {
                if (axis.isXAxis())
                    startHorizontalAxisZoom(mEvent, axis);
                else
                    startVerticalAxisZoom(mEvent, axis);
            }
            break;

        case QEvent::MouseMove:
            proceedAxisZoom(mEvent, axis);
            break;

        case QEvent::MouseButtonRelease:
            endAxisZoom(mEvent, axis);
            break;

        case QEvent::MouseButtonDblClick:
            if (mEvent->button()==Qt::LeftButton) {
                AxisBoundsDialog dialog(zoom->plot()->canvasMap(axis).s1(), zoom->plot()->canvasMap(axis).s2(), axis);
                if (dialog.exec()) {
                    ChartZoom::zoomCoordinates coords;
                    if (axis == QwtAxis::xBottom || axis == QwtPlot::xTop)
                        coords.coords.insert(QwtAxis::xBottom, {dialog.leftBorder(), dialog.rightBorder()});
                    else if (zoom->verticalScaleBounds->axis == axis.pos ||
                             zoom->verticalScaleBoundsSlave->axis == axis.pos)
                        coords.coords.insert(axis.pos, {dialog.leftBorder(), dialog.rightBorder()});
                    if (!coords.coords.isEmpty())
                        zoom->addZoom(coords, true);

                    if (dialog.autoscale()) {
                        emit needsAutoscale(axis);
                    }
                    zoom->plot()->replot();
                }
            }
            break;


        default: break;
    }
}

void AxisZoom::procKeyboardEvent(QEvent *event)
{
    QKeyEvent *kEvent = static_cast<QKeyEvent*>(event);
    switch (kEvent->key()) {
        case Qt::Key_Backspace:
            zoom->zoomBack();
            break;
        case Qt::Key_Left:
            emit moveCursor(false);
            break;
        case Qt::Key_Right:
            emit moveCursor(true);
            break;
        default: break;
    }
}

// Ограничение нового размера шкалы
double AxisZoom::limitScale(double sz,double bs)
{DD;
    // максимум
    double mx = 16*bs;
    // ограничение максимального размера
    if (sz > mx) sz = mx;
    return sz;
}

void AxisZoom::axisApplyMove(QPoint evpos, QwtAxisId axis)
{DD;
    QwtPlot *plt = zoom->plot();
    // определяем (для удобства) геометрию
    QRect gc = plt->canvas()->geometry();       // канвы графика
    QRect gw = plt->axisWidget(axis)->geometry(); // и виджета шкалы
    // определяем текущее положение курсора относительно канвы
    // (за вычетом смещений графика)
    int x = evpos.x() + gw.x() - gc.x() - scb_pxl;
    int y = evpos.y() + gw.y() - gc.y() - scb_pyt;

    switch (zoom->regime()) {
        // режим изменения левой границы
        case ChartZoom::ctLeft: {
            // ограничение на положение курсора справа
            if (x >= currentPixelWidth) x = currentPixelWidth-1;
            // вычисляем новую ширину шкалы
            double wx = currentWidth * (currentPixelWidth - cursorPosX) / (currentPixelWidth - x);
            // применяем ограничения
            wx = limitScale(wx,currentWidth);
            // вычисляем новую левую границу
            double xl = currentRightBorder - wx;
            if (xl<0.0) xl=0.0;
            currentLeftBorder = xl;
            // устанавливаем ее для горизонтальной шкалы
            zoom->horizontalScaleBounds->set(xl,currentRightBorder);
            break;
        }
            // режим изменения правой границы
        case ChartZoom::ctRight:
        {
            // ограничение на положение курсора слева
            if (x <= 0) x = 1;
            // вычисляем новую ширину шкалы
            double wx = currentWidth * cursorPosX / x;
            // применяем ограничения
            wx = limitScale(wx,currentWidth);
            // вычисляем новую правую границу
            double xr = currentLeftBorder + wx;
            // устанавливаем ее для горизонтальной шкалы
            currentRightBorder = xr;
            zoom->horizontalScaleBounds->set(currentLeftBorder,xr);
            break;
        }
            // режим изменения нижней границы
        case ChartZoom::ctBottom:
        {
            // ограничение на положение курсора сверху
            if (y <= 0) y = 1;
            // вычисляем новую высоту шкалы
            double hy = currentHeight * cursorPosY / y;
            // применяем ограничения
            hy = limitScale(hy,currentHeight);
            // вычисляем новую нижнюю границу
            double yb = currentTopBorder - hy;

            currentBottomBorder = yb;
            if (zoom->verticalScaleBoundsSlave->axis == axis.pos)
                zoom->verticalScaleBoundsSlave->set(yb,currentTopBorder);
            else
                zoom->verticalScaleBounds->set(yb, currentTopBorder);
            break;
        }
            // режим изменения верхней границы
        case ChartZoom::ctTop:
        {
            // ограничение на положение курсора снизу
            if (y >= currentPixelHeight) y = currentPixelHeight-1;
            // вычисляем новую высоту шкалы
            double hy = currentHeight * (currentPixelHeight - cursorPosY) / (currentPixelHeight - y);
            // применяем ограничения
            hy = limitScale(hy,currentHeight);
            // вычисляем новую верхнюю границу
            double yt = currentBottomBorder + hy;
            // устанавливаем ее для вертикальной шкалы
            currentTopBorder = yt;

            if (zoom->verticalScaleBoundsSlave->axis == axis.pos)
                zoom->verticalScaleBoundsSlave->set(currentBottomBorder,yt);
            else
                zoom->verticalScaleBounds->set(currentBottomBorder,yt);
            break;
        }
        default:
            break;
    }
}


void AxisZoom::startHorizontalAxisZoom(QMouseEvent *event, QwtAxisId axis)
{DD;
    if (zoom->regime() == ChartZoom::ctNone) {
        QwtPlot *plot = zoom->plot();
        QwtScaleWidget *scaleWidget = plot->axisWidget(axis);

        QwtScaleMap sm = plot->canvasMap(axis);
        currentLeftBorder = sm.s1();
        currentRightBorder = sm.s2();
        currentWidth = sm.sDist();

        // определяем геометрию
        QRect canvasGeometry = plot->canvas()->geometry();
        QRect scaleWidgetGeometry = scaleWidget->geometry();

        // текущее левое смещение графика (в пикселах относительно канвы)
        scb_pxl = plot->transform(axis, currentLeftBorder);
        // текущая ширина графика (в пикселах)
        currentPixelWidth = plot->transform(axis, currentRightBorder) - scb_pxl;
        // текущее левое смещение графика
        // (в пикселах относительно виджета шкалы)
        sab_pxl = scb_pxl + canvasGeometry.x() - scaleWidgetGeometry.x();
        // запоминаем текущее положение курсора относительно канвы
        // (за вычетом смещений графика)
        cursorPosX = event->pos().x() - sab_pxl;

        // если левая граница меньше правой,
        if (currentWidth > 0) {
            // если ширина канвы больше минимума,
            if (currentPixelWidth > 36) {
                // в зависимости от положения курсора
                // (правее или левее середины шкалы)
                // включаем соответствующий режим - изменение
                if (cursorPosX >= qFloor(currentPixelWidth/2))
                    zoom->setRegime(ChartZoom::ctRight);     // правой границы
                else zoom->setRegime(ChartZoom::ctLeft);    // или левой
            }
        }

        // если один из режимов был включен
        if (zoom->regime() != ChartZoom::ctNone) {
            // запоминаем текущий курсор
            cursor = scaleWidget->cursor();
            scaleWidget->setCursor(Qt::PointingHandCursor);
        }
    }
}

// Обработчик нажатия на кнопку мыши над шкалой
// (включение изменения масштаба шкалы)
void AxisZoom::startVerticalAxisZoom(QMouseEvent *event, QwtAxisId axis)
{DD;
    if (zoom->regime() == ChartZoom::ctNone) {
        QwtPlot *plot = zoom->plot();
        QwtScaleWidget *scaleWidget = plot->axisWidget(axis);

        QwtScaleMap sm = plot->canvasMap(axis);
        currentBottomBorder = sm.s1();
        currentTopBorder = sm.s2();
        currentHeight = sm.sDist();

        // определяем (для удобства) геометрию
        QRect canvasGeometry = plot->canvas()->geometry();
        QRect scaleWidgetGeometry = scaleWidget->geometry();

        // текущее верхнее смещение графика (в пикселах относительно канвы)
        scb_pyt = plot->transform(axis, currentTopBorder);
        // текущая высота графика (в пикселах)
        currentPixelHeight = plot->transform(axis,currentBottomBorder) - scb_pyt;
        // текущее верхнее смещение графика
        // (в пикселах относительно виджета шкалы)
        sab_pyt = scb_pyt + canvasGeometry.y() - scaleWidgetGeometry.y();
        // запоминаем текущее положение курсора относительно канвы
        // (за вычетом смещений графика)
        cursorPosY = event->pos().y() - sab_pyt;

        // если нижняя граница меньше верхней,
        if (currentHeight > 0) {
            // если высота канвы больше минимума,
            if (currentPixelHeight > 18) {
                // в зависимости от положения курсора
                // (ниже или выше середины шкалы)
                // включаем соответствующий режим - изменение
                if (cursorPosY >= floor(currentPixelHeight/2))
                    zoom->setRegime(ChartZoom::ctBottom);     // нижней границы
                else zoom->setRegime(ChartZoom::ctTop);    // или верхней
            }
        }

        // если один из режимов был включен
        if (zoom->regime() != ChartZoom::ctNone) {
            // запоминаем текущий курсор
            cursor = scaleWidget->cursor();
            scaleWidget->setCursor(Qt::PointingHandCursor);
        }
    }
}

void AxisZoom::proceedAxisZoom(QMouseEvent *mEvent, QwtAxisId axis)
{DD;
    ChartZoom::ConvType ct = zoom->regime();
    if (ct == ChartZoom::ctLeft || ct == ChartZoom::ctRight
        || ct == ChartZoom::ctBottom || ct == ChartZoom::ctTop)
    axisApplyMove(mEvent->pos(), axis);
}

void AxisZoom::endAxisZoom(QMouseEvent *mEvent, QwtAxisId axis)
{DD;
    ChartZoom::ConvType ct = zoom->regime();

    if (ct == ChartZoom::ctLeft || ct == ChartZoom::ctRight
        ||ct == ChartZoom::ctBottom ||ct == ChartZoom::ctTop) {

        zoom->plot()->axisWidget(axis)->setCursor(cursor);
        zoom->setRegime(ChartZoom::ctNone);

        // emit axisClicked signal only if it is really just a click within 3 pixels
        if (qAbs(mEvent->pos().x() - sab_pxl - cursorPosX)<3) {
            double xVal = zoom->plot()->canvasMap(axis).invTransform(mEvent->pos().x());
            emit xAxisClicked(xVal, mEvent->modifiers() & Qt::ControlModifier);
        }

        // запоминаем совершенное перемещение
        ChartZoom::zoomCoordinates coords;
        if (axis.isXAxis()) {
            coords.coords.insert(axis.pos, {currentLeftBorder, currentRightBorder});
        }
        if (axis.isYAxis()) {
            coords.coords.insert(axis.pos, {currentBottomBorder, currentTopBorder});
        }
        if (!coords.coords.isEmpty()) zoom->addZoom(coords);
    }
}

