#include "plot.h"

#include <qwt_plot_canvas.h>
#include <qwt_legend.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_curve.h>
#include <qwt_symbol.h>

#include "dfdfiledescriptor.h"
#include "curve.h"

#include "qwtchartzoom.h"
#include "qwheelzoomsvc.h"
#include "qaxiszoomsvc.h"

#include <qwt_plot_zoomer.h>
#include <qwt_picker.h>
#include <qwt_picker_machine.h>
#include <qwt_plot_renderer.h>

#include <qwt_plot_layout.h>

#include <QMessageBox>
#include <QFileDialog>
#include <QDesktopWidget>
#include <QApplication>
#include "mainwindow.h"
#include "graphpropertiesdialog.h"

static QList<QColor> usedColors;

QColor getColor()
{
    static uint colors[16]={
        0x00b40000,
        0x00000080,
        0x00008080,
        0x00803f00,
        0x00ff8000,
        0x000000ff,
        0x00808000,
        0x0000ffff,
        0x00f0f0c0, //240, 240, 192
        0x00800080,
        0x00ff00ff,
        0x00007800, //0, 120, 0
        0x00000000,
        0x00ff8080,
        0x008080ff,
        0x00a0a0a4
    };


    for (int i=0; i<16; ++i) {
        QColor c = QColor(QRgb(colors[i]));
        if (!usedColors.contains(c)) {
            usedColors.append(c);
            return c;
        }
    }

    return QColor(QRgb(0x00808080));
}

Plot::Plot(QWidget *parent) :
    QwtPlot(parent), freeGraph(0)
{
    QwtPlotCanvas *canvas = new QwtPlotCanvas();
    canvas->setFocusIndicator( QwtPlotCanvas::CanvasFocusIndicator );
    canvas->setFocusPolicy( Qt::StrongFocus );
    canvas->setPalette(Qt::white);
    canvas->setFrameStyle(QFrame::StyledPanel);
    setCanvas(canvas);

    setAutoReplot(true);

    //setTitle("Legend Test");
    //setFooter("Footer");

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

    QwtLegend *leg = new QwtLegend();
    leg->setDefaultItemMode(QwtLegendData::Clickable);
    connect(leg, SIGNAL(clicked(QVariant,int)),this,SLOT(editLegendItem(QVariant,int)));
    insertLegend(leg, QwtPlot::RightLegend);


    //zoom = new QwtPlotZoomer(this->canvas());
//setAlignCanvasToScales( true );

    zoom = new QwtChartZoom(this);
    wheelZoom = new QWheelZoomSvc();
    wheelZoom->attach(zoom);

    axisZoom = new QAxisZoomSvc();
    axisZoom->attach(zoom);

    picker = new QwtPlotPicker(this->canvas());

    picker->setStateMachine(new QwtPickerTrackerMachine);
    picker->setTrackerMode(QwtPicker::AlwaysOn);
    picker->setRubberBand(QwtPicker::CrossRubberBand );
    picker->setRubberBandPen(QPen(QColor(60,60,60), 0.5, Qt::DashLine));
    bool pickerEnabled = MainWindow::getSetting("pickerEnabled", true).toBool();
    picker->setEnabled(pickerEnabled);
}

Plot::~Plot()
{
    delete freeGraph;
    qDeleteAll(graphs);
    delete grid;
    delete axisZoom;
    delete wheelZoom;
    delete zoom;
}

bool Plot::hasGraphs() const
{
    QwtPlotItemList curveList = itemList(QwtPlotItem::Rtti_PlotCurve);
    return !curveList.isEmpty();
}

int Plot::totalGraphsCount() const
{
    QwtPlotItemList curveList = itemList(QwtPlotItem::Rtti_PlotCurve);
    return curveList.size();
}

bool Plot::hasFreeGraph() const
{
    return (freeGraph != 0);
}

void Plot::deleteGraphs()
{
    delete freeGraph;
    freeGraph = 0;

    qDeleteAll(graphs);
    graphs.clear();
    leftGraphs.clear();
    rightGraphs.clear();
    x1.clear();
    y1.clear();
    y2.clear();
    usedColors.clear();

    for (int i=0; i<4; ++i)
        setAxisTitle(i, "");

    updateAxes();
    updateLegend();
    replot();
}

