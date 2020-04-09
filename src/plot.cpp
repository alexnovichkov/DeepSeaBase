#include "plot.h"

#include <qwt_plot_canvas.h>
//#include <qwt_legend.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_curve.h>
#include <qwt_symbol.h>

#include "fileformats/dfdfiledescriptor.h"
#include "fileformats/ufffile.h"
#include "curve.h"
#include "linecurve.h"
#include "barcurve.h"

#include "chartzoom.h"

#include <qwt_plot_zoomer.h>
#include <qwt_picker.h>
#include <qwt_picker_machine.h>
#include <qwt_plot_renderer.h>
#include <qwt_plot_layout.h>
#include <qwt_plot_panner.h>
#include <qwt_plot_magnifier.h>
#include <qwt_text.h>

#include <QtCore>
#include <QMessageBox>
#include <QFileDialog>
#include <QDesktopWidget>
#include <QApplication>
#include <QClipboard>
#include <QPrinter>
#include <QPrintDialog>
#include <QMenu>
#include <QAction>
#include <QAudioOutput>

#include "mainwindow.h"
#include "curvepropertiesdialog.h"

#include "colorselector.h"
#include "legend.h"

#include "plotpicker.h"
#include "logscaleengine.h"

#include "logging.h"
#include "trackingpanel.h"

#include "dataiodevice.h"


#include "playpanel.h"


// простой фабричный метод создания кривой нужного типа
Curve * createCurve(const QString &legendName, FileDescriptor *descriptor, int channel)
{
    // считаем, что шаг по оси х 0 только у октав и третьоктав
    if (descriptor->channel(channel)->xStep()==0.0)
        return new BarCurve(legendName, descriptor, channel);

    return new LineCurve(legendName, descriptor, channel);
}


Plot::Plot(QWidget *parent) :
    QwtPlot(parent), zoom(0)
{DD;
    canvas = new QwtPlotCanvas();
    canvas->setFocusIndicator(QwtPlotCanvas::CanvasFocusIndicator);
    canvas->setPalette(Qt::white);
    canvas->setFrameStyle(QFrame::StyledPanel);
    setCanvas(canvas);
    //setContextMenuPolicy(Qt::ActionsContextMenu);

    setAutoReplot(true);

    trackingPanel = new TrackingPanel(this);
    trackingPanel->setVisible(false);
    connect(trackingPanel,SIGNAL(closeRequested()),SIGNAL(trackingPanelCloseRequested()));
    connect(this, SIGNAL(curvesChanged()), trackingPanel, SLOT(update()));


    playerPanel = new PlayPanel(this);
    playerPanel->setVisible(false);
    connect(playerPanel,SIGNAL(closeRequested()),SIGNAL(playerPanelCloseRequested()));
    connect(this, SIGNAL(curvesChanged()), playerPanel, SLOT(update()));

    axisLabelsVisible = MainWindow::getSetting("axisLabelsVisible", true).toBool();
    yValuesPresentationLeft = DataHolder::ShowAsDefault;
    yValuesPresentationRight = DataHolder::ShowAsDefault;


//    setAxesCount(QwtPlot::xBottom,2);
    interactionMode = ScalingInteraction;

    // grid
    grid = new QwtPlotGrid;
    grid->enableXMin(true);
    grid->setMajorPen(Qt::gray, 0, Qt::DotLine);
    grid->setMinorPen(Qt::darkGray, 0, Qt::DotLine);
    grid->attach(this);

    xScale = false;
    y1Scale = false;
    y2Scale = false;

    CheckableLegend *leg = new CheckableLegend();
    connect(leg, SIGNAL(clicked(QwtPlotItem*)),this,SLOT(editLegendItem(QwtPlotItem*)));
    connect(leg, SIGNAL(markedForDelete(QwtPlotItem*)),this, SLOT(deleteCurveFromLegend(QwtPlotItem*)));
    connect(leg, SIGNAL(markedToMove(QwtPlotItem*)),this, SLOT(moveCurve(QwtPlotItem*)));
    connect(leg, SIGNAL(fixedChanged(QwtPlotItem*)),this, SLOT(fixCurve(QwtPlotItem*)));
    insertLegend(leg, QwtPlot::RightLegend);


    zoom = new ChartZoom(this);
    zoom->setZoomEnabled(true);
    connect(zoom,SIGNAL(updateTrackingCursor(double,bool)), trackingPanel, SLOT(setXValue(double,bool)));
    connect(zoom,SIGNAL(contextMenuRequested(QPoint,int)),SLOT(showContextMenu(QPoint,int)));
    connect(zoom,SIGNAL(moveCursor(bool)), trackingPanel, SLOT(moveCursor(bool)));

    picker = new PlotPicker(canvas);
    connect(picker,SIGNAL(labelSelected(bool)),zoom,SLOT(labelSelected(bool)));
    connect(picker,SIGNAL(updateTrackingCursor(double,bool)),trackingPanel, SLOT(setXValue(double,bool)));
    connect(picker,SIGNAL(cursorMovedTo(QwtPlotMarker*,double)), trackingPanel, SLOT(setXValue(QwtPlotMarker*,double)));
    connect(picker,SIGNAL(cursorSelected(QwtPlotMarker*)), trackingPanel, SLOT(updateSelectedCursor(QwtPlotMarker*)));
    connect(picker,SIGNAL(moveCursor(bool)), trackingPanel, SLOT(moveCursor(bool)));
    // передаем информацию об установленном курсоре в picker, чтобы тот смог обновить инфу о выделенном курсоре
    connect(trackingPanel,SIGNAL(cursorSelected(QwtPlotMarker*)), picker, SLOT(updateSelectedCursor(QwtPlotMarker*)));

    connect(picker,SIGNAL(cursorSelected(QwtPlotMarker*)), playerPanel, SLOT(updateSelectedCursor(QwtPlotMarker*)));
    connect(picker,SIGNAL(cursorMovedTo(QwtPlotMarker*,double)), playerPanel, SLOT(setXValue(QwtPlotMarker*,double)));


    picker->setEnabled(MainWindow::getSetting("pickerEnabled", true).toBool());
}

