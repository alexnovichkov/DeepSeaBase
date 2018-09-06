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

#include "qaxiszoomsvc.h"
#include <QMenu>
#include <qwt_scale_widget.h>
#include "axisboundsdialog.h"
#include "logging.h"

// Конструктор
QAxisZoomSvc::QAxisZoomSvc() :
    QObject()
{
}

// Прикрепление интерфейса к менеджеру масштабирования
void QAxisZoomSvc::attach(ChartZoom *zm)
{DD;
    // запоминаем указатель на менеджер масштабирования
    zoom = zm;
    // для всех шкал графика, за которым закреплен менеджер,
    for (int ax=0; ax < QwtPlot::axisCnt; ax++)
        // назначаем обработчик событий (фильтр событий)
        zoom->plot()->axisWidget(ax)->installEventFilter(this);
}

// Обработчик всех событий
bool QAxisZoomSvc::eventFilter(QObject *target,QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
                    procKeyboardEvent(event);
                }

   // if (zoom->activated) {
        int ax = -1;
        for (int a=0; a < QwtPlot::axisCnt; a++)
            // если событие произошло для данной шкалы, то
            if (target == zoom->plot()->axisWidget(a))
            {
                ax = a;     // запоминаем номер шкалы
                break;
            }

        if (ax >= 0) {
            // если произошло одно из событий от мыши, то
            if (event->type() == QEvent::MouseButtonPress ||
                event->type() == QEvent::MouseMove ||
                event->type() == QEvent::MouseButtonRelease ||
                event->type() == QEvent::MouseButtonDblClick)
                axisMouseEvent(event,ax);   // вызываем соответствующий обработчик
//            else if (event->type() == QEvent::KeyPress) {
//                procKeyboardEvent(event);
//            }
        }
  //  }

    return QObject::eventFilter(target,event);
}

// Ограничение нового размера шкалы
double QAxisZoomSvc::limitScale(double sz,double bs)
{DD;
    // максимум
    double mx = 16*bs;
    // ограничение максимального размера
    if (sz > mx) sz = mx;
    return sz;
}

void QAxisZoomSvc::axisApplyMove(QPoint evpos, int ax)
{DD;
    // получаем указатель на график
    QwtPlot *plt = zoom->plot();
    // определяем (для удобства) геометрию
    QRect gc = plt->canvas()->geometry();       // канвы графика
    QRect gw = plt->axisWidget(ax)->geometry(); // и виджета шкалы
    // определяем текущее положение курсора относительно канвы
    // (за вычетом смещений графика)
    int x = evpos.x() + gw.x() - gc.x() - scb_pxl;
    int y = evpos.y() + gw.y() - gc.y() - scb_pyt;

    bool axisChanged = false;

    switch (zoom->regim()) {
        // режим изменения левой границы
        case ChartZoom::ctAxisHL: {
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
            axisChanged = true;   // изменилась граница
            break;
        }
            // режим изменения правой границы
        case ChartZoom::ctAxisHR:
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
            axisChanged = true;   // изменилась граница
            break;
        }
            // режим изменения нижней границы
        case ChartZoom::ctAxisVB:
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
            if (zoom->verticalScaleBoundsSlave->axis == ax)
                zoom->verticalScaleBoundsSlave->set(yb,currentTopBorder);
            else
                zoom->verticalScaleBounds->set(yb, currentTopBorder);
            axisChanged = true;   // изменилась граница
            break;
        }
            // режим изменения верхней границы
        case ChartZoom::ctAxisVT:
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

            if (zoom->verticalScaleBoundsSlave->axis == ax)
                zoom->verticalScaleBoundsSlave->set(currentBottomBorder,yt);
            else
                zoom->verticalScaleBounds->set(currentBottomBorder,yt);
            axisChanged = true;   // изменилась граница
            break;
        }
        default:
            break;
    }
    if (axisChanged) plt->replot();
}

