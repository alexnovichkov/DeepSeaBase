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

// Конструктор
QAxisZoomSvc::QAxisZoomSvc() :
    QObject()
{
}

// Прикрепление интерфейса к менеджеру масштабирования
void QAxisZoomSvc::attach(QwtChartZoom *zm)
{
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
   // if (zoom->activated) {
        int ax = -1;
        // просматриваем список шкал
        for (int a=0; a < QwtPlot::axisCnt; a++)
            // если событие произошло для данной шкалы, то
            if (target == zoom->plot()->axisWidget(a))
            {
                ax = a;     // запоминаем номер шкалы
                break;
            }

        // если шкала была найдена, то
        if (ax >= 0)
            // если произошло одно из событий от мыши, то
            if (event->type() == QEvent::MouseButtonPress ||
                event->type() == QEvent::MouseMove ||
                event->type() == QEvent::MouseButtonRelease)
                axisMouseEvent(event,ax);   // вызываем соответствующий обработчик
  //  }

    return QObject::eventFilter(target,event);
}

// Ограничение размера индикатора
int QAxisZoomSvc::limitSize(int sz,int bs)
{
    // минимум
    int mn = floor(16*bs/31);
    // ограничение минимального размера
    if (sz < mn) sz = mn;
    // максимум
    int mx = floor(31*bs/16);
    // ограничение максимальной размера
    if (sz > mx) sz = mx;
    return sz;
}

// Получение геометрии индикации масштабирования шкалы
QRect *QAxisZoomSvc::axisZoomRect(QPoint evpos,int ax)
{
    // получаем указатель на график
    QwtPlot *plt = zoom->plot();
    // определяем (для удобства) геометрию
    QRect gc = plt->canvas()->geometry();       // канвы графика
    QRect gw = plt->axisWidget(ax)->geometry(); // и виджета шкалы
    // определяем текущее положение курсора относительно канвы графика
    int x = evpos.x() + gw.x() - gc.x() - scb_pxl;
    int y = evpos.y() + gw.y() - gc.y() - scb_pyt;
    // запоминаем (для удобства)
    int wax = gw.width();   // ширину виджета шкалы
    int hax = gw.height();  // и высоту
    // читаем режим масштабирования
    QwtChartZoom::QConvType ct = zoom->regim();
    // объявляем положение левого верхнего угла,
    int wl,wt,ww,wh;    // ширину и высоту
    // если масштабируется горизонтальная шкала, то
    if (ax == QwtPlot::xBottom ||
        ax == QwtPlot::xTop)
    {
        // если изменяется правая граница, то
        if (ct == QwtChartZoom::ctAxisHR)
        {
            // ограничение на положение курсора слева
            int mn = floor(currentPixelWidth/16);
            // если курсор слишком близко к левой границе, то
            if (x < mn) x = mn;
            // ширина прямоугольника
            ww = floor(x * currentPixelWidth / cursorPosX);
            // применяем ограничения
            ww = limitSize(ww,currentPixelWidth);
            // левый отступ прямоугольника
            wl = sab_pxl;
        }
        else    // иначе (изменяется левая граница)
        {
            // ограничение на положение курсора справа
            int mx = floor(15*currentPixelWidth/16);
            // если курсор слишком близко к правой границе, то
            if (x > mx) x = mx;
            // ширина прямоугольника
            ww = floor((currentPixelWidth - x) * currentPixelWidth / (currentPixelWidth - cursorPosX));
            // применяем ограничения
            ww = limitSize(ww,currentPixelWidth);
            // левый отступ прямоугольника
            wl = sab_pxl + currentPixelWidth - ww;
        }
        // высота прямоугольника
        wh = 4;
        // верхний отступ прямоугольника
        wt = 10;    // для нижней шкалы
        // если не помещается на шкале, корректируем
        if (wt + wh > hax) wt = hax - wh;
        // для верхней шкалы симметрично
        if (ax == QwtPlot::xTop) wt = hax - wt - wh;
    }
    else    // иначе (масштабируется вертикальная шкала)
    {
        // если изменяется нижняя граница, то
        if (ct == QwtChartZoom::ctAxisVB)
        {
            // ограничение на положение курсора сверху
            int mn = floor(currentPixelHeight/16);
            // если курсор слишком близко к верхней границе, то
            if (y < mn) y = mn;
            // высота прямоугольника
            wh = floor(y * currentPixelHeight / cursorPosY);
            // применяем ограничения
            wh = limitSize(wh,currentPixelHeight);
            // верхний отступ прямоугольника
            wt = sab_pyt;
        }
        else    // иначе (изменяется верхняя граница)
        {
            // ограничение на положение курсора снизу
            int mx = floor(15*currentPixelHeight/16);
            // если курсор слишком близко к нижней границе, то
            if (y > mx) y = mx;
            // высота прямоугольника
            wh = floor((currentPixelHeight - y) * currentPixelHeight / (currentPixelHeight - cursorPosY));
            // применяем ограничения
            wh = limitSize(wh,currentPixelHeight);
            // верхний отступ прямоугольника = смещению курсора
            wt = sab_pyt + currentPixelHeight - wh;
        }
        // ширина прямоугольника
        ww = 4;
        // верхний отступ прямоугольника
        wl = 10;    // для правой шкалы
        // если не помещается на шкале, корректируем
        if (wl + ww > wax) wl = wax - ww;
        // для левой шкалы симметрично
        if (ax == QwtPlot::yLeft) wl = wax - wl - ww;
    }
    // создаем и возвращаем геометрию виджета
    // с вычисленными размерами
    return new QRect(wl,wt,ww,wh);
}