Plot::~Plot()
{DD;
    delete trackingPanel;
    delete playerPanel;
    qDeleteAll(curves);
    delete grid;
    delete picker;

    MainWindow::setSetting("axisLabelsVisible", axisLabelsVisible);
    MainWindow::setSetting("autoscale-x", !zoom->horizontalScaleBounds->isFixed());
    MainWindow::setSetting("autoscale-y", !zoom->verticalScaleBounds->isFixed());
    MainWindow::setSetting("autoscale-y-slave", !zoom->verticalScaleBoundsSlave->isFixed());
    delete zoom;
}

void Plot::update()
{DD;
    updateAxes();
    updateAxesLabels();
    updateLegend();
    if (leftCurves.isEmpty())
        zoom->verticalScaleBounds->reset();
    if (rightCurves.isEmpty())
        zoom->verticalScaleBoundsSlave->reset();
    if (!hasCurves())
        zoom->horizontalScaleBounds->reset();

    if (!zoom->horizontalScaleBounds->isFixed())
        zoom->horizontalScaleBounds->autoscale();
    if (!leftCurves.isEmpty() && !zoom->verticalScaleBounds->isFixed())
        zoom->verticalScaleBounds->autoscale();
    if (!rightCurves.isEmpty() && !zoom->verticalScaleBoundsSlave->isFixed())
        zoom->verticalScaleBoundsSlave->autoscale();

    replot();
}

bool Plot::hasCurves() const
{DD;
    return !curves.isEmpty();
}

void Plot::deleteAllCurves(bool forceDeleteFixed)
{DD;
    int leftUndeleted = curves.size();

    for (int i=curves.size()-1; i>=0; --i) {
        Curve *c = curves[i];
        if (!c->fixed || forceDeleteFixed) {
            emit curveDeleted(c->descriptor, c->channelIndex);
            deleteCurve(c, true);
            leftUndeleted--;
        }
    }

//    qDeleteAll(curves);
//    curves.clear();
//    leftCurves.clear();
//    rightCurves.clear();
//    ColorSelector::instance()->resetState();
    playerPanel->reset();

//    yLeftName.clear();
//    yRightName.clear();
//    xName.clear();

//    update();
//    emit curvesChanged();
}

void Plot::deleteCurvesForDescriptor(FileDescriptor *descriptor)
{DD;
    for (int i = curves.size()-1; i>=0; --i) {
        Curve *curve = curves[i];
        if (descriptor == curve->descriptor) {
            deleteCurve(curve, true);
        }
    }
}

