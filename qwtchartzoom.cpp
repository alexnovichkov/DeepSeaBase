/**********************************************************/
/*                                                        */
/*             Реализация класса QwtChartZoom             */
/*                      Версия 1.5.2                      */
/*                                                        */
/* Разработал Мельников Сергей Андреевич,                 */
/* г. Каменск-Уральский Свердловской обл., 2012 г.,       */
/* при поддержке Ю. А. Роговского, г. Новосибирск.        */
/*                                                        */
/* Разрешается свободное использование и распространение. */
/* Упоминание автора обязательно.                         */
/*                                                        */
/**********************************************************/

#include "qwtchartzoom.h"

#include <qwt_scale_widget.h>

// Конструктор
QwtChartZoom::QwtChartZoom(QwtPlot *qp) :
    QObject(qp)
{
    // получаем главное окно
    mwin = generalParent(qp);
    // и назначаем обработчик событий (фильтр событий)
    mwin->installEventFilter(this);

    // сбрасываем флаг для того, чтобы перед первым изменением масштаба
    // текущие границы графика были зафиксированы в качестве исходных
    isbF = false;
    // сбрасываем признак режима
    convType = ctNone;

    // получаем компонент QwtPlot, над которым будут производиться все преобразования
    qwtp = qp;
    // устанавливаем ему свойство, разрешающее обрабатывать события от клавиатуры
    qp->setFocusPolicy(Qt::StrongFocus);

    qp->replot();   // перестраиваем график
    // Координатная сетка
    QwtPlotGrid *grid = NULL;
    // оси, к которым она прикреплена
    int xAx;    // горизонтальная
    int yAx;    // вертикальная
    // получаем список элементов графика
    QwtPlotItemList pil = qp->itemList();
    // перебираем список элементов
    for (int k=0; k < pil.count(); k++)
    {
        // получаем указатель на элемент
        QwtPlotItem *pi = pil.at(k);
        // если это координатная сетка, то
        if (pi->rtti() == QwtPlotItem::Rtti_PlotGrid)
        {
            // запоминаем указатель на нее
            grid = (QwtPlotGrid *)pi;
            // выясняем к какой оси она прикреплена
            xAx = grid->xAxis();    // из пары горизонтальных
            yAx = grid->yAxis();    // и пары вертикальных
            // прекращаем просмотр списка элементов
            break;
        }
    }
    // если координатная сетка была найдена, то
    if (grid != NULL)
    {
        // назначаем основную и дополнительную шкалу, отдавая предпочтение
        // той, к которой прикреплена сетка
            // горизонтальную
        allocAxis(xAx,QwtPlot::xBottom + QwtPlot::xTop - xAx,&masterX,&slaveX);
            // вертикальную
        allocAxis(yAx,QwtPlot::yLeft + QwtPlot::yRight - yAx,&masterY,&slaveY);
    }
    else    // иначе (координатная сетка отсутствует)
    {
        // назначаем основную и дополнительную шкалу, отдавая предпочтение
            // нижней из горизонтальных
        allocAxis(QwtPlot::xBottom,QwtPlot::xTop,&masterX,&slaveX);
            // и левой из вертикальных
        allocAxis(QwtPlot::yLeft,QwtPlot::yRight,&masterY,&slaveY);
    }
    // запоминаем количество делений на горизонтальной шкале
    mstHorDiv = qp->axisMaxMajor(masterX);
    slvHorDiv = qp->axisMaxMajor(slaveX);
/*    // получаем карту основной горизонтальной шкалы
    QwtScaleMap sm = qp->canvasMap(masterX);
    // и устанавливаем такие же границы для дополнительной
    qp->setAxisScale(slaveX,sm.s1(),sm.s2());*/
    // запоминаем количество делений на вертикальной шкале
    mstVerDiv = qp->axisMaxMajor(masterY);
    slvVerDiv = qp->axisMaxMajor(slaveY);
    // создаем контейнеры границ шкалы
    isb_x = new QScaleBounds(qp,masterX,slaveX);    // горизонтальной
    isb_y = new QScaleBounds(qp,masterY,slaveY);    // и вертикальной
    qp->replot();   // перестраиваем график

    // устанавливаем обработчик всех событий
    qwtp->installEventFilter(this);
    // для всех шкал графика
    for (int ax=0; ax < QwtPlot::axisCnt; ax++)
        // назначаем обработчик событий (фильтр событий)
        qwtp->axisWidget(ax)->installEventFilter(this);

    // создаем интерфейс масштабирования графика
    mnzmsvc = new QMainZoomSvc();
    // и прикрепляем его к менеджеру
    mnzmsvc->attach(this);

    // создаем интерфейс перемещенния графика
    drzmsvc = new QDragZoomSvc();
    // и прикрепляем его к менеджеру
    drzmsvc->attach(this);
}

