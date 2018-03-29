#include "plot.h"

#include <qwt_plot_canvas.h>
//#include <qwt_legend.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_curve.h>
#include <qwt_symbol.h>

#include "dfdfiledescriptor.h"
#include "curve.h"

#include "qwtchartzoom.h"

#include <qwt_plot_zoomer.h>
#include <qwt_picker.h>
#include <qwt_picker_machine.h>
#include <qwt_plot_renderer.h>
#include <qwt_scale_engine.h>
#include <qwt_plot_layout.h>
#include <qwt_plot_panner.h>
#include <qwt_plot_magnifier.h>

#include <QtCore>
#include <QMessageBox>
#include <QFileDialog>
#include <QDesktopWidget>
#include <QApplication>
#include <QClipboard>
#include <QPrinter>
#include <QPrintDialog>
#include <QMenu>

#include "mainwindow.h"
#include "graphpropertiesdialog.h"

#include "colorselector.h"
#include "legend.h"

#include "plotpicker.h"
#include <QAction>

#include "logging.h"
#include "trackingpanel.h"

Plot::Plot(QWidget *parent) :
    QwtPlot(parent), zoom(0)
{DD;
    canvas = new QwtPlotCanvas();
    canvas->setFocusIndicator( QwtPlotCanvas::CanvasFocusIndicator);
    canvas->setFocusPolicy( Qt::StrongFocus);
    canvas->setPalette(Qt::white);
    canvas->setFrameStyle(QFrame::StyledPanel);
    setCanvas(canvas);
    //setContextMenuPolicy(Qt::ActionsContextMenu);

    setAutoReplot(true);



    trackingPanel = new TrackingPanel(this);
    trackingPanel->setVisible(false);
    connect(trackingPanel,SIGNAL(closeRequested()),SIGNAL(trackingPanelCloseRequested()));

    axisLabelsVisible = MainWindow::getSetting("axisLabelsVisible", true).toBool();



    interactionMode = ScalingInteraction;

    // grid
    grid = new QwtPlotGrid;
    grid->enableXMin(true);
    grid->setMajorPen(Qt::gray, 0, Qt::DotLine);
    grid->setMinorPen(Qt::darkGray, 0, Qt::DotLine);
    grid->attach(this);

    x1.clear();
    y1.clear();
    y2.clear();

    // axis
    setAxisScale(QwtPlot::yLeft, 0, 10);
    setAxisScale(QwtPlot::xBottom, 0, 10);


    xScale = false;
    y1Scale = false;
    y2Scale = false;

    Legend *leg = new Legend();
    connect(leg, SIGNAL(clicked(QVariant,int)),this,SLOT(editLegendItem(QVariant,int)));
    connect(leg, SIGNAL(markedForDelete(QVariant,int)),this, SLOT(deleteGraph(QVariant,int)));
    insertLegend(leg, QwtPlot::RightLegend);


//    QwtPlotPanner *panner = new QwtPlotPanner(canvas);
//    panner->setMouseButton(Qt::RightButton);
//    panner->setAxisEnabled(QwtPlot::xBottom, true);
//    panner->setAxisEnabled(QwtPlot::yLeft, true);
//    panner->setAxisEnabled(QwtPlot::yRight, false);
//    panner->setEnabled(true);

//    QwtPlotPanner *panner1 = new QwtPlotPanner(canvas);
//    panner1->setMouseButton(Qt::RightButton, Qt::ControlModifier);
//    panner1->setAxisEnabled(QwtPlot::xBottom, true);
//    panner1->setAxisEnabled(QwtPlot::yRight, true);
//    panner1->setAxisEnabled(QwtPlot::yLeft, false);
//    panner1->setEnabled(true);

//    QwtPlotMagnifier *magnifier = new QwtPlotMagnifier(canvas);
//    magnifier->setMouseButton(Qt::NoButton);

//    setAxisScaleEngine(QwtPlot::xBottom, new QwtLogScaleEngine());


    zoom = new QwtChartZoom(this);
    zoom->setEnabled(false);
    connect(zoom,SIGNAL(updateTrackingCursor(double,bool)), trackingPanel, SLOT(updateTrackingCursor(double,bool)));
    connect(zoom,SIGNAL(contextMenuRequested(QPoint,int)),SLOT(showContextMenu(QPoint,int)));

    picker = new PlotPicker(canvas);
    connect(picker,SIGNAL(labelSelected(bool)),zoom,SLOT(labelSelected(bool)));
    connect(picker,SIGNAL(updateTrackingCursor(double,bool)),trackingPanel, SLOT(updateTrackingCursor(double,bool)));
    connect(picker,SIGNAL(cursorMovedTo(QwtPlotMarker*,double)), trackingPanel, SLOT(updateTrackingCursor(QwtPlotMarker*,double)));
    connect(trackingPanel,SIGNAL(switchHarmonics(bool)),picker,SLOT(showHarmonics(bool)));

    picker->setEnabled(MainWindow::getSetting("pickerEnabled", true).toBool());
}