//не удаляем, если фиксирована
void Plot::deleteCurveFromLegend(QwtPlotItem*item)
{DD;
    if (Curve *c = dynamic_cast<Curve *>(item)) {
        if (!c->fixed) {
            emit curveDeleted(c->descriptor, c->channelIndex);
            deleteCurve(c, true);
        }
    }
}

void Plot::deleteCurveForChannelIndex(FileDescriptor *dfd, int channel, bool doReplot)
{DD;
    if (Curve *curve = plotted(dfd, channel)) {
        deleteCurve(curve, doReplot);
    }
}

//удаляет кривую, была ли она фиксирована или нет.
//Все проверки проводятся выше
void Plot::deleteCurve(Curve *curve, bool doReplot)
{DD;
    if (curve) {
        ColorSelector::instance()->freeColor(curve->pen().color());

        int removed = leftCurves.removeAll(curve);
        if (removed > 0) {
            zoom->verticalScaleBounds->removeToAutoscale(curve->yMin(), curve->yMax());
        }

        removed = rightCurves.removeAll(curve);
        if (removed > 0) {
            zoom->verticalScaleBoundsSlave->removeToAutoscale(curve->yMin(), curve->yMax());
        }
        removed = curves.removeAll(curve);
        if (removed > 0) {
            zoom->horizontalScaleBounds->removeToAutoscale(curve->xMin(), curve->xMax());
        }
        QString title = curve->title();


        delete curve;
        curve = 0;
        checkDuplicates(title);

        if (leftCurves.isEmpty()) {
            yLeftName.clear();
        }
        if (rightCurves.isEmpty()) {
            yRightName.clear();
            enableAxis(QwtPlot::yRight, false);
        }
        if (!hasCurves()) {
            xName.clear();
        }

        if (doReplot) {
            update();
        }
        emit curvesChanged();
    }
}