// Деструктор
QwtChartZoom::~QwtChartZoom()
{
    // удаляем интерфейс перемещенния графика
    delete drzmsvc;
    // удаляем интерфейс масштабирования графика
    delete mnzmsvc;
    // удаляем контейнеры границ шкалы
    delete isb_x;    // горизонтальной
    delete isb_y;    // и вертикальной
}

// Определение главного родителя
QObject *QwtChartZoom::generalParent(QObject *p)
{
    // берем в качестве предыдущего родителя график
    // (возможен и другой объект в аргументе функции)
    QObject *gp = p;
    // определяем родителя на текущем уровне
    QObject *tp = gp->parent();
    // пока родитель на текущем уровне не NULL
    while (tp != NULL)
    {
        // понижаем уровень:
        // запоминаем в качестве предыдущего родителя текущий
        gp = tp;
        // определяем родителя на следующем уровне
        tp = gp->parent();
    }
    // возвращаем в качестве главного родителя предыдущий
    return gp;
}

// Назначение основной и дополнительной шкалы
void QwtChartZoom::allocAxis(int pre,int alt,
    QwtPlot::Axis *master,QwtPlot::Axis *slave)
{
    // получаем карту предпочтительной шкалы
    QwtScaleMap smp = qwtp->canvasMap(pre); // предпочтительной шкалы
    QwtScaleMap sma = qwtp->canvasMap(alt); // и альтернативной
    // если предпочтительная шкала доступна или
    // альтернативная шкала недоступна и при этом
    // границы предпочтительной шкалы не совпадают или
    // границы альтернативной шкалы совпадают, то
    if ((qwtp->axisEnabled(pre) ||
        !qwtp->axisEnabled(alt)) &&
        (smp.s1() != smp.s2() ||
         sma.s1() == sma.s2()))
    {
        // назначаем предпочтительную шкалу основной,
        *master = (QwtPlot::Axis)pre;
        // а альтернативную дополнительной
        *slave = (QwtPlot::Axis)alt;
    }
    else    // иначе
            // (предпочтительная шкала недоступна и
            // альтернативная шкала доступна или
            // границы предпочтительной шкалы совпадают и
            // границы альтернативной шкалы не совпадают)
    {
        // назначаем альтернативную шкалу основной,
        *master = (QwtPlot::Axis)alt;
        // а предпочтительную дополнительной
        *slave = (QwtPlot::Axis)pre;
    }
}

// Текущий режим масштабирования
QwtChartZoom::QConvType QwtChartZoom::regim() {
    return convType;
}

// Переключение режима масштабирования
void QwtChartZoom::setRegim(QwtChartZoom::QConvType ct) {
    convType = ct;
}

// указатель на опекаемый компонент QwtPlot
QwtPlot *QwtChartZoom::plot() {
    return qwtp;
}

// Основная горизонтальная шкала
QwtPlot::Axis QwtChartZoom::masterH() {
    return masterX;
}

// Дополнительная горизонтальная шкала
QwtPlot::Axis QwtChartZoom::slaveH() {
    return slaveX;
}

// Основная вертикальная шкала
QwtPlot::Axis QwtChartZoom::masterV() {
    return masterY;
}

// Дополнительная вертикальная шкала
QwtPlot::Axis QwtChartZoom::slaveV() {
    return slaveY;
}

// Установка цвета рамки, задающей новый размер графика
void QwtChartZoom::setRubberBandColor(QColor clr) {
    mnzmsvc->setRubberBandColor(clr);
}

// Включение/выключение легкого режима
void QwtChartZoom::setLightMode(bool lm)
{
    light = lm; // запоминаем значение
    // и устанавливаем его для интерфейса QDragZoomSvc
    drzmsvc->setLightMode(lm);
}

// Включение/выключение индикации перемещаемой области графика
// (имеет эффект, если включен легкий режим)
void QwtChartZoom::indicateDragBand(QDragIndiStyle indi) {
    drzmsvc->setIndicatorStyle(indi);
}

// Установка цвета виджета индикатора перемещения
void QwtChartZoom::setDragBandColor(QColor clr) {
    drzmsvc->setDragBandColor(clr);
}

// Фиксация текущих границ графика в качестве исходных
void QwtChartZoom::fixBoundaries() {
    // здесь только сбрасывается флаг и тем самым
    // указывается на необходимость фиксировать границы
    isbF = false;
    // фактическая фиксация границ произойдет в момент начала
    // какого-либо преобразования при вызове fixBounds()
}

// Включение/выключение синхронизации дополнительной
// горизонтальной шкалы графика
void QwtChartZoom::setHorSync(bool hs)
{
    // если состояние синхронизации изменилось, то
    // перестраиваем график
    if (setHSync(hs)) qwtp->replot();
}

