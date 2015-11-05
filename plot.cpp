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

#include <qwt_plot_layout.h>

#include <QtCore>
#include <QMessageBox>
#include <QFileDialog>
#include <QDesktopWidget>
#include <QApplication>
#include <QClipboard>
#include <QPrinter>
#include <QPrintDialog>

#include "mainwindow.h"
#include "graphpropertiesdialog.h"

#include "colorselector.h"
#include "legend.h"

#include "plotpicker.h"
#include <QAction>

#include "logging.h"

Plot::Plot(QWidget *parent) :
    QwtPlot(parent)/*, freeGraph(0)*/
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

//    minStep = 0.0;

    axisLabelsVisible = MainWindow::getSetting("axisLabelsVisible", true).toBool();

    _showTrackingCursor = false;

    _trackingCursor = new QwtPlotMarker();
    _trackingCursor->setLineStyle( QwtPlotMarker::VLine );
    _trackingCursor->setLinePen( Qt::black, 0, Qt::DashDotLine );

    //setTitle("Legend Test");
    //setFooter("Footer");

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

    Legend *leg = new Legend();
    //leg->setDefaultItemMode(QwtLegendData::Clickable);
    connect(leg, SIGNAL(clicked(QVariant,int)),this,SLOT(editLegendItem(QVariant,int)));
    connect(leg, SIGNAL(markedForDelete(QVariant,int)),this, SLOT(deleteGraph(QVariant,int)));
    insertLegend(leg, QwtPlot::RightLegend);


    zoom = new QwtChartZoom(this);
    zoom->setEnabled(true);

    picker = new PlotPicker(canvas);
    connect(picker,SIGNAL(labelSelected(bool)),zoom,SLOT(labelSelected(bool)));

    bool pickerEnabled = MainWindow::getSetting("pickerEnabled", true).toBool();
    picker->setEnabled(pickerEnabled);
    connect(zoom,SIGNAL(updateTrackingCursor(double)),SLOT(updateTrackingCursor(double)));
    connect(picker,SIGNAL(updateTrackingCursor(double)),SLOT(updateTrackingCursor(double)));
}

Plot::~Plot()
{DD;
//    delete freeGraph;
    trackingPanel->hide();
    qDeleteAll(graphs);
    delete grid;
    delete zoom;
    delete picker;
    delete _trackingCursor;
    MainWindow::setSetting("axisLabelsVisible", axisLabelsVisible);
}