void Plot::showContextMenu(const QPoint &pos, const int axis)
{DD;
    if (!hasCurves()) return;

    QMenu *menu = new QMenu(this);
    bool *scale = 0;

    if (axis == QwtPlot::xBottom) scale = &xScale;
    if (axis == QwtPlot::yLeft) scale = &y1Scale;
    if (axis == QwtPlot::yRight) scale = &y2Scale;

    if (scale && axis == QwtPlot::xBottom)
    menu->addAction((*scale)?"Линейная шкала":"Логарифмическая шкала", [=](){
        QwtScaleEngine *engine = 0;
        if (!(*scale)) {
            engine = new LogScaleEngine();
//            engine = new QwtLogScaleEngine();
            engine->setBase(2);
            //engine->setTransformation(new LogTransform);
        }
        else {
            engine = new QwtLinearScaleEngine();
        }
        setAxisScaleEngine(xBottomAxis, engine);

        if (scale) *scale = !(*scale);
    });

    // определяем, все ли графики представляют временные данные
    bool time = true;

    foreach (Curve *c, curves) {
        if (c->channel->type() != Descriptor::TimeResponse) {
            time = false;
            break;
        }
    }

    if (time && axis == QwtPlot::xBottom) {
        menu->addAction("Сохранить временной сегмент", [=](){
            double xStart = canvasMap(xBottom).s1();
            double xEnd = canvasMap(xBottom).s2();

            QList<FileDescriptor*> files;

            foreach(Curve *c, curves) {
                if (!files.contains(c->descriptor))
                    files << c->descriptor;
            }

            emit saveTimeSegment(files, xStart, xEnd);
        });
    }

    QList<Curve*> list;
    int *ax = 0;
    if (axis == QwtPlot::yLeft && !leftCurves.isEmpty()) {
        list = leftCurves;
        ax = &yValuesPresentationLeft;
    }
    if (axis == QwtPlot::yRight && !rightCurves.isEmpty()) {
        list = rightCurves;
        ax = &yValuesPresentationRight;
    }
    if (!list.isEmpty()) {
        QAction *a = new QAction("Показывать как");
        QMenu *am = new QMenu(this);
        QActionGroup *ag = new QActionGroup(am);

        QAction *act3 = new QAction("Амплитуды линейные", ag);
        act3->setCheckable(true);
        if (*ax == DataHolder::ShowAsAmplitudes) act3->setChecked(true);
        act3->setData(DataHolder::ShowAsAmplitudes);
        am->addAction(act3);

        QAction *act4 = new QAction("Амплитуды в дБ", ag);
        act4->setCheckable(true);
        if (*ax == DataHolder::ShowAsAmplitudesInDB) act4->setChecked(true);
        act4->setData(DataHolder::ShowAsAmplitudesInDB);
        am->addAction(act4);

        QAction *act5 = new QAction("Фазы", ag);
        act5->setCheckable(true);
        if (*ax == DataHolder::ShowAsPhases) act5->setChecked(true);
        act5->setData(DataHolder::ShowAsPhases);
        am->addAction(act5);

        QAction *act1 = new QAction("Действит.", ag);
        act1->setCheckable(true);
        if (*ax == DataHolder::ShowAsReals) act1->setChecked(true);
        act1->setData(DataHolder::ShowAsReals);
        am->addAction(act1);

        QAction *act2 = new QAction("Мнимые", ag);
        act2->setCheckable(true);
        if (*ax == DataHolder::ShowAsImags) act2->setChecked(true);
        act2->setData(DataHolder::ShowAsImags);
        am->addAction(act2);

        connect(ag, &QActionGroup::triggered, [=](QAction*act){
            int presentation = act->data().toInt();
            foreach (Curve *c, list) {
                c->channel->data()->setYValuesPresentation(presentation);
            }
            *ax = presentation;
            this->recalculateScale(axis == QwtPlot::yLeft);
            this->update();
        });

        a->setMenu(am);
        menu->addAction(a);
    }

    if (!leftCurves.isEmpty() && !rightCurves.isEmpty()) {
        menu->addSection("Левая и правая оси");
        menu->addAction("Совместить нули левой и правой осей", [=](){
            // 1. Центруем нуль левой оси
            QwtScaleMap leftMap = canvasMap(yLeft);
            double s1 = leftMap.s1();
            double s2 = leftMap.s2();

            double p1 = leftMap.p1();
            double p2 = leftMap.p2();
            double delta = leftMap.invTransform((p1+p2)/2.0);

            ChartZoom::zoomCoordinates coords;
            coords.coords.insert(QwtPlot::yLeft, {s1-delta, s2-delta});

            // 2. Центруем нуль правой оси
            QwtScaleMap rightMap = canvasMap(yRight);
            s1 = rightMap.s1();
            s2 = rightMap.s2();

            p1 = rightMap.p1();
            p2 = rightMap.p2();
            delta = rightMap.invTransform((p1+p2)/2.0);

            coords.coords.insert(QwtPlot::yRight, {s1-delta, s2-delta});
            zoom->addZoom(coords, true);
        });
        menu->addAction("Совместить диапазоны левой и правой осей", [=](){
            QwtScaleMap leftMap = canvasMap(yLeft);
            double s1 = leftMap.s1();
            double s2 = leftMap.s2();

            QwtScaleMap rightMap = canvasMap(yRight);
            double s3 = rightMap.s1();
            double s4 = rightMap.s2();
            double s = std::min(s1,s3);
            double ss = std::min(s2,s4);

            ChartZoom::zoomCoordinates coords;
            coords.coords.insert(QwtPlot::yLeft, {s, ss});
            coords.coords.insert(QwtPlot::yRight, {s, ss});

            zoom->addZoom(coords, true);
        });
    }

    menu->exec(pos);
}

bool Plot::canBePlottedOnLeftAxis(Channel *ch)
{DD;
    if (!hasCurves()) // нет графиков - можем построить что угодно
        return true;
    if (abscissaType(ch->xName()) == abscissaType(xName) || xName.isEmpty()) { // тип графика совпадает
        if (leftCurves.isEmpty() || yLeftName.isEmpty() || ch->yName() == yLeftName)
            return true;
    }
    return false;
}

bool Plot::canBePlottedOnRightAxis(Channel *ch)
{DD;
    if (!hasCurves()) // нет графиков - всегда на левой оси
        return true;
    if (abscissaType(ch->xName()) == abscissaType(xName) || xName.isEmpty()) { // тип графика совпадает
        if (rightCurves.isEmpty() || yRightName.isEmpty() || ch->yName() == yRightName)
            return true;
    }
    return false;
}

void Plot::prepareAxis(int axis)
{DD;
    if (!axisEnabled(axis))
        enableAxis(axis);
}

void Plot::setAxis(int axis, const QString &name)
{DD;
    switch (axis) {
        case QwtPlot::yLeft: yLeftName = name; break;
        case QwtPlot::yRight: yRightName = name; break;
        case QwtPlot::xBottom: xName = name; break;
        default: break;
    }
}