// Включение/выключение синхронизации дополнительной
// вертикальной шкалы графика
void QwtChartZoom::setVerSync(bool vs)
{
    // если состояние синхронизации изменилось, то
    // перестраиваем график
    if (setVSync(vs)) qwtp->replot();
}

// Включение/выключение синхронизации дополнительной
// горизонтальной и вертикальной шкалы графика
void QwtChartZoom::setSync(bool s)
{
    // включаем/выключаем синхронизацию горизонтальной шкалы
    bool repF = setHSync(s);
    // и вертикальной
    repF |= setVSync(s);
    // если изменилось состояние синхронизации какой-либо шкалы,
    // то перестраиваем график
    if (repF) qwtp->replot();
}

// Непосредственное включение/выключение синхронизации
// дополнительной горизонтальной шкалы графика
bool QwtChartZoom::setHSync(bool hs)
{
    // выходим, если потребное состояние синхронизации
    // дополнительной горизонтальной шкалы уже установлено
    if (isb_x->sync == hs) return false;
    // запоминаем новое состояние синхронизации
    isb_x->sync = hs;
    // переустанавливаем границы дополнительной шкалы
    isb_x->dup();
    // возвращаем признак, что состояние
    return true;    // синхронизации изменилось
}

// Непосредственное включение/выключение синхронизации
// дополнительной вертикальной шкалы графика
bool QwtChartZoom::setVSync(bool vs)
{
    // выходим, если потребное состояние синхронизации
    // дополнительной вертикальной шкалы уже установлено
    if (isb_y->sync == vs) return false;
    // запоминаем новое состояние синхронизации
    isb_y->sync = vs;
    // переустанавливаем границы дополнительной шкалы
    isb_y->dup();
    // возвращаем признак, что состояние
    return true;    // синхронизации изменилось
}

// Обновление графика
void QwtChartZoom::updatePlot()
{
    qwtp->replot();
}

// Обработчик всех событий
bool QwtChartZoom::eventFilter(QObject *target,QEvent *event)
{
    // если событие произошло для главного окна,
    if (target == mwin)
        // если окно было отображено на экране, или изменились его размеры, то
        if (event->type() == QEvent::Show ||
            event->type() == QEvent::Resize)
            updatePlot();   // обновляем график
    // если событие произошло для графика, то
    if (target == qwtp)
        // если изменились размеры графика, то
        if (event->type() == QEvent::Resize)
            updatePlot();   // обновляем график
    // передаем управление стандартному обработчику событий
    return QObject::eventFilter(target,event);
}

// Фактическая фиксация текущих границ графика
// в качестве исходных (если флаг isbF сброшен)
void QwtChartZoom::fixBounds()
{
    // если этого еще не было сделано
    if (!isbF)
    {
        // фиксируем границы
        isb_x->fix();   // горизонтальные
        isb_y->fix();   // и вертикальные
        // устанавливаем флажок фиксации границ графика
        isbF = true;
    }
}

// Восстановление исходных границ графика
void QwtChartZoom::restBounds()
{
    // устанавливаем запомненные ранее границы
    isb_x->rest();  // горизонтальной шкалы
    isb_y->rest();  // и вертикальной
    // перестраиваем график
    qwtp->replot();
}

    /**************************************************/
    /*         Реализация класса QScaleBounds         */
    /*                  Версия 1.0.1                  */
    /**************************************************/

// Конструктор
QwtChartZoom::QScaleBounds::
    QScaleBounds(QwtPlot *plt,QwtPlot::Axis mst,QwtPlot::Axis slv) {
    // запоминаем
    plot = plt;     // опекаемый график
    master = mst;   // основную шкалу
    slave = slv;    // и дополнительную
    fixed = false;  // границы еще не фиксированы
    sync = false;   // синхронизации по умолчанию нет
}

// Фиксация исходных границ шкалы
void QwtChartZoom::QScaleBounds::fix()
{
    // получаем карту основной шкалы
    QwtScaleMap sm = plot->canvasMap(master);
    // и запоминаем текущие левую и правую границы шкалы
    min = sm.s1(); max = sm.s2();
    // получаем карту дополнительной горизонтальной шкалы
    sm = plot->canvasMap(slave);
    // строим преобразование основной шкалы в дополнительную в виде
    //     s = a * m + b, где:
    // если границы основной шкалы не совпадают, то
    if (min != max)
    {
        // a = (s2 - s1) / (m2 - m1)
        ak = (sm.s2() - sm.s1()) / (max - min);
        // b = (m2*s1 - m1*s2) / (m2 - m1)
        bk = (max * sm.s1() - min * sm.s2()) / (max - min);
    }
    else    // иначе (границы основной шкалы совпадают,
            // значит и дополнительной тоже)
    {
        // a = 0
        ak = 0;
        // b = s1
        bk = sm.s1();
    }
    fixed = true;   // границы фиксированы
}

