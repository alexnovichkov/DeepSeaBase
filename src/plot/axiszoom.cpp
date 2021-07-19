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
#include "plot.h"

AxisZoom::AxisZoom(Plot *plot) :  QObject(plot), plot(plot)
{

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

ZoomStack::zoomCoordinates AxisZoom::axisApplyMove(QPoint evpos, QwtAxisId axis)
{DD;
    QRect canvasGeometry = plot->canvas()->geometry();       // канвы графика
    QRect axisGeometry = plot->axisWidget(axis)->geometry(); // и виджета шкалы
    // определяем текущее положение курсора относительно канвы
    // (за вычетом смещений графика)
    int x = evpos.x() + axisGeometry.x() - canvasGeometry.x() - currentLeftBorderInPixels;
    int y = evpos.y() + axisGeometry.y() - canvasGeometry.y() - currentTopBorderInPixels;

    ZoomStack::zoomCoordinates coords;

    switch (ct) {
        // режим изменения левой границы
        case ZoomStack::ctLeft: {
            // ограничение на положение курсора справа
            if (x >= currentPixelWidth) x = currentPixelWidth-1;
            // вычисляем новую ширину шкалы
            double wx = currentWidth * (currentPixelWidth - cursorPosX) / (currentPixelWidth - x);
            // применяем ограничения
            wx = limitScale(wx,currentWidth);
            // вычисляем новую левую границу
            double xl = currentRightBorder - wx;
            //if (xl<0.0) xl=0.0;
            currentLeftBorder = xl;
            // устанавливаем ее для горизонтальной шкалы
            coords.coords.insert(axis.pos, {xl, currentRightBorder});
            break;
        }
            // режим изменения правой границы
        case ZoomStack::ctRight:
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
            coords.coords.insert(axis.pos, {currentLeftBorder,xr});
            break;
        }
            // режим изменения нижней границы
        case ZoomStack::ctBottom:
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
            coords.coords.insert(axis.pos, {yb,currentTopBorder});
            break;
        }
            // режим изменения верхней границы
        case ZoomStack::ctTop:
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
            coords.coords.insert(axis.pos, {currentBottomBorder,yt});
            break;
        }
        default:
            break;
    }
    return coords;
}