void Plot::moveToAxis(int axis, double min, double max)
{DD;
    switch (axis) {
        case QwtPlot::xBottom:
            zoom->horizontalScaleBounds->add(min, max);
            if (!zoom->horizontalScaleBounds->isFixed()) zoom->horizontalScaleBounds->autoscale();
            break;
        case QwtPlot::yLeft:
            zoom->verticalScaleBoundsSlave->removeToAutoscale(min, max);
            zoom->verticalScaleBounds->add(min, max);
            if (!zoom->verticalScaleBounds->isFixed()) zoom->verticalScaleBounds->autoscale();
            break;
        case QwtPlot::yRight:
            zoom->verticalScaleBounds->removeToAutoscale(min, max);
            zoom->verticalScaleBoundsSlave->add(min, max);
            if (!zoom->verticalScaleBoundsSlave->isFixed()) zoom->verticalScaleBoundsSlave->autoscale();
            break;
        default: break;
    }
}

void Plot::updateAxesLabels()
{DD;
    if (leftCurves.isEmpty()) enableAxis(QwtPlot::yLeft, false);
    else {
        enableAxis(QwtPlot::yLeft, axisLabelsVisible);
        QString suffix = yValuesPresentationSuffix(yValuesPresentationLeft);
        QwtText text(QString("%1 <small>%2</small>").arg(yLeftName).arg(suffix), QwtText::RichText);
        if (axisLabelsVisible)
            setAxisTitle(QwtPlot::yLeft, text);
        else
            setAxisTitle(QwtPlot::yLeft, "");
    }

    if (rightCurves.isEmpty()) enableAxis(QwtPlot::yRight, false);
    else {
        enableAxis(QwtPlot::yRight, axisLabelsVisible);
        QString suffix = yValuesPresentationSuffix(yValuesPresentationRight);
        QwtText text(QString("%1 <small>%2</small>").arg(yRightName).arg(suffix), QwtText::RichText);
        if (axisLabelsVisible)
            setAxisTitle(QwtPlot::yRight, text);
        else
            setAxisTitle(QwtPlot::yRight, "");
    }
    if (axisEnabled(QwtPlot::xBottom)) {
        setAxisTitle(QwtPlot::xBottom, axisLabelsVisible ? xName : "");
    }
}

void Plot::removeLabels()
{
    foreach (Curve *c, curves) {
        c->removeLabels();
    }
   replot();
}

void Plot::moveCurve(Curve *curve, int axis)
{DD;
    if ((axis == QwtPlot::yLeft && canBePlottedOnLeftAxis(curve->channel))
        || (axis == QwtPlot::yRight && canBePlottedOnRightAxis(curve->channel))) {
        prepareAxis(axis);
        setAxis(axis, curve->channel->yName());
        curve->setYAxis(axis);

        if (leftCurves.contains(curve)) {
            leftCurves.removeAll(curve);
            if (rightCurves.isEmpty()) {
                yValuesPresentationRight = curve->channel->data()->yValuesPresentation();
            }
            curve->channel->data()->setYValuesPresentation(yValuesPresentationRight);
            rightCurves.append(curve);
        }
        else if (rightCurves.contains(curve)) {
            rightCurves.removeAll(curve);
            if (leftCurves.isEmpty()) {
                yValuesPresentationLeft = curve->channel->data()->yValuesPresentation();
            }
            curve->channel->data()->setYValuesPresentation(yValuesPresentationLeft);
            leftCurves.append(curve);
        }
        emit curvesChanged();

        updateAxesLabels();
        moveToAxis(axis, curve->channel->yMin(), curve->channel->yMax());
    }
    else QMessageBox::warning(this, "Не могу поменять ось", "Эта ось уже занята графиком другого типа!");


}

void Plot::moveCurve(QwtPlotItem *curve)
{
    if (Curve *c = dynamic_cast<Curve*>(curve))
        moveCurve(c, c->yAxis() == QwtPlot::yLeft ? QwtPlot::yRight : QwtPlot::yLeft);
}

void Plot::fixCurve(QwtPlotItem *curve)
{
    if (Curve *c = dynamic_cast<Curve*>(curve)) {
        c->switchFixed();
        updateLegend();
    }

}

bool Plot::hasDuplicateNames(const QString name) const
{
    int count = 0;
    foreach(Curve *c, curves) {
        if (c->title() == name) count++;
    }
    return (count > 1);
}