// Установка заданных границ шкалы
void QwtChartZoom::QScaleBounds::set(double mn,double mx)
{
    // если границы еще не фиксированы, фиксируем их
    if (!fixed) fix();
    // устанавливаем нижнюю и верхнюю границы шкалы
    plot->setAxisScale(master,mn,mx);   // основной
                                        // и дополнительной
    if (sync) plot->setAxisScale(slave,mn,mx);
    else plot->setAxisScale(slave,ak*mn+bk,ak*mx+bk);
}

// Восстановление исходных границ шкалы
void QwtChartZoom::QScaleBounds::rest() {
    // если границы уже фиксированы, то восстанавливаем исходные
    if (fixed) set(min,max);
}

// Переустановка границ дополнительной шкалы
void QwtChartZoom::QScaleBounds::dup()
{
    // если границы еще не фиксированы, фиксируем их
    if (!fixed) fix();
    // получаем карту основной шкалы
    QwtScaleMap sm = plot->canvasMap(master);
    // и устанавливаем границы для дополнительной
    if (sync) plot->setAxisScale(slave,sm.s1(),sm.s2());
    else plot->setAxisScale(slave,ak*sm.s1()+bk,ak*sm.s2()+bk);
}

// Определение влияет ли указанная шкала на другие
bool QwtChartZoom::QScaleBounds::affected(QwtPlot::Axis ax)
{
    // если шкала основная, то вляиет
    if (ax == master) return true;
    // если шкала дополнительная и включена синхронизация, то вляиет
    if (ax == slave && sync) return true;
    // иначе (дополнительная и синхронизация выключена) не влияет
    return false;
}

// Установка количества делений на шкале с образца
void QwtChartZoom::QScaleBounds::setDiv(QwtScaleDiv *sdv)
{
    // устанавливаем количество делений для основной шкалы,
    plot->setAxisScaleDiv(master,*sdv);
    // а если задан режим синхронизации, то и для дополнительной
    if (sync) plot->setAxisScaleDiv(slave,*sdv);
}

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

// Конструктор
QMainZoomSvc::QMainZoomSvc() :
    QObject()
{
    // очищаем виджет, отвечающий за отображение выделенной области
    zwid = 0;
    // и назначаем ему цвет (по умолчанию - черный)
    zwClr = Qt::black;
}

// Прикрепление интерфейса к менеджеру масштабирования
void QMainZoomSvc::attach(QwtChartZoom *zm)
{
    // запоминаем указатель на менеджер масштабирования
    zoom = zm;
    // назначаем для графика обработчик событий (фильтр событий)
    zm->plot()->installEventFilter(this);
}

// Установка цвета рамки, задающей новый размер графика
void QMainZoomSvc::setRubberBandColor(QColor clr) {
    zwClr = clr;
}

// Обработчик всех событий
bool QMainZoomSvc::eventFilter(QObject *target,QEvent *event)
{
    // если событие произошло для графика, то
    if (target == zoom->plot())
        // если произошло одно из событий от мыши, то
        if (event->type() == QEvent::MouseButtonPress ||
            event->type() == QEvent::MouseMove ||
            event->type() == QEvent::MouseButtonRelease)
            procMouseEvent(event);  // вызываем соответствующий обработчик
    // передаем управление стандартному обработчику событий
    return QObject::eventFilter(target,event);
}

// Прорисовка виджета выделенной области
void QMainZoomSvc::showZoomWidget(QRect zr)
{
    // устанавливаем положение и размеры виджета, отображающего выделенную область
    zwid->setGeometry(zr);
    // запоминаем для удобства
    int dw = zr.width();    // ширину области
    int dh = zr.height();   // и высоту
    // формируем маску для виджета, отображающего выделенную область
    QRegion rw(0,0,dw,dh);      // непрозрачная область
    QRegion rs(1,1,dw-2,dh-2);  // прозрачная область
    // устанавливаем маску путем вычитания из непрозрачной области прозрачной
    zwid->setMask(rw.subtracted(rs));
    // делаем виджет, отображающий выделенную область, видимым
    zwid->setVisible(true);
    // перерисовываем виджет
    zwid->repaint();
}

// Обработчик обычных событий от мыши
void QMainZoomSvc::procMouseEvent(QEvent *event)
{
    // создаем указатель на событие от мыши
    QMouseEvent *mEvent = static_cast<QMouseEvent *>(event);
    // в зависимости от типа события вызываем соответствующий обработчик
    switch (event->type())
    {
        // нажата кнопка мыши
    case QEvent::MouseButtonPress: startZoom(mEvent); break;
        // перемещение мыши
    case QEvent::MouseMove: selectZoomRect(mEvent); break;
        // отпущена кнопка мыши
    case QEvent::MouseButtonRelease: procZoom(mEvent); break;
        // для прочих событий ничего не делаем
    default: ;
    }
}