Plot::~Plot()
{DD;
//    delete freeGraph;
    delete trackingPanel;
    qDeleteAll(graphs);
    delete grid;
    delete zoom;
    delete picker;

    MainWindow::setSetting("axisLabelsVisible", axisLabelsVisible);
}

void Plot::update()
{DD;
    updateAxes();
    updateAxesLabels();
    updateLegend();
//    double x = _trackingCursor->value().x();
//    updateTrackingCursor(x, false);
//    x = _trackingCursor1->value().x();
//    updateTrackingCursor(x, true);
    replot();
}

bool Plot::hasGraphs() const
{DD;
    QwtPlotItemList curveList = itemList(QwtPlotItem::Rtti_PlotCurve);
    return !curveList.isEmpty();
}

int Plot::totalGraphsCount() const
{DD;
    QwtPlotItemList curveList = itemList(QwtPlotItem::Rtti_PlotCurve);
    return curveList.size();
}

void Plot::deleteGraphs()
{DD;
    qDeleteAll(graphs);
    graphs.clear();
    leftGraphs.clear();
    rightGraphs.clear();
    x1.clear();
    y1.clear();
    y2.clear();
    ColorSelector::instance()->resetState();
//    minStep = 0.0;

    yLeftName.clear();
    yRightName.clear();
    xName.clear();

    //delete zoom;
    //zoom = new QwtChartZoom(this);
    //connect(picker,SIGNAL(labelSelected(bool)),zoom,SLOT(labelSelected(bool)));
    //connect(zoom,SIGNAL(updateTrackingCursor(double,bool)),SLOT(updateTrackingCursor(double,bool)));
    //connect(zoom,SIGNAL(contextMenuRequested(QPoint,int)),SLOT(showContextMenu(QPoint,int)));

    update();
}

void Plot::deleteGraphs(FileDescriptor *descriptor)
{DD;
    for (int i = graphs.size()-1; i>=0; --i) {
        Curve *graph = graphs[i];
        if (descriptor == graph->descriptor) {
            deleteGraph(graph, true);
        }
    }
}

void Plot::deleteGraph(const QVariant &info, int index)
{DD;
    Q_UNUSED(index)

    QwtPlotItem *item = infoToItem(info);
    if (item) {
        Curve *c = dynamic_cast<Curve *>(item);
        if (c) {
            emit curveDeleted(c->descriptor, c->channelIndex);
            deleteGraph(c, true);
        }
    }
}

void Plot::showContextMenu(const QPoint &pos, const int axis)
{DD;
    QMenu *menu = new QMenu(this);
    bool *scale = 0;

    if (axis == QwtPlot::xBottom) scale = &xScale;
    if (axis == QwtPlot::yLeft) scale = &y1Scale;
    if (axis == QwtPlot::yRight) scale = &y2Scale;

    menu->addAction((*scale)?"Линейная шкала":"Логарифмическая шкала", [=](){
        QwtScaleEngine *engine = 0;
        if (!(*scale)) {
            engine = new QwtLogScaleEngine();
        }
        else {
            engine = new QwtLinearScaleEngine();
        }
        setAxisScaleEngine(axis, engine);

        if (scale) *scale = !(*scale);
    });
    menu->exec(pos);
}

void Plot::deleteGraph(FileDescriptor *dfd, int channel, bool doReplot)
{DD;
    if (Curve *graph = plotted(dfd, channel)) {
        deleteGraph(graph, doReplot);
    }
}

void Plot::deleteGraph(Channel *c, bool doReplot)
{DD;
    if (Curve *graph = plotted(c)) {
        deleteGraph(graph, doReplot);
    }
}