void Plot::checkDuplicates(const QString name)
{
    QList<int> l;
    for(int i=0; i<curves.size(); ++i) {
        if (curves[i]->title() == name) l << i;
    }
    foreach(int i, l) curves[i]->duplicate = l.size()>1;
}

QString Plot::yValuesPresentationSuffix(int yValuesPresentation) const
{
    switch (DataHolder::YValuesPresentation(yValuesPresentation)) {
        case DataHolder::ShowAsDefault: return QString();
        case DataHolder::ShowAsReals: return " [Re]";
        case DataHolder::ShowAsImags: return " [Im]";
        case DataHolder::ShowAsAmplitudes: return " [Abs]";
        case DataHolder::ShowAsAmplitudesInDB: return " [dB]";
        case DataHolder::ShowAsPhases: return " [Phase]";
        default: break;
    }
    return QString();
}

void Plot::recalculateScale(bool leftAxis)
{
    ChartZoom::ScaleBounds *ybounds = 0;
    if (leftAxis) ybounds = zoom->verticalScaleBounds;
    else ybounds = zoom->verticalScaleBoundsSlave;

    QList<Curve*> l;
    if (leftAxis) l = leftCurves;
    else l = rightCurves;

    ybounds->reset();
    foreach (Curve *c, l) {
        ybounds->add(c->yMin(), c->yMax());
    }

}

bool Plot::plotCurve(FileDescriptor *descriptor, int channel, QColor *col, bool &plotOnRight, int fileNumber)
{DD;
    if (plotted(descriptor, channel)) return false;

    Channel *ch = descriptor->channel(channel);
    if (!ch->populated()) {
        ch->populate();
    }

    bool plotOnFirstYAxis = canBePlottedOnLeftAxis(ch);
    bool plotOnSecondYAxis = canBePlottedOnRightAxis(ch);
    bool skipped = false;

    if (!plotOnRight) {//trying to plot on left
        if (!plotOnFirstYAxis) {//cannot plot on left
            if (!plotOnSecondYAxis) {//cannot plot on right either
                skipped = true;
            }
            else plotOnRight = true; //confirm plotting on right
        }
    }
    else /*if (plotOnRight)*/ {//trying to plot on right
        if (!plotOnSecondYAxis) {//cannot plot on right
            if (!plotOnFirstYAxis) {//cannot plot on left either
                skipped = true;
            }
            else plotOnRight = false; //confirm plotting on left
        }
    }

    if (skipped) {
        QMessageBox::warning(this, QString("Не могу построить канал"),
                             QString("Тип графика не подходит.\n"
                                     "Сначала очистите график."));
        return false;
    }


    setAxis(QwtPlot::xBottom, descriptor->xName());
    prepareAxis(QwtPlot::xBottom);

    // если графиков нет, по умолчанию будем строить амплитуды по первому добавляемому графику
    if (!plotOnRight && leftCurves.isEmpty()) {
        yValuesPresentationLeft = ch->data()->yValuesPresentation();
    }
    if (plotOnRight && rightCurves.isEmpty()) {
        yValuesPresentationRight = ch->data()->yValuesPresentation();
    }

    if (!plotOnRight) ch->data()->setYValuesPresentation(yValuesPresentationLeft);
    else ch->data()->setYValuesPresentation(yValuesPresentationRight);

    QwtPlot::Axis ax = plotOnRight ? QwtPlot::yRight : QwtPlot::yLeft;

    setAxis(ax, ch->yName());
    prepareAxis(ax);


    Curve *g = createCurve(ch->legendName(), descriptor, channel);
    QColor nextColor = ColorSelector::instance()->getColor();
    QPen pen = g->pen();
    pen.setColor(nextColor);
    pen.setWidth(1);
    g->setPen(pen);
    g->oldPen = pen;
    if (col) *col = nextColor;

    curves << g;
    g->fileNumber = fileNumber;
    checkDuplicates(g->title());
    if (hasDuplicateNames(g->title())) {
        g->duplicate = true;
    }

    g->setYAxis(ax);
    if (plotOnRight) {
        rightCurves << g;
    }
    else {
        leftCurves << g;
    }


    ChartZoom::ScaleBounds *ybounds = 0;
    if (zoom->verticalScaleBounds->axis == ax) ybounds = zoom->verticalScaleBounds;
    else ybounds = zoom->verticalScaleBoundsSlave;

    zoom->horizontalScaleBounds->add(g->xMin(), g->xMax());
    ybounds->add(g->yMin(), g->yMax());

    g->attachTo(this);
    update();
    emit curvesChanged();

    return true;
}