// Обработчик событий от мыши для шкалы
void QAxisZoomSvc::axisMouseEvent(QEvent *event,int axis)
{DD;
    QMouseEvent *mEvent = static_cast<QMouseEvent *>(event);
    switch (event->type())
    {
        case QEvent::MouseButtonPress:
            if (mEvent->button()==Qt::RightButton) {
                emit contextMenuRequested(mEvent->globalPos(), axis);
            }
            else if (axis == QwtPlot::xBottom || axis == QwtPlot::xTop)
                startHorizontalAxisZoom(mEvent, axis);
            else
                startVerticalAxisZoom(mEvent, axis);
            break;

        case QEvent::MouseMove:
            proceedAxisZoom(mEvent,axis);
            break;

        case QEvent::MouseButtonRelease:
            endAxisZoom(mEvent,axis);
            break;

        case QEvent::MouseButtonDblClick:
            if (mEvent->button()==Qt::LeftButton) {
                AxisBoundsDialog dialog(zoom->plot()->canvasMap(axis).s1(), zoom->plot()->canvasMap(axis).s2(), axis);
                if (dialog.exec()) {
                    ChartZoom::zoomCoordinates coords;
                    if (axis == QwtPlot::xBottom || axis == QwtPlot::xTop)
                        coords.coords.insert(zoom->masterH(), {dialog.leftBorder(), dialog.rightBorder()});
                    else if (zoom->verticalScaleBounds->axis == axis)
                        coords.coords.insert(zoom->masterV(), {dialog.leftBorder(), dialog.rightBorder()});
                    else if (zoom->verticalScaleBoundsSlave->axis == axis)
                        coords.coords.insert(zoom->slaveV(), {dialog.leftBorder(), dialog.rightBorder()});
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

void QAxisZoomSvc::procKeyboardEvent(QEvent *event)
{
    QKeyEvent *kEvent = static_cast<QKeyEvent*>(event);
    switch (kEvent->key()) {
        case Qt::Key_Backspace:
            zoom->zoomBack();
            break;
        default: break;
    }
}

// Обработчик нажатия на кнопку мыши над шкалой
// (включение изменения масштаба шкалы)
void QAxisZoomSvc::startHorizontalAxisZoom(QMouseEvent *event, int axis)
{DD;
    // если в данный момент еще не включен ни один из режимов
    if (zoom->regim() == ChartZoom::ctNone) {
        // если нажата левая кнопка мыши, то
        // включаем один из режимов масштабирования
        if (event->button() == Qt::LeftButton) {
            // получаем указатели на
            QwtPlot *plot = zoom->plot();                // график
            QwtScaleWidget *scaleWidget = plot->axisWidget(axis);   // виджет шкалы

            // получаем карту основной горизонтальной шкалы
            QwtScaleMap sm = plot->canvasMap(axis);
            currentLeftBorder = sm.s1();
            currentRightBorder = sm.s2();
            currentWidth = sm.sDist();

            // определяем (для удобства) геометрию
            QRect canvasGeometry = plot->canvas()->geometry();   // канвы графика
            QRect scaleWidgetGeometry = scaleWidget->geometry();              // и виджета шкалы
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
            double xVal = sm.invTransform(event->pos().x());
            emit xAxisClicked(xVal, event->modifiers() & Qt::ControlModifier);

            // если левая граница меньше правой,
            if (currentWidth > 0) {
                // если ширина канвы больше минимума,
                if (currentPixelWidth > 36) {
                    // в зависимости от положения курсора
                    // (правее или левее середины шкалы)
                    // включаем соответствующий режим - изменение
                    if (cursorPosX >= floor(currentPixelWidth/2))
                        zoom->setRegime(ChartZoom::ctAxisHR);     // правой границы
                    else zoom->setRegime(ChartZoom::ctAxisHL);    // или левой
                }
            }

            // если один из режимов был включен
            if (zoom->regim() != ChartZoom::ctNone) {
                // запоминаем текущий курсор
                cursor = scaleWidget->cursor();
                scaleWidget->setCursor(Qt::PointingHandCursor);
            }
        }
    }
}

// Обработчик нажатия на кнопку мыши над шкалой
// (включение изменения масштаба шкалы)
void QAxisZoomSvc::startVerticalAxisZoom(QMouseEvent *event, int axis)
{DD;
    // если в данный момент еще не включен ни один из режимов
    if (zoom->regim() == ChartZoom::ctNone) {
        // если нажата левая кнопка мыши, то
        // включаем один из режимов масштабирования
        if (event->button() == Qt::LeftButton) {
            // получаем указатели на
            QwtPlot *plot = zoom->plot();                // график
            QwtScaleWidget *scaleWidget = plot->axisWidget(axis);   // виджет шкалы


            // получаем карту основной вертикальной шкалы
            QwtScaleMap sm = plot->canvasMap(axis);
            currentBottomBorder = sm.s1();
            currentTopBorder = sm.s2();
            currentHeight = sm.sDist();

            // определяем (для удобства) геометрию
            QRect canvasGeometry = plot->canvas()->geometry();   // канвы графика
            QRect scaleWidgetGeometry = scaleWidget->geometry();              // и виджета шкалы

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
                        zoom->setRegime(ChartZoom::ctAxisVB);     // нижней границы
                    else zoom->setRegime(ChartZoom::ctAxisVT);    // или верхней
                }
            }

            // если один из режимов был включен
            if (zoom->regim() != ChartZoom::ctNone) {
                // запоминаем текущий курсор
                cursor = scaleWidget->cursor();
                scaleWidget->setCursor(Qt::PointingHandCursor);
            }
        }
    }
}

// Обработчик перемещения мыши
// (выделение новых границ шкалы)
void QAxisZoomSvc::proceedAxisZoom(QMouseEvent *mEvent,int ax)
{DD;
    // читаем режим масштабирования
    ChartZoom::QConvType ct = zoom->regim();
    // выходим, если не включен ни один из режимов изменения шкалы
    if (ct != ChartZoom::ctAxisHL &&
        ct != ChartZoom::ctAxisHR &&
        ct != ChartZoom::ctAxisVB &&
        ct != ChartZoom::ctAxisVT) return;
    // применяем результаты перемещения границы шкалы
    axisApplyMove(mEvent->pos(),ax);
}

// Обработчик отпускания кнопки мыши
// (выполнение изменения масштаба шкалы)
void QAxisZoomSvc::endAxisZoom(QMouseEvent *mEvent,int ax)
{DD;
    // читаем режим масштабирования
    ChartZoom::QConvType ct = zoom->regim();
    // выходим, если не включен ни один из режимов изменения шкалы
    if (ct != ChartZoom::ctAxisHL &&
        ct != ChartZoom::ctAxisHR &&
        ct != ChartZoom::ctAxisVB &&
        ct != ChartZoom::ctAxisVT) return;
    // если отпущена левая кнопка мыши, то
    if (mEvent->button() == Qt::LeftButton)
    {
        // воостанавливаем курсор
        zoom->plot()->axisWidget(ax)->setCursor(cursor);
        // выключаем режим масштабирования
        zoom->setRegime(ChartZoom::ctNone);

        // запоминаем совершенное перемещение
        ChartZoom::zoomCoordinates coords;
        if (ax == zoom->masterH()) {
            coords.coords.insert(ax, {currentLeftBorder, currentRightBorder});
        }
        if (ax == zoom->masterV() || ax == zoom->slaveV()) {
            coords.coords.insert(ax, {currentBottomBorder, currentTopBorder});
        }
        if (!coords.coords.isEmpty()) zoom->addZoom(coords);
    }
}