void Plot::deleteGraphs(const QString &dfdGuid)
{
    qDebug()<<dfdGuid;
    for (int i = graphs.size()-1; i>=0; --i) {
        Curve *graph = graphs[i];
        qDebug()<<graph->dfd->DFDGUID;
        if (dfdGuid == graph->dfd->DFDGUID) {qDebug()<<"is";
            deleteGraph(graph, true);
        }
    }
}

void Plot::deleteGraph(DfdFileDescriptor *dfd, int channel, bool doReplot)
{
    if (Curve *graph = plotted(dfd, channel)) {
        deleteGraph(graph, doReplot);
    }
}

void Plot::deleteGraph(Curve *graph, bool doReplot)
{
    if (graph) {
        usedColors.removeAll(graph->pen().color());
        graphs.removeAll(graph);
        leftGraphs.removeAll(graph);
        rightGraphs.removeAll(graph);
        delete graph;
        graph = 0;

        if (leftGraphs.isEmpty()) {
            yLeftName.clear();
            setAxisTitle(QwtPlot::yLeft, yLeftName);
        }
        if (rightGraphs.isEmpty()) {
            yRightName.clear();
            setAxisTitle(QwtPlot::yRight, yRightName);
            enableAxis(QwtPlot::yRight, false);
        }
        if (!hasGraphs()) {
            xName.clear();
            setAxisTitle(QwtPlot::xBottom, xName);
            x1.clear();
            y1.clear();
            y2.clear();
        }

        if (doReplot) {
            updateAxes();
            updateLegend();
            replot();
        }
    }
}

bool Plot::plotChannel(DfdFileDescriptor *dfd, int channel, bool addToFixed)
{
    if (plotted(dfd, channel)) return false;

    bool plotOnFirstYAxis = false;
    bool plotOnSecondYAxis = false;
    bool rewriteXAxis = false;

    Channel *ch = dfd->channels[channel];

    // нет ни одного графика
    if (!hasGraphs()) {
        plotOnFirstYAxis = true;
        rewriteXAxis = true;
    }
    // есть один временный график
    else if (graphsCount()==0 && !addToFixed) {//строим временный график - всегда на левой оси
        plotOnFirstYAxis = true;
        rewriteXAxis = true;
    }
    // есть постоянные графики
    else {
        /** TODO: заменить на анализ типа графика */
        if (ch->parent->XName == xName || xName.isEmpty()) { // тип графика совпадает
            if (leftGraphs.isEmpty() || yLeftName.isEmpty() || ch->YName == yLeftName)
                plotOnFirstYAxis = true;
            else if (rightGraphs.isEmpty() || yRightName.isEmpty() || ch->YName == yRightName)
                plotOnSecondYAxis = true;
        }
    }

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

    if (rewriteXAxis)
        setAxisTitle(QwtPlot::xBottom, dfd->XName);

    QwtPlot::Axis ax = QwtPlot::yLeft;
    if (plotOnSecondYAxis) ax = QwtPlot::yRight;
    if (!axisEnabled(ax)) enableAxis(ax);
    setAxisTitle(ax, ch->YName);

    Curve *g = 0;

    if (addToFixed) {
        Curve *graph = new Curve(ch->legendName(), dfd, channel);
        QColor nextColor = getColor();
        graph->setPen(nextColor, 1);

        graphs << graph;
        g = graph;
    }
    else {
        deleteGraph(freeGraph, false);
        freeGraph = new Curve(ch->legendName(), 0, -1);
        QColor nextColor = Qt::black;
        freeGraph->setPen(nextColor, 2);

        g = freeGraph;
    }
    g->legend = ch->legendName();
    g->setLegendIconSize(QSize(16,8));
    g->setRawSamples(ch->parent->XBegin, ch->xStep, ch->yValues, ch->NumInd);
    g->attach(this);

    g->setYAxis(ax);
    if (plotOnSecondYAxis) {
        rightGraphs << g;
        yRightName = ch->YName;
    }
    else {
        leftGraphs << g;
        yLeftName = ch->YName;
    }
    xName = dfd->XName;

    x1.min = qMin(ch->xMin, x1.min);
    x1.max = qMax(ch->xMaxInitial, x1.max);
    setAxisScale(QwtPlot::xBottom, x1.min, x1.max);

    Range &r = y1;
    if (plotOnSecondYAxis) r = y2;
    r.min = qMin(ch->yMinInitial, r.min);
    r.max = qMax(ch->yMaxInitial, r.max);
    setAxisScale(ax, r.min, r.max);


    updateAxes();
    updateLegend();
    replot();
    return true;
}