Curve * Plot::plotted(FileDescriptor *dfd, int channel) const
{DD;
    foreach (Curve *curve, curves) {
        if (curve->descriptor == dfd && curve->channelIndex == channel) return curve;
    }
    return 0;
}

Curve * Plot::plotted(Channel *channel) const
{DD;
    foreach (Curve *curve, curves) {
        if (curve->channel == channel) return curve;
    }
    return 0;
}

Range Plot::xRange() const
{
    QwtScaleMap sm = canvasMap(QwtPlot::xBottom);
    return {sm.s1(), sm.s2()};
}

Range Plot::yLeftRange() const
{
    QwtScaleMap sm = canvasMap(QwtPlot::yLeft);
    return {sm.s1(), sm.s2()};
}

Range Plot::yRightRange() const
{
    QwtScaleMap sm = canvasMap(QwtPlot::yRight);
    return {sm.s1(), sm.s2()};
}

void Plot::switchLabelsVisibility()
{
    axisLabelsVisible = !axisLabelsVisible;
    updateAxesLabels();
}

void Plot::updateLegends()
{DD;
    foreach (Curve *curve, leftCurves) {
        curve->setTitle(curve->channel->legendName());
    }
    foreach (Curve *curve, rightCurves) {
        curve->setTitle(curve->channel->legendName());
    }
    updateLegend();
}

void Plot::savePlot()
{DD;
    QString lastPicture = MainWindow::getSetting("lastPicture", "plot.bmp").toString();
    lastPicture = QFileDialog::getSaveFileName(this, QString("Сохранение графика"), lastPicture,
                                               "Растровые изображения (*.bmp);;Файлы pdf (*.pdf);;Файлы svg (*.svg)");
    if (lastPicture.isEmpty()) return;

    importPlot(lastPicture);

    MainWindow::setSetting("lastPicture", lastPicture);
}

void Plot::copyToClipboard()
{DD;
    QTemporaryFile file("DeepSeaBase-XXXXXX.bmp");
    if (file.open()) {
        QString fileName = file.fileName();
        file.close();

        importPlot(fileName);
        QImage img;
        if (img.load(fileName)) {
            qApp->clipboard()->setImage(img);
        }
    }
}

void Plot::print()
{DD;
    QTemporaryFile file("DeepSeaBase-XXXXXX.bmp");
    if (file.open()) {
        QString fileName = file.fileName();
        file.close();

        importPlot(fileName);

        QImage img;
        if (img.load(fileName)) {
            QPrinter printer;
            printer.setOrientation(QPrinter::Landscape);

            QPrintDialog printDialog(&printer, this);
            if (printDialog.exec()) {
                qreal left,right,top,bottom;
                printer.getPageMargins(&left, &top, &right, &bottom, QPrinter::Millimeter);
                left = 15.0;
                top = 40.0;
                printer.setPageMargins(left, top, right, bottom, QPrinter::Millimeter);
                QPainter painter(&printer);
                QRect rect = painter.viewport();
                QSize size = img.size();
                size.scale(rect.size(), Qt::KeepAspectRatio);
                painter.setViewport(rect.x(), rect.y(),
                                    size.width(), size.height());
                painter.setWindow(img.rect());
                painter.drawImage(0, 0, img);
            }
        }
    }
}

void Plot::switchInteractionMode()
{DD;
    if (interactionMode == ScalingInteraction) {
        setInteractionMode(DataInteraction);
    }
    else {
        setInteractionMode(ScalingInteraction);
    }
}

void Plot::switchTrackingCursor()
{DD;
    trackingPanel->switchVisibility();
}

void Plot::switchPlayerVisibility()
{DD;
    playerPanel->switchVisibility();
}

void Plot::toggleAutoscale(int axis, bool toggled)
{
    switch (axis) {
        case 0: // x axis
            zoom->horizontalScaleBounds->setFixed(!toggled);
            break;
        case 1: // y axis
            zoom->verticalScaleBounds->setFixed(!toggled);
            break;
        case 2: // y slave axis
            zoom->verticalScaleBoundsSlave->setFixed(!toggled);
            break;
        default:
            break;
    }
}

