#include "plot.h"

#include <qwt_plot_canvas.h>
#include <qwt_legend.h>
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

#include "canvaspicker.h"
#include "plotpicker.h"

Plot::Plot(QWidget *parent) :
    QwtPlot(parent), freeGraph(0)
{
    canvas = new QwtPlotCanvas();
    canvas->setFocusIndicator( QwtPlotCanvas::CanvasFocusIndicator);
    canvas->setFocusPolicy( Qt::StrongFocus);
    canvas->setPalette(Qt::white);
    canvas->setFrameStyle(QFrame::StyledPanel);
    setCanvas(canvas);

    setAutoReplot(true);

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

    QwtLegend *leg = new QwtLegend();
    leg->setDefaultItemMode(QwtLegendData::Clickable);
    connect(leg, SIGNAL(clicked(QVariant,int)),this,SLOT(editLegendItem(QVariant,int)));
    insertLegend(leg, QwtPlot::RightLegend);


    zoom = new QwtChartZoom(this);
    zoom->setEnabled(true);

    picker = new PlotPicker(canvas);

    bool pickerEnabled = MainWindow::getSetting("pickerEnabled", true).toBool();
    picker->setEnabled(pickerEnabled);

   // connect(picker, SIGNAL(moved(QPointF)), this, SLOT(pointSelected(QPointF)));
  //  picker = 0;

  //  canvasPicker = new CanvasPicker(this);

  //  canvasPicker->setEnabled(false);
    canvasPicker = 0;
}

void Plot::pointSelected(const QPointF &pos)
{
    qDebug()<<pos<<"selected";
}

Plot::~Plot()
{
    delete freeGraph;
    qDeleteAll(graphs);
    delete grid;
    delete zoom;
    delete picker;
    delete canvasPicker;
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
    ColorSelector::instance()->resetState();

    for (int i=0; i<4; ++i)
        setAxisTitle(i, "");

    delete zoom;
    zoom = new QwtChartZoom(this);
    //zoom->resetBounds();

    updateAxes();
    updateLegend();
    replot();
}