void AxisZoom::startHorizontalAxisZoom(QMouseEvent *event, QwtAxisId axis)
{DD;
    if (ct == ZoomStack::ctNone) {
        QwtScaleWidget *scaleWidget = plot->axisWidget(axis);

        QwtScaleMap canvasMap = plot->canvasMap(axis);
        currentLeftBorder = canvasMap.s1();
        currentRightBorder = canvasMap.s2();
        currentWidth = canvasMap.sDist();

        // определяем геометрию
        QRect canvasGeometry = plot->canvas()->geometry();
        QRect scaleWidgetGeometry = scaleWidget->geometry();

        // текущее левое смещение графика (в пикселах относительно канвы)
        currentLeftBorderInPixels = plot->transform(axis, currentLeftBorder);
        // текущая ширина графика (в пикселах)
        currentPixelWidth = plot->transform(axis, currentRightBorder) - currentLeftBorderInPixels;
        // текущее левое смещение графика
        // (в пикселах относительно виджета шкалы)
        currentLeftShiftInPixels = currentLeftBorderInPixels + canvasGeometry.x() - scaleWidgetGeometry.x();
        // запоминаем текущее положение курсора относительно канвы
        // (за вычетом смещений графика)
        cursorPosX = event->pos().x() - currentLeftShiftInPixels;

        // если левая граница меньше правой,
        if (currentWidth > 0) {
            // если ширина канвы больше минимума,
            if (currentPixelWidth > 36) {
                // в зависимости от положения курсора
                // (правее или левее середины шкалы)
                // включаем соответствующий режим - изменение
                if (cursorPosX >= qFloor(currentPixelWidth/2))
                    ct = ZoomStack::ctRight;     // правой границы
                else
                    ct = ZoomStack::ctLeft;    // или левой
            }
        }

        // если один из режимов был включен
        if (ct != ZoomStack::ctNone) {
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
    if (ct == ZoomStack::ctNone) {
        QwtScaleWidget *scaleWidget = plot->axisWidget(axis);

        QwtScaleMap sm = plot->canvasMap(axis);
        currentBottomBorder = sm.s1();
        currentTopBorder = sm.s2();
        currentHeight = sm.sDist();

        // определяем (для удобства) геометрию
        QRect canvasGeometry = plot->canvas()->geometry();
        QRect scaleWidgetGeometry = scaleWidget->geometry();

        // текущее верхнее смещение графика (в пикселах относительно канвы)
        currentTopBorderInPixels = plot->transform(axis, currentTopBorder);
        // текущая высота графика (в пикселах)
        currentPixelHeight = plot->transform(axis,currentBottomBorder) - currentTopBorderInPixels;
        // текущее верхнее смещение графика
        // (в пикселах относительно виджета шкалы)
        currentTopShiftInPixels = currentTopBorderInPixels + canvasGeometry.y() - scaleWidgetGeometry.y();
        // запоминаем текущее положение курсора относительно канвы
        // (за вычетом смещений графика)
        cursorPosY = event->pos().y() - currentTopShiftInPixels;

        // если нижняя граница меньше верхней,
        if (currentHeight > 0) {
            // если высота канвы больше минимума,
            if (currentPixelHeight > 18) {
                // в зависимости от положения курсора
                // (ниже или выше середины шкалы)
                // включаем соответствующий режим - изменение
                if (cursorPosY >= floor(currentPixelHeight/2))
                    ct = ZoomStack::ctBottom;     // нижней границы
                else ct = ZoomStack::ctTop;    // или верхней
            }
        }

        // если один из режимов был включен
        if (ct != ZoomStack::ctNone) {
            // запоминаем текущий курсор
            cursor = scaleWidget->cursor();
            scaleWidget->setCursor(Qt::PointingHandCursor);
        }
    }
}

ZoomStack::zoomCoordinates AxisZoom::proceedAxisZoom(QMouseEvent *mEvent, QwtAxisId axis)
{DD;
    if (ct == ZoomStack::ctLeft || ct == ZoomStack::ctRight
        || ct == ZoomStack::ctBottom || ct == ZoomStack::ctTop)
        return axisApplyMove(mEvent->pos(), axis);


    QwtScaleMap sm = plot->canvasMap(axis);

    if (axis.isXAxis()) {
        if (mEvent->pos().x() - sm.p1() >= sm.pDist()/2)
            emit hover(axis, 2);
        else
            emit hover(axis, 1);
    }
    else {
        if (sm.p1() - mEvent->pos().y() <= sm.pDist()/2)
            emit hover(axis, 2);
        else
            emit hover(axis, 1);
    }
    return ZoomStack::zoomCoordinates();
}

ZoomStack::zoomCoordinates AxisZoom::endAxisZoom(QMouseEvent *mEvent, QwtAxisId axis)
{DD;
    ZoomStack::zoomCoordinates coords;
    if (ct == ZoomStack::ctLeft || ct == ZoomStack::ctRight
        ||ct == ZoomStack::ctBottom ||ct == ZoomStack::ctTop) {

        plot->axisWidget(axis)->setCursor(cursor);

        if (ct == ZoomStack::ctLeft || ct == ZoomStack::ctRight) {
            // emit axisClicked signal only if it is really just a click within 3 pixels
            if (qAbs(mEvent->pos().x() - currentLeftShiftInPixels - cursorPosX)<3) {
                double xVal = plot->canvasMap(axis).invTransform(mEvent->pos().x());
                emit xAxisClicked(xVal, mEvent->modifiers() & Qt::ControlModifier);
            }
            else if (axis.isXAxis()) {
                // запоминаем совершенное перемещение
                coords.coords.insert(axis.pos, {currentLeftBorder, currentRightBorder});
            }
        }
        if (ct == ZoomStack::ctBottom ||ct == ZoomStack::ctTop) {
            // emit axisClicked signal only if it is really just a click within 3 pixels
            if (qAbs(mEvent->pos().y() - currentTopShiftInPixels - cursorPosY)<3) {
                double yVal = plot->canvasMap(axis).invTransform(mEvent->pos().y());
                emit yAxisClicked(yVal, mEvent->modifiers() & Qt::ControlModifier);
            }
            else if (axis.isYAxis()) {
                // запоминаем совершенное перемещение
                coords.coords.insert(axis.pos, {currentBottomBorder, currentTopBorder});
            }
        }
        ct = ZoomStack::ctNone;
    }
    return ZoomStack::zoomCoordinates();
}

