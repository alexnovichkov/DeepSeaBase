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
#include "qwheelzoomsvc.h"
#include "qaxiszoomsvc.h"
#include <qwt_scale_widget.h>

#include <QKeyEvent>

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
    qwtPlot = qp;
    // устанавливаем ему свойство, разрешающее обрабатывать события от клавиатуры
    qp->setFocusPolicy(Qt::StrongFocus);

    // назначаем основную и дополнительную шкалу
    masterX = QwtPlot::xBottom;
    slaveX = QwtPlot::xTop;

    masterY = QwtPlot::yLeft;
    slaveY = QwtPlot::yRight;

    // запоминаем количество делений на горизонтальной шкале
    mstHorDiv = qp->axisMaxMajor(masterX);
    slvHorDiv = qp->axisMaxMajor(slaveX);
    // запоминаем количество делений на вертикальной шкале
    mstVerDiv = qp->axisMaxMajor(masterY);
    slvVerDiv = qp->axisMaxMajor(slaveY);
    // создаем контейнеры границ шкалы
    horizontalScaleBounds = new QScaleBounds(qp,masterX,slaveX);    // горизонтальной
    verticalScaleBounds = new QScaleBounds(qp,masterY,slaveY);    // и вертикальной

    // устанавливаем обработчик всех событий
    qwtPlot->installEventFilter(this);
    // для всех шкал графика
    for (int ax=0; ax < QwtPlot::axisCnt; ax++)
        // назначаем обработчик событий (фильтр событий)
        qwtPlot->axisWidget(ax)->installEventFilter(this);

    // создаем интерфейс масштабирования графика
    mainZoom = new QMainZoomSvc();
    connect(mainZoom,SIGNAL(xAxisClicked(double,bool)),SIGNAL(updateTrackingCursor(double,bool)));
    mainZoom->attach(this);

    // создаем интерфейс перемещенния графика
    dragZoom = new QDragZoomSvc();
    dragZoom->attach(this);

    wheelZoom = new QWheelZoomSvc();
    wheelZoom->attach(this);

    axisZoom = new QAxisZoomSvc();
    connect(axisZoom,SIGNAL(xAxisClicked(double,bool)),SIGNAL(updateTrackingCursor(double,bool)));
    connect(axisZoom,SIGNAL(contextMenuRequested(QPoint,int)),SIGNAL(contextMenuRequested(QPoint,int)));
    axisZoom->attach(this);
}

// Деструктор
QwtChartZoom::~QwtChartZoom()
{
    // удаляем интерфейс перемещенния графика
    delete dragZoom;
    // удаляем интерфейс масштабирования графика
    delete mainZoom;
    delete axisZoom;
    delete wheelZoom;
    // удаляем контейнеры границ шкалы
    delete horizontalScaleBounds;    // горизонтальной
    delete verticalScaleBounds;    // и вертикальной
}

// Определение главного родителя
QObject *QwtChartZoom::generalParent(QObject *p)
{
    // берем в качестве предыдущего родителя график
    // (возможен и другой объект в аргументе функции)
    QObject *generalParent_ = p;
    // определяем родителя на текущем уровне
    QObject *tp = generalParent_->parent();
    // пока родитель на текущем уровне не NULL
    while (tp != NULL)
    {
        // понижаем уровень:
        // запоминаем в качестве предыдущего родителя текущий
        generalParent_ = tp;
        // определяем родителя на следующем уровне
        tp = generalParent_->parent();
    }
    // возвращаем в качестве главного родителя предыдущий
    return generalParent_;
}

// Текущий режим масштабирования
QwtChartZoom::QConvType QwtChartZoom::regim() {
    return convType;
}

// Переключение режима масштабирования
void QwtChartZoom::setRegime(QwtChartZoom::QConvType ct) {
    convType = ct;
}

// указатель на опекаемый компонент QwtPlot
QwtPlot *QwtChartZoom::plot() {
    return qwtPlot;
}

// Основная горизонтальная шкала
QwtPlot::Axis QwtChartZoom::masterH() const {
    return masterX;
}

// Дополнительная горизонтальная шкала
QwtPlot::Axis QwtChartZoom::slaveH() const {
    return slaveX;
}

// Основная вертикальная шкала
QwtPlot::Axis QwtChartZoom::masterV() const {
    return masterY;
}

// Дополнительная вертикальная шкала
QwtPlot::Axis QwtChartZoom::slaveV() const {
    return slaveY;
}

// Установка цвета рамки, задающей новый размер графика
void QwtChartZoom::setRubberBandColor(QColor clr) {
    mainZoom->setRubberBandColor(clr);
}

// Фиксация текущих границ графика в качестве исходных
void QwtChartZoom::fixBoundaries() {
    // здесь только сбрасывается флаг и тем самым
    // указывается на необходимость фиксировать границы
    isbF = false;
    // фактическая фиксация границ произойдет в момент начала
    // какого-либо преобразования при вызове fixBounds()
}