// Ограничение нового размера шкалы
double QAxisZoomSvc::limitScale(double sz,double bs)
{
    // максимум
    double mx = 16*bs;
    // ограничение максимального размера
    if (sz > mx) sz = mx;
    return sz;
}

// Применение результатов перемещения границы шкалы
void QAxisZoomSvc::axisApplyMove(QPoint evpos, int ax)
{
    // получаем указатель на график
    QwtPlot *plt = zoom->plot();
    // определяем (для удобства) геометрию
    QRect gc = plt->canvas()->geometry();       // канвы графика
    QRect gw = plt->axisWidget(ax)->geometry(); // и виджета шкалы
    // определяем текущее положение курсора относительно канвы
    // (за вычетом смещений графика)
    int x = evpos.x() + gw.x() - gc.x() - scb_pxl;
    int y = evpos.y() + gw.y() - gc.y() - scb_pyt;
    bool bndCh = false; // пока ничего не изменилось
    // читаем режим масштабирования
    QwtChartZoom::QConvType ct = zoom->regim();
    // в зависимости от включенного режима выполняем некоторые действия
    switch (ct) {
        // режим изменения левой границы
        case QwtChartZoom::ctAxisHL: {
            // ограничение на положение курсора справа
            if (x >= currentPixelWidth) x = currentPixelWidth-1;
            // вычисляем новую ширину шкалы
            double wx = currentWidth * (currentPixelWidth - cursorPosX) / (currentPixelWidth - x);
            // применяем ограничения
            wx = limitScale(wx,currentWidth);
            // вычисляем новую левую границу
            double xl = currentRightBorder - wx;
            if (xl<0.0) xl=0.0;

            // устанавливаем ее для горизонтальной шкалы
            zoom->horizontalScaleBounds->set(xl,currentRightBorder,ax);
            bndCh = true;   // изменилась граница
            break;
        }
            // режим изменения правой границы
        case QwtChartZoom::ctAxisHR:
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
            zoom->horizontalScaleBounds->set(currentLeftBorder,xr,ax);
            bndCh = true;   // изменилась граница
            break;
        }
            // режим изменения нижней границы
        case QwtChartZoom::ctAxisVB:
        {
            // ограничение на положение курсора сверху
            if (y <= 0) y = 1;
            // вычисляем новую высоту шкалы
            double hy = currentHeight * cursorPosY / y;
            // применяем ограничения
            hy = limitScale(hy,currentHeight);
            // вычисляем новую нижнюю границу
            double yb = currentTopBorder - hy;
            // устанавливаем ее для вертикальной шкалы
            zoom->verticalScaleBounds->set(yb,currentTopBorder, ax);
            bndCh = true;   // изменилась граница
            break;
        }
            // режим изменения верхней границы
        case QwtChartZoom::ctAxisVT:
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
            zoom->verticalScaleBounds->set(currentBottomBorder,yt,ax);
            bndCh = true;   // изменилась граница
            break;
        }
            // для прочих режимов ничего не делаем
        default: ;
    }
    // если какя-либо граница изменилась, то перестраиваем график
    if (bndCh) plt->replot();
}