// Обработчик нажатия на кнопку мыши
// (включение изменения масштаба)
void QMainZoomSvc::startZoom(QMouseEvent *mEvent)
{
    // фиксируем исходные границы графика (если этого еще не было сделано)
    zoom->fixBounds();
    // если в данный момент еще не включен ни один из режимов
    if (zoom->regim() == QwtChartZoom::ctNone)
    {
        // получаем указатели на
        QwtPlot *plt = zoom->plot();        // график
        QWidget *cv = plt->canvas();  // и канву
        // получаем геометрию канвы графика
        QRect cg = cv->geometry();
        // определяем текущее положение курсора (относительно канвы графика)
        scp_x = mEvent->pos().x() - cg.x();
        scp_y = mEvent->pos().y() - cg.y();
        // если курсор находится над канвой графика
        if (scp_x >= 0 && scp_x < cg.width() &&
            scp_y >= 0 && scp_y < cg.height())
            // если нажата левая кнопка мыши, то
            if (mEvent->button() == Qt::LeftButton)
            {
                // прописываем соответствующий признак режима
                zoom->setRegim(QwtChartZoom::ctZoom);
                // запоминаем текущий курсор
                tCursor = cv->cursor();
                // устанавливаем курсор Cross
                cv->setCursor(Qt::CrossCursor);
                // создаем виджет, который будет отображать выделенную область
                // (он будет прорисовываться на том же виджете, что и график)
                zwid = new QWidget(plt->parentWidget());
                // и назначаем ему цвет
                zwid->setStyleSheet(QString(
                    "background-color:rgb(%1,%2,%3);").arg(
                    zwClr.red()).arg(zwClr.green()).arg(zwClr.blue()));
            }
    }
}

// Обработчик перемещения мыши
// (выделение новых границ графика)
void QMainZoomSvc::selectZoomRect(QMouseEvent *mEvent) {
    // если включен режим изменения масштаба, то
    if (zoom->regim() == QwtChartZoom::ctZoom)
    {
        // получаем указатель на график
        QwtPlot *plt = zoom->plot();
        // получаем геометрию графика
        QRect pg = plt->geometry();
        // и геометрию канвы графика
        QRect cg = plt->canvas()->geometry();
        // scp_x - координата курсора в пикселах по горизонтальной оси
        //     в начальный момент времени (когда была нажата левая кнопка мыши)
        // mEvent->pos().x() - cg.x() - координата курсора в пикселах
        //     по горизонтальной оси в текущий момент времени
        // mEvent->pos().x() - cg.x() - scp_x - смещение курсора в пикселах
        //     по горизонтальной оси от начального положения и соответственно
        //     ширина dx выделенной области
        int dx = mEvent->pos().x() - cg.x() - scp_x;
        // pg.x() - положение графика по горизонтальной оси
        //     относительно виджета, его содержащего
        // pg.x() + cg.x() - положение канвы графика по горизонтальной оси
        //     относительно виджета, его содержащего
        // pg.x() + cg.x() + scp_x - положение gx0 начальной точки по горизонтальной оси
        //     относительно виджета, содержащего график, она нужна в качестве опоры
        //     для отображения выделенной области
        int gx0 = pg.x() + cg.x() + scp_x;
        // если ширина выделенной области отрицательна, то текущая точка находится левее начальной,
        //     и тогда именно ее мы используем в качестве опоры для отображения выделенной области
        if (dx < 0) {dx = -dx; gx0 -= dx;}
        // иначе если ширина равна нулю, то для того чтобы выделенная область все-таки отбражалась,
        //     принудительно сделаем ее равной единице
        else if (dx == 0) dx = 1;
        // аналогично определяем высоту dy выделенной области
        int dy = mEvent->pos().y() - cg.y() - scp_y;
        // и положение gy0 начальной точки по вертикальной оси
        int gy0 = pg.y() + cg.y() + scp_y;
        // если высота выделенной области отрицательна, то текущая точка находится выше начальной,
        //     и тогда именно ее мы используем в качестве опоры для отображения выделенной области
        if (dy < 0) {dy = -dy; gy0 -= dy;}
        // иначе если высота равна нулю, то для того чтобы выделенная область все-таки отбражалась,
        //     принудительно сделаем ее равной единице
        else if (dy == 0) dy = 1;
        // отображаем выделенную область
        showZoomWidget(QRect(gx0,gy0,dx,dy));
    }
}