Curve * Plot::plotted(DfdFileDescriptor *dfd, int channel) const
{
    foreach (Curve *graph, graphs) {
        if (graph->dfd == dfd && graph->channel == channel) return graph;
    }
    return 0;
}

void Plot::updateLegends()
{
    foreach (Curve *graph, leftGraphs) {
        graph->legend = graph->dfd->channels.at(graph->channel)->legendName();
        graph->setTitle(graph->legend);
    }
    foreach (Curve *graph, rightGraphs) {
        graph->legend = graph->dfd->channels.at(graph->channel)->legendName();
        graph->setTitle(graph->legend);
    }
    updateLegend();
}

void Plot::savePlot()
{
    QwtPlotRenderer renderer;
    renderer.setDiscardFlag(QwtPlotRenderer::DiscardBackground);
    renderer.setLayoutFlag(QwtPlotRenderer::FrameWithScales);

    QString lastPicture = MainWindow::getSetting("lastPicture", "plot.bmp").toString();
    lastPicture = QFileDialog::getSaveFileName(this, QString("Сохранение графика"), lastPicture, "Изображения (*.bmp)");
    if (lastPicture.isEmpty()) return;

    QFont axisfont = axisFont(QwtPlot::yLeft);
    axisfont.setPointSize(axisfont.pointSize()+1);

    for (int i=0; i<QwtPlot::axisCnt; ++i)
        if (axisEnabled(i)) setAxisFont(i, axisfont);

    foreach (Curve *graph, leftGraphs) {
        QPen pen = graph->pen();
        pen.setWidth(2);
        graph->setPen(pen);
        graph->setTitle(QwtText("<font size=5>"+graph->legend+"</font>"));
    }
    foreach (Curve *graph, rightGraphs) {
        QPen pen = graph->pen();
        pen.setWidth(2);
        graph->setPen(pen);
        graph->setTitle(QwtText("<font size=5>"+graph->legend+"</font>"));
    }
    QwtLegend *leg = new QwtLegend();
    insertLegend(leg, QwtPlot::BottomLegend);

//    QwtPlotItemList list = this->itemList(/*QwtPlotItem::Rtti_PlotTextLabel*/);




    renderer.renderDocument(this, lastPicture, QSizeF(400,200), qApp->desktop()->logicalDpiX());


    axisfont.setPointSize(axisfont.pointSize()-1);
    for (int i=0; i<QwtPlot::axisCnt; ++i)
        if (axisEnabled(i)) setAxisFont(i, axisfont);

    foreach (Curve *graph, leftGraphs) {
        QPen pen = graph->pen();
        pen.setWidth(1);
        graph->setPen(pen);
        graph->setTitle(QwtText(graph->legend));
    }
    foreach (Curve *graph, rightGraphs) {
        QPen pen = graph->pen();
        pen.setWidth(1);
        graph->setPen(pen);
        graph->setTitle(QwtText(graph->legend));
    }
    leg = new QwtLegend();
    leg->setDefaultItemMode(QwtLegendData::Clickable);
    connect(leg, SIGNAL(clicked(QVariant,int)),this,SLOT(editLegendItem(QVariant,int)));
    insertLegend(leg, QwtPlot::RightLegend);

    MainWindow::setSetting("lastPicture", lastPicture);
}

void Plot::switchCursor()
{
    bool pickerEnabled = picker->isEnabled();
    picker->setEnabled(!pickerEnabled);
    MainWindow::setSetting("pickerEnabled", !pickerEnabled);
}

void Plot::editLegendItem(const QVariant &itemInfo, int index)
{
    QwtPlotItem *item = infoToItem(itemInfo);
    if (item) {
        QwtPlotCurve *c = dynamic_cast<QwtPlotCurve *>(item);
        if (c) {
            GraphPropertiesDialog dialog(c, this);
            dialog.exec();
        }
    }
}