void Plot::autoscale(int axis)
{
    switch (axis) {
        case 0: // x axis
            zoom->horizontalScaleBounds->autoscale();
            break;
        case 1: // y axis
            zoom->verticalScaleBounds->autoscale();
            break;
        case 2: // y slave axis
            zoom->verticalScaleBoundsSlave->autoscale();
            break;
        default:
            break;
    }
}

void Plot::setInteractionMode(Plot::InteractionMode mode)
{DD;
    interactionMode = mode;
    if (picker) picker->setMode(mode);
    if (zoom)
        zoom->setZoomEnabled(mode == ScalingInteraction);
    if (canvas) canvas->setFocusIndicator(mode == ScalingInteraction?
                                              QwtPlotCanvas::CanvasFocusIndicator:
                                              QwtPlotCanvas::ItemFocusIndicator);
}

void Plot::importPlot(const QString &fileName)
{DD;
    QwtPlotRenderer renderer;
    renderer.setDiscardFlag(QwtPlotRenderer::DiscardBackground);
    renderer.setLayoutFlag(QwtPlotRenderer::FrameWithScales);

    QFont axisfont = axisFont(QwtPlot::yLeft);
    axisfont.setPointSize(axisfont.pointSize()+1);

    for (int i=0; i<QwtPlot::axisCnt; ++i)
        if (axisEnabled(i)) setAxisFont(i, axisfont);

    foreach (Curve *curve, curves) {
        QPen pen = curve->pen();
        if (pen.width()<2) pen.setWidth(2);
        pen.setColor(pen.color().lighter(120));
        curve->setPen(pen);
//        curve->setTitle(QwtText("<font size=5>"+curve->channel->legendName()+"</font>"));
    }

    QwtAbstractLegend *leg = new QwtLegend();
    insertLegend(leg, QwtPlot::BottomLegend);


    QString format = fileName.section(".", -1,-1);
    renderer.renderDocument(this, fileName, format,
                            QSizeF(400,200), qApp->desktop()->logicalDpiX());


    axisfont.setPointSize(axisfont.pointSize()-1);
    for (int i=0; i<QwtPlot::axisCnt; ++i)
        if (axisEnabled(i)) setAxisFont(i, axisfont);

    foreach (Curve *curve, curves) {
        curve->setPen(curve->oldPen);
        curve->setTitle(curve->channel->legendName());
    }

    leg = new CheckableLegend();
    connect(leg, SIGNAL(clicked(QwtPlotItem*)),this,SLOT(editLegendItem(QwtPlotItem*)));
    connect(leg, SIGNAL(markedForDelete(QwtPlotItem*)),this, SLOT(deleteCurveFromLegend(QwtPlotItem*)));
    connect(leg, SIGNAL(markedToMove(QwtPlotItem*)),this, SLOT(moveCurve(QwtPlotItem*)));
    connect(leg, SIGNAL(fixedChanged(QwtPlotItem*)),this, SLOT(fixCurve(QwtPlotItem*)));
    insertLegend(leg, QwtPlot::RightLegend);
}

void Plot::switchCursor()
{DD;
    if (!picker) return;

    bool pickerEnabled = picker->isEnabled();
    picker->setEnabled(!pickerEnabled);
    MainWindow::setSetting("pickerEnabled", !pickerEnabled);
}

void Plot::editLegendItem(const QVariant &itemInfo, int index)
{DD;
    Q_UNUSED(index)

    QwtPlotItem *item = infoToItem(itemInfo);
    if (item) {
        Curve *c = dynamic_cast<Curve *>(item);
        if (c) {
            CurvePropertiesDialog dialog(c, this);
            connect(&dialog,SIGNAL(curveChanged(Curve*)),this, SIGNAL(curveChanged(Curve*)));
            connect(&dialog,SIGNAL(curveChanged(Curve*)),trackingPanel, SLOT(update()));
            dialog.exec();
        }
    }
}

void Plot::editLegendItem(QwtPlotItem *item)
{DD;
    Curve *c = dynamic_cast<Curve *>(item);
    if (c) {
        CurvePropertiesDialog dialog(c, this);
        connect(&dialog,SIGNAL(curveChanged(Curve*)),this, SIGNAL(curveChanged(Curve*)));
        connect(&dialog,SIGNAL(curveChanged(Curve*)),trackingPanel, SLOT(update()));
        dialog.exec();
    }
}