// Обработчик отпускания кнопки мыши
// (выполнение изменения масштаба)
void QMainZoomSvc::procZoom(QMouseEvent *mEvent)
{
    // если включен режим изменения масштаба или режим перемещения графика
    if (zoom->regim() == QwtChartZoom::ctZoom)
        // если отпущена левая кнопка мыши, то
        if (mEvent->button() == Qt::LeftButton)
        {
            // получаем указатели на
            QwtPlot *plt = zoom->plot();        // график
            QWidget *cv = plt->canvas();  // и канву
            // восстанавливаем курсор
            cv->setCursor(tCursor);
            // удаляем виджет, отображающий выделенную область
            delete zwid;
            // получаем геометрию канвы графика
            QRect cg = cv->geometry();
            // определяем положение курсора, т.е. координаты xp и yp
            // конечной точки выделенной области (в пикселах относительно канвы QwtPlot)
            int xp = mEvent->pos().x() - cg.x();
            int yp = mEvent->pos().y() - cg.y();
            // если выделение производилось справа налево или снизу вверх,
            // то восстанавливаем исходные границы графика (отменяем увеличение)
            if (xp < scp_x || yp < scp_y) zoom->restBounds();
            // иначе если размер выделенной области достаточен, то изменяем масштаб
            else if (xp - scp_x >= 8 && yp - scp_y >= 8)
            {
                QwtPlot::Axis mX = zoom->masterH();
                // определяем левую границу горизонтальной шкалы по начальной точке
                double lf = plt->invTransform(mX,scp_x);
                // определяем правую границу горизонтальной шкалы по конечной точке
                double rg = plt->invTransform(mX,xp);
                // устанавливаем нижнюю и верхнюю границы вертикальной шкалы
                zoom->isb_x->set(lf,rg);
                // получаем основную вертикальную шкалу
                QwtPlot::Axis mY = zoom->masterV();
                // определяем нижнюю границу вертикальной шкалы по конечной точке
                double bt = plt->invTransform(mY,yp);
                // определяем верхнюю границу вертикальной шкалы по начальной точке
                double tp = plt->invTransform(mY,scp_y);
                // устанавливаем нижнюю и верхнюю границы вертикальной шкалы
                zoom->isb_y->set(bt,tp);
                // перестраиваем график (синхронно с остальными)
                plt->replot();
            }
            // очищаем признак режима
            zoom->setRegim(QwtChartZoom::ctNone);
        }
}

/**********************************************************/
/*                                                        */
/*             Реализация класса QDragZoomSvc             */
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

// Конструктор
QDragZoomSvc::QDragZoomSvc() :
    QObject()
{
    // по умолчанию легкий режим выключен
    light = false;
    // очищаем виджет, отвечающий за отображение индикатора перемещения
    zwid = 0;
    // и назначаем ему цвет (по умолчанию черный)
    dwClr = Qt::black;
    // по умолчанию включен простой режим индикации перемещение графика
    // (но только в том случае, если включен легкий режим)
    indiDrB = QwtChartZoom::disSimple;
}

// Прикрепление интерфейса к менеджеру масштабирования
void QDragZoomSvc::attach(QwtChartZoom *zm)
{
    // запоминаем указатель на менеджер масштабирования
    zoom = zm;
    // назначаем для графика обработчик событий (фильтр событий)
    zm->plot()->installEventFilter(this);
}

// Включение/выключение легкого режима
void QDragZoomSvc::setLightMode(bool lm) {
    light = lm;
}

// Установка цвета виджета индикатора перемещения
void QDragZoomSvc::setDragBandColor(QColor clr) {
    dwClr = clr;
}

// Включение/выключение индикации перемещаемой области
// (имеет эффект, если включен легкий режим)
void QDragZoomSvc::setIndicatorStyle(QwtChartZoom::QDragIndiStyle indi) {
    indiDrB = indi;
}

// Обработчик всех событий
bool QDragZoomSvc::eventFilter(QObject *target,QEvent *event)
{
    // если событие произошло для графика, то
    if (target == zoom->plot())
        // если произошло одно из событий от мыши, то
        if (event->type() == QEvent::MouseButtonPress ||
            event->type() == QEvent::MouseMove ||
            event->type() == QEvent::MouseButtonRelease)
            dragMouseEvent(event);  // вызываем соответствующий обработчик
    // передаем управление стандартному обработчику событий
    return QObject::eventFilter(target,event);
}

// Добавление в маску индикатора вертикальных линий сетки для меток горизонтальной шкалы
QRegion QDragZoomSvc::addHorTicks(QRegion rw,QwtScaleDiv::TickType tt)
{
    // получаем указатель на график
    QwtPlot *plt = zoom->plot();
    // получаем список основных меток горизонтальной шкалы
    QList<double> vl = plt->axisScaleDiv(zoom->masterH()).ticks(tt);

    // перебираем все метки горизонтальной шкалы
    for (int k=0; k < vl.count(); k++)
    {
        // вычисляем смещение метки относительно канвы
        int x = plt->transform(zoom->masterH(),vl.at(k));
        // формируем вертикальную линию сетки
        QRegion rs(x-1,1,1,rw.boundingRect().height()-2);
        // добавляем ее в маску
        rw = rw.united(rs);
    }
    // возвращаем измененную маску
    return rw;
}