void Plot::deleteGraph(Curve *graph, bool doReplot)
{DD;
    if (graph) {
//        emit curveDeleted(graph);
        ColorSelector::instance()->freeColor(graph->pen().color());
        graphs.removeAll(graph);
        leftGraphs.removeAll(graph);
        rightGraphs.removeAll(graph);
        delete graph;
        graph = 0;

        if (leftGraphs.isEmpty()) {
            yLeftName.clear();
        }
        if (rightGraphs.isEmpty()) {
            yRightName.clear();
            enableAxis(QwtPlot::yRight, false);
        }
        if (!hasGraphs()) {
            xName.clear();
            x1.clear();
            y1.clear();
            y2.clear();
        }

        if (doReplot) {
            update();
        }
    }
}

bool Plot::canBePlottedOnLeftAxis(Channel *ch)
{DD;
    if (!hasGraphs()) // нет графиков - всегда на левой оси
        return true;
    if (ch->xName() == xName || xName.isEmpty()) { // тип графика совпадает
        if (leftGraphs.isEmpty() || yLeftName.isEmpty() || ch->yName() == yLeftName)
            return true;
    }
    return false;
}

bool Plot::canBePlottedOnRightAxis(Channel *ch)
{DD;
    if (!hasGraphs()) // нет графиков - всегда на левой оси
        return true;
    if (ch->xName() == xName || xName.isEmpty()) { // тип графика совпадает
        if (rightGraphs.isEmpty() || yRightName.isEmpty() || ch->yName() == yRightName)
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

void Plot::updateAxesLabels()
{DD;
    if (leftGraphs.isEmpty()) enableAxis(QwtPlot::yLeft, false);
    else {
        enableAxis(QwtPlot::yLeft, axisLabelsVisible);
        setAxisTitle(QwtPlot::yLeft, axisLabelsVisible ? yLeftName : "");
    }

    if (rightGraphs.isEmpty()) enableAxis(QwtPlot::yRight, false);
    else {
        enableAxis(QwtPlot::yRight, axisLabelsVisible);
        setAxisTitle(QwtPlot::yRight, axisLabelsVisible ? yRightName : "");
    }
    if (axisEnabled(QwtPlot::xBottom)) {
        setAxisTitle(QwtPlot::xBottom, axisLabelsVisible ? xName : "");
    }
}

void Plot::moveGraph(Curve *curve)
{DD;
    if (leftGraphs.contains(curve)) {
        leftGraphs.removeAll(curve);
        rightGraphs.append(curve);
    }
    else if (rightGraphs.contains(curve)) {
        rightGraphs.removeAll(curve);
        leftGraphs.append(curve);
    }
}

bool Plot::plotChannel(FileDescriptor *descriptor, int channel, QColor *col)
{DD;
    if (plotted(descriptor, channel)) return false;

    Channel *ch = descriptor->channel(channel);
    if (!ch->populated()) ch->populate();

    const bool plotOnFirstYAxis = canBePlottedOnLeftAxis(ch);
    const bool plotOnSecondYAxis = plotOnFirstYAxis ? false : canBePlottedOnRightAxis(ch);

    static bool skipped = false;
    if (!plotOnFirstYAxis && !plotOnSecondYAxis) {
        if (!skipped) {
            QMessageBox::warning(this, QString("Не могу построить канал"),
                                 QString("Тип графика не подходит.\n"
                                         "Сначала очистите график."));
            skipped = true;
        }
        return false;
    }
    else
        skipped = false;

    setAxis(QwtPlot::xBottom, descriptor->xName());
    prepareAxis(QwtPlot::xBottom);

    QwtPlot::Axis ax = plotOnFirstYAxis ? QwtPlot::yLeft : QwtPlot::yRight;

    setAxis(ax, ch->yName());
    prepareAxis(ax);

    Curve *g = new Curve(ch->legendName(), descriptor, channel);
    QColor nextColor = ColorSelector::instance()->getColor();
    QPen pen = g->pen();
    pen.setColor(nextColor);
    pen.setWidth(1);
    g->setPen(pen);
    g->oldPen = pen;
    if (col) *col = nextColor;

    graphs << g;

    g->setLegendIconSize(QSize(16,8));

    bool needFixBoundaries = !hasGraphs() && zoom;

    g->attach(this);

    g->setYAxis(ax);
    if (plotOnSecondYAxis) {
        rightGraphs << g;
        yRightName = ch->yName();
    }
    else {
        leftGraphs << g;
        yLeftName = ch->yName();
    }
    xName = descriptor->xName();

    // если график находится в своем исходном масштабе, то есть построен один
    // график или несколько, но еще не масштабированы, то меняем этот исходный масштаб на больший
    // иначе сохраням масштаб по осям
    QwtScaleMap smX = canvasMap(QwtPlot::xBottom);
    QwtScaleMap smY = canvasMap(ax);

    Range &r = y1;
    if (plotOnSecondYAxis) r = y2;

    if (graphs.size()<=1 || (smX.s1()==x1.min && smX.s2()==x1.max
                             && smY.s1()==r.min && smY.s2()==r.max)) {
        x1.min = qMin(ch->xBegin(), x1.min);
        x1.max = qMax(ch->xMaxInitial(), x1.max);
        setAxisScale(QwtPlot::xBottom, x1.min, x1.max);

        r.min = qMin(ch->yMinInitial(), r.min);
        r.max = qMax(ch->yMaxInitial(), r.max);
        setAxisScale(ax, r.min, r.max);
    }

    if (needFixBoundaries)
        if (zoom) zoom->fixBoundaries();

    update();
    return true;
}

Curve * Plot::plotted(FileDescriptor *dfd, int channel) const
{DD;
    foreach (Curve *graph, graphs) {
        if (graph->descriptor == dfd && graph->channelIndex == channel) return graph;
    }
    return 0;
}

Curve * Plot::plotted(Channel *channel) const
{DD;
    foreach (Curve *graph, graphs) {
        if (graph->channel == channel) return graph;
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
    foreach (Curve *graph, leftGraphs) {
        graph->setTitle(graph->channel->legendName());
    }
    foreach (Curve *graph, rightGraphs) {
        graph->setTitle(graph->channel->legendName());
    }
    updateLegend();
}

void Plot::savePlot()
{DD;
    QString lastPicture = MainWindow::getSetting("lastPicture", "plot.bmp").toString();
    lastPicture = QFileDialog::getSaveFileName(this, QString("Сохранение графика"), lastPicture, "Изображения (*.bmp)");
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

void Plot::switchHarmonicsMode()
{DD;
    picker->showHarmonics(!picker->harmonics());
}

void Plot::switchTrackingCursor()
{DD;
    trackingPanel->switchVisibility();
}

void Plot::setInteractionMode(Plot::InteractionMode mode)
{DD;
    interactionMode = mode;
    if (picker) picker->setMode(mode);
    if (zoom) zoom->setEnabled(mode == ScalingInteraction);
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

    foreach (Curve *graph, graphs) {
        QPen pen = graph->pen();
        if (pen.width()<2) pen.setWidth(2);
        pen.setColor(pen.color().lighter(120));
        graph->setPen(pen);
        graph->setTitle(QwtText("<font size=5>"+graph->channel->legendName()+"</font>"));
    }

    QwtLegend *leg = new QwtLegend();
    insertLegend(leg, QwtPlot::BottomLegend);


    renderer.renderDocument(this, fileName, QSizeF(400,200), qApp->desktop()->logicalDpiX());


    axisfont.setPointSize(axisfont.pointSize()-1);
    for (int i=0; i<QwtPlot::axisCnt; ++i)
        if (axisEnabled(i)) setAxisFont(i, axisfont);

    foreach (Curve *graph, graphs) {
        graph->setPen(graph->oldPen);
        graph->setTitle(QwtText(graph->channel->legendName()));
    }

    leg = new Legend();
    //leg->setDefaultItemMode(QwtLegendData::Clickable);
    connect(leg, SIGNAL(clicked(QVariant,int)),this,SLOT(editLegendItem(QVariant,int)));
    connect(leg, SIGNAL(markedForDelete(QVariant,int)),this, SLOT(deleteGraph(QVariant,int)));
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
            GraphPropertiesDialog dialog(c, this);
            connect(&dialog,SIGNAL(curveChanged(Curve*)),this, SIGNAL(curveChanged(Curve*)));
            dialog.exec();
        }
    }
}