void Plot::update()
{
    updateAxes();
    updateAxesLabels();
    updateLegend();
    double x = _trackingCursor->value().x();
    updateTrackingCursor(x);
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
//    delete freeGraph;
//    freeGraph = 0;

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

    delete zoom;
    zoom = new QwtChartZoom(this);
    //zoom->resetBounds();

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

void Plot::updateTrackingCursor(double xVal)
{DD;
    if (_showTrackingCursor) {
        if (graphs.isEmpty()) {
            _trackingCursor->setValue(xVal, 0.0);
            _trackingCursor->attach(this);
            trackingPanel->setX(xVal);
            return;
        }

        //first we need to find the closest point to xVal;
        //1. search the curve with the minimum xstep;
        double xstep = graphs.first()->channel->xStep();
        for (int i=1; i<graphs.size(); ++i) {
            if (graphs.at(i)->channel->xStep()<xstep)
                xstep = graphs.at(i)->channel->xStep();
        }
        if (xstep==0.0) xstep = graphs.first()->channel->xStep();
        if (xstep==0.0) return;

        //2. compute the actual xVal based on the xVal and xstep
        int steps = int(xVal/xstep);
//        qDebug()<<"xstep"<<xstep<<"xval"<<xVal<<"steps"<<steps;
        if (steps <= 0) xVal = 0.0;
        else {
            if (qAbs(xstep*(steps+1)-xVal) < qAbs(xstep*steps-xVal)) steps++;
            xVal = xstep*steps;
        }


        //3. get the y values from all graphs
        QList<TrackingPanel::TrackInfo> list;
        foreach(Curve *c,graphs) {
            steps = int(xVal/c->channel->xStep());
            if (steps < 0) steps=0;
            if (qAbs(c->channel->xStep()*(steps+1)-xVal) < qAbs(c->channel->xStep()*steps-xVal)) steps++;
            if (steps>c->channel->samplesCount()) steps=c->channel->samplesCount();

            TrackingPanel::TrackInfo ti{c->channel->name(), c->channel->color(), c->sample(steps).y(),
                        c->sample(steps).x()};
            list << ti;
        }
        trackingPanel->setX(xVal);
        trackingPanel->updateState(list);

        _trackingCursor->setValue(xVal, 0.0);
        _trackingCursor->attach(this);


    }
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
{
    switch (axis) {
        case QwtPlot::yLeft: yLeftName = name; break;
        case QwtPlot::yRight: yRightName = name; break;
        case QwtPlot::xBottom: xName = name; break;
        default: break;
    }
}

void Plot::updateAxesLabels()
{
    if (leftGraphs.isEmpty()) enableAxis(QwtPlot::yLeft, false);
    else {
        enableAxis(QwtPlot::yLeft, axisLabelsVisible);
        setAxisTitle(QwtPlot::yLeft, axisLabelsVisible ? yLeftName : "");
    }
//    enableAxis(QwtPlot::yLeft, !leftGraphs.isEmpty());
//    QwtText t = axisTitle(QwtPlot::yLeft);
//    t.setText(yLeftName);
//    t.setColor(axisLabelsVisible?Qt::black:Qt::white);
//    setAxisTitle(QwtPlot::yLeft, t);

//    QPalette pal=palette();
//    QColor col=pal.color(QPalette::Button);
//    pal.setColor(QPalette::Text, col);
//    setPalette(pal);

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

    if (!ch->populated()) ch->populate();

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
        zoom->fixBoundaries();

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

//void Plot::onCurveChanged(Curve *curve)
//{DD;
//    emit curveChanged(curve);

//}

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
{
    picker->showHarmonics(!picker->harmonics());
}

void Plot::switchTrackingCursor()
{DD;
//    qDebug()<<"Tracking cursor enabled";
    _showTrackingCursor = !_showTrackingCursor;
    if (!_showTrackingCursor) _trackingCursor->detach();
    trackingPanel->setVisible(_showTrackingCursor);
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


#include <QtWidgets>

TrackingPanel::TrackingPanel(QWidget *parent) : QWidget(parent)
{
    setWindowFlags(Qt::Tool | Qt::WindowTitleHint);

    tree = new QTreeWidget(this);
    tree->setColumnCount(4);
    tree->setAlternatingRowColors(true);
    tree->setHeaderLabels(QStringList()<<""<<"Название"<<"X"<<"Y");
    tree->setRootIsDecorated(false);
    tree->setColumnWidth(0,50);
    tree->setColumnWidth(1,150);

    button = new QPushButton("Копировать", this);
    connect(button,&QPushButton::clicked,[=](){
        QStringList list;
        for(int i=0; i<tree->topLevelItemCount(); ++i) {
            QString val = tree->topLevelItem(i)->text(3);
            val.replace(".",",");
            if (box->isChecked())
                list << QString("%1\t%2").arg(tree->topLevelItem(i)->text(1)).arg(val);
            else
                list << val;
        }
        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(list.join("\n"));
    });

    box = new QCheckBox("Включая названия каналов", this);


    QVBoxLayout *l = new QVBoxLayout;
    QHBoxLayout *ll = new QHBoxLayout;
    ll->addWidget(button);
    ll->addWidget(box);
    l->addWidget(tree);
    l->addLayout(ll);
    setLayout(l);
    resize(350,300);
}

void TrackingPanel::updateState(const QList<TrackingPanel::TrackInfo> &curves)
{
    for(int i=0; i<curves.size(); ++i) {
        QTreeWidgetItem *item = tree->topLevelItem(i);
        if (!item) {item = new QTreeWidgetItem();
            item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemNeverHasChildren);
            tree->addTopLevelItem(item);
        }
        item->setBackgroundColor(0,curves[i].color);
        item->setText(1,curves[i].name);
        item->setText(2, QString::number(curves[i].xval));
        item->setText(3, QString::number(curves[i].yval));
    }
    for (int i=tree->topLevelItemCount()-1; i>=curves.size(); --i)
        delete tree->takeTopLevelItem(i);
    tree->resizeColumnToContents(0);
    tree->resizeColumnToContents(2);
    tree->resizeColumnToContents(3);
}

void TrackingPanel::setX(double x)
{
    tree->headerItem()->setText(2, QString::number(x));
}