// Добавление в маску индикатора горизонтальных линий сетки для меток вертикальной шкалы
QRegion QDragZoomSvc::addVerTicks(QRegion rw,QwtScaleDiv::TickType tt)
{
    // получаем указатель на график
    QwtPlot *plt = zoom->plot();
    // получаем список основных меток вертикальной шкалы
    QList<double> vl = plt->axisScaleDiv(zoom->masterV()).ticks(tt);

    // перебираем все метки вертикальной шкалы
    for (int k=0; k < vl.count(); k++)
    {
        // вычисляем смещение метки относительно канвы
        int y = plt->transform(zoom->masterV(),vl.at(k));
        // формируем горизонтальную линию сетки
        QRegion rs(1,y-1,rw.boundingRect().width()-2,1);
        // добавляем ее в маску
        rw = rw.united(rs);
    }
    // возвращаем измененную маску
    return rw;
}

// Прорисовка изображения индикатора перемещения
void QDragZoomSvc::showDragWidget(QPoint evpos)
{
    // получаем указатель на график
    QwtPlot *plt = zoom->plot();
    // получаем геометрию графика
    QRect pg = plt->geometry();             // графика
    QRect cg = plt->canvas()->geometry();   // и канвы графика
    // запоминаем для удобства
    int ww = cg.width() - 2;    // ширину канвы
    int wh = cg.height() - 2;   // и высоту
    // формируем положение и размер рамки канвы
    QRect wg(pg.x()+1+evpos.x()-scp_x,pg.y()+1+evpos.y()-scp_y,ww,wh);
    // устанавливаем положение и размеры виджета индикатора
    zwid->setGeometry(wg);
    // объявляем составляющие маски для виджета индикатора
    QRegion rw(0,0,ww,wh);      // непрозрачная область
    QRegion rs(1,1,ww-2,wh-2);  // прозрачная область
    // формируем маску путем вычитания из непрозрачной области прозрачной
    rw = rw.subtracted(rs);
    // если включен подробный режим индикации, то
    if (indiDrB == QwtChartZoom::disDetailed)
    {
        // добавляем в маску вертикальные линии сетки для делений горизонтальной шкалы
        rw = addHorTicks(rw,QwtScaleDiv::MajorTick);    // основных
        rw = addHorTicks(rw,QwtScaleDiv::MediumTick);   // средних
        rw = addHorTicks(rw,QwtScaleDiv::MinorTick);    // минимальных
        // добавляем в маску горизонтальные линии сетки для основных делений
        rw = addVerTicks(rw,QwtScaleDiv::MajorTick);    // вертикальной шкалы
    }
    // устанавливаем маску
    zwid->setMask(rw);
    // делаем виджет, индицирующий перемещение графика, видимым
    zwid->setVisible(true);
    // перерисовываем виджет
    zwid->repaint();
}

// Применение результатов перемещения графика
void QDragZoomSvc::applyDrag(QPoint evpos)
{
    // получаем указатель на график
    QwtPlot *plt = zoom->plot();
    // получаем геометрию канвы графика
    QRect cg = plt->canvas()->geometry();
    // scp_x - координата курсора в пикселах по горизонтальной оси
    //     в начальный момент времени (когда была нажата правая кнопка мыши)
    // evpos.x() - cg.x() - координата курсора
    //     в пикселах по горизонтальной оси в текущий момент времени
    // evpos.x() - cg.x() - scp_x - смещение курсора в пикселах
    //     по горизонтальной оси от начального положения
    // (evpos.x() - cg.x() - scp_x) * cs_kx -  это же смещение,
    //     но уже в единицах горизонтальной шкалы
    // dx - смещение границ по горизонтальной оси берется с обратным знаком
    //     (чтобы график относительно границ переместился вправо, сами границы следует сместить влево)
    double dx = -(evpos.x() - cg.x() - scp_x) * cs_kx;
    // устанавливаем новые левую и правую границы шкалы для горизонтальной оси
    //     новые границы = начальные границы + смещение
    zoom->isb_x->set(scb_xl + dx,scb_xr + dx);
    // аналогично определяем dy - смещение границ по вертикальной оси
    double dy = -(evpos.y() - cg.y() - scp_y) * cs_ky;
    // устанавливаем новые нижнюю и верхнюю границы вертикальной шкалы
    zoom->isb_y->set(scb_yb + dy,scb_yt + dy);
    // перестраиваем график (синхронно с остальными)
    plt->replot();
}

// Обработчик событий от мыши
void QDragZoomSvc::dragMouseEvent(QEvent *event)
{
    // создаем указатель на событие от мыши
    QMouseEvent *mEvent = static_cast<QMouseEvent *>(event);
    // в зависимости от типа события вызываем соответствующий обработчик
    switch (event->type())
    {
        // нажата кнопка мыши
    case QEvent::MouseButtonPress: startDrag(mEvent); break;
        // перемещение мыши
    case QEvent::MouseMove: procDrag(mEvent); break;
        // отпущена кнопка мыши
    case QEvent::MouseButtonRelease: endDrag(mEvent); break;
        // для прочих событий ничего не делаем
    default: ;
    }
}

