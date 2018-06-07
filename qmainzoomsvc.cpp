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
#include "qwtchartzoom.h"
#include <QRubberBand>

// Конструктор
QMainZoomSvc::QMainZoomSvc() :
    QObject()
{DD;
    // очищаем виджет, отвечающий за отображение выделенной области
    zwid = 0;
}

// Прикрепление интерфейса к менеджеру масштабирования
void QMainZoomSvc::attach(QwtChartZoom *zm)
{DD;
    // запоминаем указатель на менеджер масштабирования
    zoom = zm;
    // назначаем для графика обработчик событий (фильтр событий)
    zm->plot()->canvas()->installEventFilter(this);
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
            procZoom(mEvent);
            break;
        case QEvent::MouseButtonDblClick:
            if (mEvent->button() == Qt::LeftButton)
                zoom->resetBounds(Qt::Horizontal | Qt::Vertical);
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
            if (kEvent->modifiers() == Qt::NoModifier)
                zoom->resetBounds(Qt::Horizontal | Qt::Vertical);
            else if (kEvent->modifiers() & Qt::ControlModifier)
                zoom->resetBounds(Qt::Horizontal);
            else if (kEvent->modifiers() & Qt::ShiftModifier)
                zoom->resetBounds(Qt::Vertical);
            break;
        }
        case Qt::Key_Escape: {
            if (zoom->regim() == QwtChartZoom::ctZoom) {
                // восстанавливаем курсор
                zoom->plot()->canvas()->setCursor(tCursor);
                // удаляем виджет, отображающий выделенную область
                if (zwid) zwid->hide();
                zoom->setRegime(QwtChartZoom::ctNone);
            }
        }
        default: break;
    }
}

// Обработчик нажатия на кнопку мыши
// (включение изменения масштаба)
void QMainZoomSvc::startZoom(QMouseEvent *mEvent)
{DD;
    // фиксируем исходные границы графика (если этого еще не было сделано)
    zoom->fixBounds();
    // если в данный момент еще не включен ни один из режимов
    if (zoom->regim() == QwtChartZoom::ctNone)
    {
        // получаем указатели на
        QwtPlot *plot = zoom->plot();        // график
        QWidget *plotCanvas = plot->canvas();  // и канву
        // получаем геометрию канвы графика
        QRect cg = plotCanvas->geometry();
        // определяем текущее положение курсора (относительно канвы графика)
        scp_x = mEvent->pos().x();
        scp_y = mEvent->pos().y();
        // если курсор находится над канвой графика
        if (scp_x >= 0 && scp_x < cg.width() &&
            scp_y >= 0 && scp_y < cg.height()) {
            // если нажата левая кнопка мыши, то
            if (mEvent->button() == Qt::LeftButton)
            {
                // прописываем соответствующий признак режима
                zoom->setRegime(QwtChartZoom::ctZoom);
                // создаем виджет, который будет отображать выделенную область
                // (он будет прорисовываться на том же виджете, что и график)
                if (!zwid) {
                    zwid = new QRubberBand(QRubberBand::Rectangle, zoom->plot()->canvas());
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
    if (zoom->regim() == QwtChartZoom::ctZoom)
    {
        int dx = mEvent->pos().x() - scp_x;
        if (dx == 0) dx = 1;

        int dy = mEvent->pos().y() - scp_y;
        if (dy == 0) dy = 1;
        // отображаем выделенную область
        if (zwid) {
            zwid->setGeometry(QRect(scp_x, scp_y, dx,dy).normalized());
            zwid->show();
        }
    }
}

// Обработчик отпускания кнопки мыши
// (выполнение изменения масштаба)
void QMainZoomSvc::procZoom(QMouseEvent *mEvent)
{DD;
    // если включен режим изменения масштаба или режим перемещения графика
    if (zoom->regim() == QwtChartZoom::ctZoom) {
        // если отпущена левая кнопка мыши, то
        if (mEvent->button() == Qt::LeftButton)
        {
            QwtPlot *plt = zoom->plot();
            // удаляем виджет, отображающий выделенную область
            if (zwid) zwid->hide();
            // определяем положение курсора, т.е. координаты xp и yp
            // конечной точки выделенной области (в пикселах относительно канвы QwtPlot)
            int xp = mEvent->pos().x() ;
            int yp = mEvent->pos().y() ;

            // если был одинарный щелчок мышью, то трактуем как установку курсора
            if (xp-scp_x==0 && yp-scp_y==0) {
                emit xAxisClicked(zoom->plot()->canvasMap(QwtPlot::xBottom).invTransform(scp_x), mEvent->modifiers() & Qt::ControlModifier);
            }

            if (qAbs(xp - scp_x) >= 8 && qAbs(yp - scp_y) >= 8)
            {
                int leftmostX = qMin(xp, scp_x); int rightmostX = qMax(xp, scp_x);
                int leftmostY = qMin(yp, scp_y); int rightmostY = qMax(yp, scp_y);
                QwtPlot::Axis mX = zoom->masterH();
                // определяем левую границу горизонтальной шкалы по начальной точке
                double lf = plt->invTransform(mX,leftmostX);
                // определяем правую границу горизонтальной шкалы по конечной точке
                double rg = plt->invTransform(mX,rightmostX);
                // устанавливаем нижнюю и верхнюю границы горизонтальной шкалы
                zoom->horizontalScaleBounds->set(lf,rg,-1);
                // получаем основную вертикальную шкалу
                QwtPlot::Axis mY = zoom->masterV();
                // определяем нижнюю границу вертикальной шкалы по конечной точке
                double bt = plt->invTransform(mY,rightmostY);
                // определяем верхнюю границу вертикальной шкалы по начальной точке
                double tp = plt->invTransform(mY,leftmostY);
                // устанавливаем нижнюю и верхнюю границы вертикальной шкалы
                zoom->verticalScaleBounds->set(bt,tp,-1);
                // перестраиваем график (синхронно с остальными)
                plt->replot();
            }
            // очищаем признак режима
            zoom->setRegime(QwtChartZoom::ctNone);
        }
    }
}