// Обновление графика
void QwtChartZoom::updatePlot()
{
    qwtPlot->replot();
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
    if (target == qwtPlot)
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
        horizontalScaleBounds->fix();   // горизонтальные
        verticalScaleBounds->fix();   // и вертикальные
        // устанавливаем флажок фиксации границ графика
        isbF = true;
    }
}

// Восстановление исходных границ графика
void QwtChartZoom::resetBounds(Qt::Orientations orientations)
{
    // устанавливаем запомненные ранее границы
    if (orientations & Qt::Horizontal) horizontalScaleBounds->reset();  // горизонтальной шкалы
    if (orientations & Qt::Vertical) verticalScaleBounds->reset();  // и вертикальной
    // перестраиваем график
    qwtPlot->replot();
}

void QwtChartZoom::labelSelected(bool selected)
{
    setEnabled(!selected);
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
void QwtChartZoom::QScaleBounds::set(double mn, double mx, int axis)
{
    // если границы еще не фиксированы, фиксируем их
    if (!fixed) fix();
    // устанавливаем нижнюю и верхнюю границы шкалы
    plot->setAxisScale(axis == -1?master:axis, mn,mx);   // основной
}

// Восстановление исходных границ шкалы
void QwtChartZoom::QScaleBounds::reset() {
    // если границы уже фиксированы, то восстанавливаем исходные
    if (fixed) set(min,max,-1);
}

// Переустановка границ дополнительной шкалы
void QwtChartZoom::QScaleBounds::dup()
{
    // если границы еще не фиксированы, фиксируем их
    if (!fixed) fix();
    // получаем карту основной шкалы
    QwtScaleMap sm = plot->canvasMap(master);
    // и устанавливаем границы для дополнительной
    plot->setAxisScale(slave,ak*sm.s1()+bk,ak*sm.s2()+bk);
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
    if (zoom->activated)
        // если событие произошло для графика, то
        if (target == zoom->plot()) {
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
        case QEvent::MouseButtonDblClick:
            if (mEvent->button() == Qt::LeftButton)
                zoom->resetBounds(Qt::Horizontal | Qt::Vertical);
            break;
            // для прочих событий ничего не делаем
        default: ;
    }
}

void QMainZoomSvc::procKeyboardEvent(QEvent *event)
{
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
        default: break;
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
                emit xAxisClicked(plt->canvasMap(QwtPlot::xBottom).invTransform(scp_x),
                                  mEvent->modifiers() & Qt::ControlModifier);
                // прописываем соответствующий признак режима
                zoom->setRegime(QwtChartZoom::ctZoom);
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
          //  if (xp < scp_x || yp < scp_y) zoom->resetBounds();
            // иначе если размер выделенной области достаточен, то изменяем масштаб
           /* else*/ if (qAbs(xp - scp_x) >= 8 && qAbs(yp - scp_y) >= 8)
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
}

// Прикрепление интерфейса к менеджеру масштабирования
void QDragZoomSvc::attach(QwtChartZoom *zm)
{
    // запоминаем указатель на менеджер масштабирования
    zoom = zm;
    // назначаем для графика обработчик событий (фильтр событий)
    zm->plot()->installEventFilter(this);
}

// Обработчик всех событий
bool QDragZoomSvc::eventFilter(QObject *target,QEvent *event)
{
  //  if (zoom->activated)
        if (target == zoom->plot())
            if (event->type() == QEvent::MouseButtonPress ||
                event->type() == QEvent::MouseMove ||
                event->type() == QEvent::MouseButtonRelease)
                dragMouseEvent(event);
    return QObject::eventFilter(target,event);
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
    zoom->horizontalScaleBounds->set(scb_xl + dx,scb_xr + dx,-1);
    // аналогично определяем dy - смещение границ по вертикальной оси
    double dy = -(evpos.y() - cg.y() - scp_y) * cs_ky;
    // устанавливаем новые нижнюю и верхнюю границы вертикальной шкалы
    zoom->verticalScaleBounds->set(scb_yb + dy,scb_yt + dy,-1);
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
    case QEvent::MouseMove: proceedDrag(mEvent); break;
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
                zoom->setRegime(QwtChartZoom::ctDrag);
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
            }
    }
}

// Обработчик перемещения мыши
// (выполнение перемещения или выбор нового положения графика)
void QDragZoomSvc::proceedDrag(QMouseEvent *mEvent)
{
    // если включен режим перемещения графика, то
    if (zoom->regim() == QwtChartZoom::ctDrag)
    {
        // устанавливаем курсор ClosedHand
        zoom->plot()->canvas()->setCursor(Qt::ClosedHandCursor);
        // применяем результаты перемещения графика
        applyDrag(mEvent->pos());
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
            // восстанавливаем курсор
            zoom->plot()->canvas()->setCursor(tCursor);
            zoom->setRegime(QwtChartZoom::ctNone);  // и очищаем признак режима
        }
}