// Обработчик нажатия на кнопку мыши
// (включение перемещения графика)
void QDragZoomSvc::startDrag(QMouseEvent *mEvent)
{
    // фиксируем исходные границы графика (если этого еще не было сделано)
    zoom->fixBounds();
    // если в данный момент еще не включен ни один из режимов
    if (zoom->regim() == QwtChartZoom::ctNone)
    {
        // получаем указатели на
        QwtPlot *plt = zoom->plot();        // график
        QWidget *cv = plt->canvas();  // и канву
        // получаем геометрию канвы графика
        QRect cg = cv->geometry();
        // определяем текущее положение курсора (относительно канвы графика)
        scp_x = mEvent->pos().x() - cg.x();
        scp_y = mEvent->pos().y() - cg.y();
        // если курсор находится над канвой графика
        if (scp_x >= 0 && scp_x < cg.width() &&
            scp_y >= 0 && scp_y < cg.height())
            // если нажата правая кнопка мыши, то
            if (mEvent->button() == Qt::RightButton)
            {
                // прописываем соответствующий признак режима
                zoom->setRegim(QwtChartZoom::ctDrag);
                // запоминаем текущий курсор
                tCursor = cv->cursor();
                // устанавливаем курсор OpenHand
                cv->setCursor(Qt::OpenHandCursor);
                // определяем текущий масштабирующий множитель по горизонтальной оси
                // (т.е. узнаем на сколько изменяется координата по шкале x
                // при перемещении курсора вправо на один пиксел)
                cs_kx = plt->invTransform(zoom->masterH(),scp_x + 1) -
                    plt->invTransform(zoom->masterH(),scp_x);
                // получаем основную вертикальную шкалу
                QwtPlot::Axis mY = zoom->masterV();
                // определяем текущий масштабирующий множитель по вертикальной оси
                // (аналогично)
                cs_ky = plt->invTransform(mY,scp_y + 1) -
                    plt->invTransform(mY,scp_y);
                // получаем карту основной горизонтальной шкалы
                QwtScaleMap sm = plt->canvasMap(zoom->masterH());
                // для того чтобы фиксировать начальные левую и правую границы
                scb_xl = sm.s1(); scb_xr = sm.s2();
                // аналогично получаем карту основной вертикальной шкалы
                sm = plt->canvasMap(mY);
                // для того чтобы фиксировать начальные нижнюю и верхнюю границы
                scb_yb = sm.s1(); scb_yt = sm.s2();
                // если легкий режим и включена индикация, то
                if (light and indiDrB != QwtChartZoom::disNone)
                {
                    // создаем виджет, индицирующий перемещение графика
                    zwid = new QWidget(plt->parentWidget());
                    // назначаем ему цвет
                    zwid->setStyleSheet(QString(
                        "background-color:rgb(%1,%2,%3);").arg(
                        dwClr.red()).arg(dwClr.green()).arg(dwClr.blue()));
                    // прорисовываем изображение индикатора перемещения
                    showDragWidget(mEvent->pos());
                }
            }
    }
}

// Обработчик перемещения мыши
// (выполнение перемещения или выбор нового положения графика)
void QDragZoomSvc::procDrag(QMouseEvent *mEvent)
{
    // если включен режим перемещения графика, то
    if (zoom->regim() == QwtChartZoom::ctDrag)
    {
        // устанавливаем курсор ClosedHand
        zoom->plot()->canvas()->setCursor(Qt::ClosedHandCursor);
        if (light)  // если легкий режим, то
        {
            // если включена индикация, то
            if (indiDrB != QwtChartZoom::disNone)
                // прорисовываем изображение индикатора перемещения
                showDragWidget(mEvent->pos());
        }
        // иначе применяем результаты перемещения графика
        else applyDrag(mEvent->pos());
    }
}

// Обработчик отпускания кнопки мыши
// (выключение перемещения графика)
void QDragZoomSvc::endDrag(QMouseEvent *mEvent)
{
    // если включен режим изменения масштаба или режим перемещения графика
    if (zoom->regim() == QwtChartZoom::ctDrag)
        // если отпущена правая кнопка мыши, то
        if (mEvent->button() == Qt::RightButton)
        {
            if (light)  // если легкий режим, то
            {
                // если включена индикация, то удаляем виджет индикатора
                if (indiDrB != QwtChartZoom::disNone) delete zwid;
                // применяем результаты перемещения графика
                applyDrag(mEvent->pos());
            }
            // восстанавливаем курсор
            zoom->plot()->canvas()->setCursor(tCursor);
            zoom->setRegim(QwtChartZoom::ctNone);  // и очищаем признак режима
        }
}