void Plot::deleteGraphs(const QString &dfdGuid)
{
    for (int i = graphs.size()-1; i>=0; --i) {
        Curve *graph = graphs[i];
        if (dfdGuid == graph->dfd->DFDGUID) {
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
        ColorSelector::instance()->freeColor(graph->pen().color());
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

bool Plot::canBePlottedOnLeftAxis(Channel *ch, bool addToFixed)
{
    if (!hasGraphs()) // нет графиков - всегда на левой оси
        return true;

    if (graphsCount()==0 && !addToFixed) //строим временный график - всегда на левой оси
        return true;

    if (ch->parent->XName == xName || xName.isEmpty()) { // тип графика совпадает
        if (leftGraphs.isEmpty() || yLeftName.isEmpty() || ch->YName == yLeftName)
            return true;
    }
    return false;
}

bool Plot::canBePlottedOnRightAxis(Channel *ch, bool addToFixed)
{
    if (!hasGraphs()) // нет графиков - всегда на левой оси
        return true;

    if (graphsCount()==0 && !addToFixed) //строим временный график - всегда на левой оси
        return true;

    if (ch->parent->XName == xName || xName.isEmpty()) { // тип графика совпадает
        if (rightGraphs.isEmpty() || yRightName.isEmpty() || ch->YName == yRightName)
            return true;
    }
    return false;
}

void Plot::prepareAxis(int axis, const QString &name)
{
    if (!axisEnabled(axis))
        enableAxis(axis);

    setAxisTitle(axis, name);
}

void Plot::moveGraph(Curve *curve)
{
    if (leftGraphs.contains(curve)) {
        leftGraphs.removeAll(curve);
        rightGraphs.append(curve);
    }
    else if (rightGraphs.contains(curve)) {
        rightGraphs.removeAll(curve);
        leftGraphs.append(curve);
    }
}

bool Plot::plotChannel(DfdFileDescriptor *dfd, int channel, bool addToFixed, QColor *col)
{
    if (plotted(dfd, channel)) return false;

    Channel *ch = dfd->channels[channel];

    const bool plotOnFirstYAxis = canBePlottedOnLeftAxis(ch, addToFixed);
    const bool plotOnSecondYAxis = plotOnFirstYAxis ? false : canBePlottedOnRightAxis(ch, addToFixed);

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

    prepareAxis(QwtPlot::xBottom, dfd->XName);

    QwtPlot::Axis ax = plotOnFirstYAxis ? QwtPlot::yLeft : QwtPlot::yRight;

    prepareAxis(ax, ch->YName);

    Curve *g = 0;

    if (addToFixed) {
        Curve *graph = new Curve(ch->legendName(), dfd, channel);
        QColor nextColor = ColorSelector::instance()->getColor();
        graph->setPen(nextColor, 1);
        if (col) *col = nextColor;

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
    QString lastPicture = MainWindow::getSetting("lastPicture", "plot.bmp").toString();
    lastPicture = QFileDialog::getSaveFileName(this, QString("Сохранение графика"), lastPicture, "Изображения (*.bmp)");
    if (lastPicture.isEmpty()) return;

    importPlot(lastPicture);

    MainWindow::setSetting("lastPicture", lastPicture);
}

void Plot::copyToClipboard()
{
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
{
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

void Plot::calculateMean()
{
    bool dataTypeEqual = true;
    bool stepsEqual = true; // одинаковый шаг по оси Х
    bool namesEqual = true; // одинаковые названия осей Y
    bool oldNamesEqual = true; // одинаковые старые названия осей Y
    bool oneFile = true; // каналы из одного файла
    bool writeToSeparateFile = true;

    if (graphs.size()<2) return;

    QList<Channel*> channels;

    Curve *firstCurve = graphs.first();
    Channel *firstChannel = firstCurve->dfd->channels.at(firstCurve->channel);
    channels << firstChannel;
    for (int i = 1; i<graphs.size(); ++i) {
        Curve *curve = graphs.at(i);
        Channel *channel = curve->dfd->channels.at(curve->channel);
        channels << channel;

        if (firstChannel->xStep != channel->xStep)
            stepsEqual = false;
        if (firstChannel->YName != channel->YName)
            namesEqual = false;
        if (firstChannel->YNameOld != channel->YNameOld)
            oldNamesEqual = false;
        if (firstCurve->dfd->DFDGUID != curve->dfd->DFDGUID)
            oneFile = false;
        if (firstCurve->dfd->DataType != curve->dfd->DataType)
            dataTypeEqual = false;
    }

    if (!dataTypeEqual) {
        int result = QMessageBox::warning(this, "Среднее графиков",
                                          "Графики имеют разный тип. Продолжить?",
                                          "Да", "Нет");

        if (result == 1)
            return;
    }

    if (!namesEqual) {
        int result = QMessageBox::warning(this, "Среднее графиков",
                                          "Графики по-видимому имеют разный тип. Продолжить?",
                                          "Да", "Нет");

        if (result == 1)
            return;
    }

    if (firstChannel->YName == "дБ" && !oldNamesEqual) {
        int result = QMessageBox::warning(this, "Среднее графиков",
                                          "Графики по-видимому имеют разный тип. Продолжить?",
                                          "Да", "Нет");
        if (result == 1)
            return;
    }

    if (!stepsEqual) {
        int result = QMessageBox::warning(this, "Среднее графиков",
                                          "Графики имеют разный шаг по оси X. Продолжить?",
                                          "Да", "Нет");
        if (result == 1)
            return;
    }
    if (oneFile) {
        int result = QMessageBox::information(this, "Среднее графиков",
                                              QString("Графики взяты из одной записи %1.\n").arg(firstCurve->dfd->rawFileName)
                                              +
                                              QString("Сохранить среднее в эту запись дополнительным каналом?"),
                                              "Да, записать в этот файл", "Нет, экспортировать в отдельный файл");
        writeToSeparateFile = (result == 1);
    }

    DfdFileDescriptor *meanDfd = 0;
    Channel *ch = 0;
    bool created = false;
    QString meanDfdFile;

    if (writeToSeparateFile) {
        // берем и копируем первый файл
        QString meanD = firstCurve->dfd->dfdFileName;
        meanD = meanD.left(meanD.length()-4);
        meanDfdFile = QFileDialog::getSaveFileName(this, "Сохранение среднего", meanD, "Файлы dfd (*.dfd)");
        if (meanDfdFile.isEmpty()) return;


        if (QFile::copy(firstCurve->dfd->dfdFileName, meanDfdFile)) {
            meanDfd = new DfdFileDescriptor(meanDfdFile);
            meanDfd->read();

            // удаляем все каналы, кроме того, который построен
            const int chan = firstCurve->channel;
            for (int i = meanDfd->channels.size()-1; i>chan; --i) {
                delete meanDfd->channels[i];
                meanDfd->channels.removeAt(i);
            }
            for (int i = chan-1; i>=0; --i) {
                delete meanDfd->channels[i];
                meanDfd->channels.removeAt(i);
            }

            Q_ASSERT(meanDfd->channels.size()==1);

            ch = meanDfd->channels[0];
            ch->channelIndex = 0;
        }
    }
    else {
        meanDfd = firstCurve->dfd;
        ch = meanDfd->getChannel(meanDfd->DataType, meanDfd->channels.size());
        meanDfd->channels.append(ch);
        meanDfdFile = meanDfd->dfdFileName;
    }

    if (meanDfd && ch) {
        // считаем данные для этого канала
        // если ось = дБ, сначала переводим значение в линейное
        quint32 numInd = channels.first()->NumInd;
        for (int i=1; i<channels.size(); ++i) {
            if (channels.at(i)->NumInd < numInd)
                numInd = channels.at(i)->NumInd;
        }

        delete [] ch->yValues;
        ch->yValues = new double[numInd];

        ch->yMin = 1.0e100;
        ch->yMax = -1.0e100;

        for (quint32 i=0; i<numInd; ++i) {
            double sum = 0.0;
            for (int file = 0; file < channels.size(); ++file) {
                double temp = channels[file]->yValues[i];
                if (channels[file]->YName == "дБ")
                    temp = pow(10.0, (temp/10.0));
                sum += temp;
            }
            sum /= channels.size();
            if (channels[0]->YName == "дБ")
                sum = 10.0 * log10(sum);
            if (sum < ch->yMin) ch->yMin = sum;
            if (sum > ch->yMax) ch->yMax = sum;
            ch->yValues[i] = sum;
        }

        // обновляем сведения канала
        ch->populated = true;
        ch->ChanName = "Среднее";

        QStringList list;
        foreach(Channel *c, channels)
            list << QString::number(c->channelIndex+1);
        ch->ChanDscr = "Среднее каналов "+list.join(",");

        ch->ChanBlockSize = numInd;
        ch->NumInd = numInd;
        ch->IndType = firstChannel->IndType;
        ch->InputType = firstChannel->InputType;
        ch->YName = firstChannel->YName;
        ch->YNameOld = firstChannel->YNameOld;
        ch->blockSizeInBytes = ch->ChanBlockSize * (ch->IndType % 16);
        ch->xStep = firstChannel->xStep;

        ch->xMin = ch->parent->XBegin;
        ch->xMax = ch->xMin + ch->xStep * numInd;
        ch->xMaxInitial = ch->xMax;
        ch->yMinInitial = ch->yMin;
        ch->yMaxInitial = ch->yMax;

        // обновляем dfd-файл
        meanDfd->changed = true;
        meanDfd->rawFileChanged = true;

        meanDfd->Time = QTime::currentTime();
        meanDfd->Date = QDate::currentDate();
        meanDfd->CreatedBy = "DeepSeaBase by Novichkov & sukin sons";
        meanDfd->DFDGUID = DfdFileDescriptor::createGUID();
        meanDfd->NumChans = meanDfd->channels.size();

        // будем писать канал подряд, блок = размеру файла
        meanDfd->BlockSize = 0;
        meanDfd->NumInd = ch->NumInd;
    }

    if (writeToSeparateFile)
        // implicit saving
        delete meanDfd;
    else {
        meanDfd->write();
        meanDfd->writeRawFile();
    }

    if (created)
        emit fileCreated(meanDfdFile, true);
    else
        emit fileChanged(meanDfdFile, true);
}

void Plot::switchInteractionMode()
{
    if (interactionMode == ScalingInteraction) {
        setInteractionMode(DataInteraction);
    }
    else {
        setInteractionMode(ScalingInteraction);
    }
}

void Plot::setInteractionMode(Plot::InteractionMode mode)
{
    interactionMode = mode;
    if (picker) picker->setMode(mode);
    if (zoom) zoom->setEnabled(mode == ScalingInteraction);
    if (canvas) canvas->setFocusIndicator(mode == ScalingInteraction?
                                              QwtPlotCanvas::CanvasFocusIndicator:
                                              QwtPlotCanvas::ItemFocusIndicator);
}

void Plot::importPlot(const QString &fileName)
{
    QwtPlotRenderer renderer;
    renderer.setDiscardFlag(QwtPlotRenderer::DiscardBackground);
    renderer.setLayoutFlag(QwtPlotRenderer::FrameWithScales);

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


    renderer.renderDocument(this, fileName, QSizeF(400,200), qApp->desktop()->logicalDpiX());


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
}

void Plot::switchCursor()
{
    if (!picker) return;

    bool pickerEnabled = picker->isEnabled();
    picker->setEnabled(!pickerEnabled);
    MainWindow::setSetting("pickerEnabled", !pickerEnabled);
}

void Plot::editLegendItem(const QVariant &itemInfo, int index)
{
    Q_UNUSED(index)

    QwtPlotItem *item = infoToItem(itemInfo);
    if (item) {
        Curve *c = dynamic_cast<Curve *>(item);
        if (c) {
            GraphPropertiesDialog dialog(c, this);
            dialog.exec();
        }
    }
}