// Обработчик событий от мыши для шкалы
void QAxisZoomSvc::axisMouseEvent(QEvent *event,int axis)
{
    QMouseEvent *mEvent = static_cast<QMouseEvent *>(event);
    switch (event->type())
    {
        case QEvent::MouseButtonPress:
            if (mEvent->button()==Qt::RightButton) {
                emit contextMenuRequested(mEvent->globalPos(), axis);
                //showContextMenu(mEvent,axis);
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


        default: break;
    }
}

// Обработчик нажатия на кнопку мыши над шкалой
// (включение изменения масштаба шкалы)
void QAxisZoomSvc::startHorizontalAxisZoom(QMouseEvent *event, int axis)
{

    // фиксируем исходные границы графика (если этого еще не было сделано)
    zoom->fixBounds();
    // если в данный момент еще не включен ни один из режимов
    if (zoom->regim() == QwtChartZoom::ctNone) {
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
                        zoom->setRegime(QwtChartZoom::ctAxisHR);     // правой границы
                    else zoom->setRegime(QwtChartZoom::ctAxisHL);    // или левой
                }
            }

            // если один из режимов был включен
            if (zoom->regim() != QwtChartZoom::ctNone) {
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
{
    // фиксируем исходные границы графика (если этого еще не было сделано)
    zoom->fixBounds();
    // если в данный момент еще не включен ни один из режимов
    if (zoom->regim() == QwtChartZoom::ctNone) {
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
                        zoom->setRegime(QwtChartZoom::ctAxisVB);     // нижней границы
                    else zoom->setRegime(QwtChartZoom::ctAxisVT);    // или верхней
                }
            }

            // если один из режимов был включен
            if (zoom->regim() != QwtChartZoom::ctNone) {
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
{
    // читаем режим масштабирования
    QwtChartZoom::QConvType ct = zoom->regim();
    // выходим, если не включен ни один из режимов изменения шкалы
    if (ct != QwtChartZoom::ctAxisHL &&
        ct != QwtChartZoom::ctAxisHR &&
        ct != QwtChartZoom::ctAxisVB &&
        ct != QwtChartZoom::ctAxisVT) return;
    // применяем результаты перемещения границы шкалы
    axisApplyMove(mEvent->pos(),ax);
}

// Обработчик отпускания кнопки мыши
// (выполнение изменения масштаба шкалы)
void QAxisZoomSvc::endAxisZoom(QMouseEvent *mEvent,int ax)
{
    // читаем режим масштабирования
    QwtChartZoom::QConvType ct = zoom->regim();
    // выходим, если не включен ни один из режимов изменения шкалы
    if (ct != QwtChartZoom::ctAxisHL &&
        ct != QwtChartZoom::ctAxisHR &&
        ct != QwtChartZoom::ctAxisVB &&
        ct != QwtChartZoom::ctAxisVT) return;
    // если отпущена левая кнопка мыши, то
    if (mEvent->button() == Qt::LeftButton)
    {
        // воостанавливаем курсор
        zoom->plot()->axisWidget(ax)->setCursor(cursor);
        // выключаем режим масштабирования
        zoom->setRegime(QwtChartZoom::ctNone);
    }
}

