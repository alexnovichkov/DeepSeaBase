#include "qcpplot.h"

#include <QAction>
#include <QAudioOutput>
#include <QClipboard>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QMenu>
#include <QMessageBox>
#include <QPrintDialog>
#include <QPrinter>
#include <QtCore>

#include "axisboundsdialog.h"
#include "canvaseventfilter.h"
#include "channelsmimedata.h"
#include "checkablelegend.h"
#include "colormapfactory.h"
#include "colorselector.h"
#include "cursorbox.h"
#include "cursors.h"
#include "curve.h"
#include "curvepropertiesdialog.h"
#include "enums.h"
#include "fileformats/filedescriptor.h"
#include "graph2d.h"
#include "imagerenderdialog.h"
#include "logging.h"
#include "mainwindow.h"
#include "mousecoordinates.h"
#include "picker.h"
#include "playpanel.h"
#include "plotmodel.h"
#include "plottedmodel.h"
#include "qcpaxisoverlay.h"
#include "qcpaxistickeroctave.h"
#include "qcpflowlegend.h"
#include "qcpinfooverlay.h"
#include "qcpspectrogram.h"
#include "secondaryplot.h"
#include "settings.h"
#include "unitsconverter.h"
#include "zoomstack.h"

QCPAxis::AxisType toQcpAxis(Enums::AxisType type) {
    return static_cast<QCPAxis::AxisType>(type);
}

Enums::AxisType fromQcpAxis(QCPAxis::AxisType type) {
    return static_cast<Enums::AxisType>(type);
}

QCPPlot::QCPPlot(Enums::PlotType type, QWidget *parent) : QCustomPlot(parent), plotType(type)
{
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);

    m = new PlotModel(this);

    picker = new Picker(this);
    picker->setPickPriority(Picker::PickPriority::PickLast);
    picker->setEnabled(true);

    zoom = new ZoomStack(this);
    connect(zoom, &ZoomStack::replotNeeded, this, &QCPPlot::replot);

    linTicker.reset(new QCPAxisTicker);
//    linTicker->setTickStepStrategy(QCPAxisTicker::tssFixed);
//    linTicker->setTickStep(400);
//    linTicker->setSubTickStep(60);
//    linTicker->setScaleStrategy(QCPAxisTicker::ssNone);

    logTicker.reset(new QCPAxisTickerLog);
    logTicker->setLogBase(2);
    octaveTicker.reset(new QCPAxisTickerOctave);

    leftOverlay = new QCPLeftAxisOverlay(this);
    rightOverlay = new QCPRightAxisOverlay(this);

    oldCursor = cursor();

    infoOverlay = new QCPInfoOverlay(this);
    infoOverlay->setVisible(true);

    addLayer("mouse");
    for (int i=0; i<layerCount(); ++i) layer(i)->setMode(QCPLayer::lmBuffered);

    mouseCoordinates = new MouseCoordinates(this);
    mouseCoordinates->setLayer("mouse");

    setNotAntialiasedElement(QCP::aeScatters , true);
    setNoAntialiasingOnDrag(true);

    setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    setSelectionRectMode(QCP::srmZoom);
    setMouseTracking(true);
    setPlottingHints( QCP::phCacheLabels | /*QCP::phFastPolylines |*/ QCP::phImmediateRefresh);

    axisRect()->setRangeDragAxes({xAxis, yAxis, yAxis2});
    axisRect()->setRangeDrag(Qt::Horizontal | Qt::Vertical);
    axisRect()->setRangeZoomAxes({xAxis, yAxis, yAxis2});

    //wheel event on axisRect
    connect(axisRect(), &QCPAxisRect::axesRangeScaled, this, &QCPPlot::addZoom);
    //dragging on axisRect
    connect(axisRect(), &QCPAxisRect::draggingFinished, this, &QCPPlot::addZoom);

    connect(this, &QCustomPlot::mouseMove, mouseCoordinates, &MouseCoordinates::update);
    for (auto axis: axisRect()->axes()) {
        axis->setSubTicks(true);
        axis->setNumberPrecision(10);
//        axis->setScaleType(QCPAxis::stLinear);
//        axis->setTicker(linTicker);

        connect(axis, &QCPAxis::draggingFinished, [=](const QCPRange &newRange){
            ZoomStack::zoomCoordinates coords;
            for (auto ax: axisRect()->axes()) {
                if (axis == ax) coords.insert(fromQcpAxis(axis->axisType()), newRange);
                else coords.insert(fromQcpAxis(ax->axisType()), ax->range());
            }
            if (colorScale) {
                auto ax = colorScale->axis();
                coords.insert(Enums::AxisType::atColor, ax->range());
            }

            zoom->addZoom(coords, true);
        });
        connect(axis, &QCPAxis::rangeScaled, this, &QCPPlot::addZoom);
    }

    setEventFilter(new CanvasEventFilter(this));

    if (plotType == Enums::PlotType::Spectrogram) {
        subLayout = new QCPLayoutGrid;
        plotLayout()->addElement(0, 2, subLayout);

        spectrePlot = new SpectrePlot(this, "Спектр", subLayout);
        throughPlot = new ThroughPlot(this, "Проходная", subLayout);
        plotLayout()->setColumnStretchFactors({4,0.1,2});
    }

    colors.reset(new ColorSelector());

    axisLabelsVisible = se->getSetting("axisLabelsVisible", true).toBool();
    yValuesPresentationLeft = DataHolder::ShowAsDefault;
    yValuesPresentationRight = DataHolder::ShowAsDefault;

    createLegend();

    cursors = new Cursors(this);
    cursorBox = new CursorBox(cursors, this);
    cursorBox->setWindowTitle(parent->windowTitle());
    cursorBox->setVisible(false);
    connect(se->instance(), &Settings::settingChanged, cursorBox, &CursorBox::changeFont);

    if (type == Enums::PlotType::Octave) {
        setAxisScale(Enums::AxisType::atBottom, Enums::AxisScale::ThirdOctave);
    }

    connect(picker, &Picker::removeNeeded, this, &QCPPlot::removeCursor);
    connect(this, &QCPPlot::curvesCountChanged, cursors, &Cursors::update);
    connect(cursors, &Cursors::cursorPositionChanged, this, qOverload<>(&QCPPlot::updateSecondaryPlots));
    connect(this, &QCPPlot::curvesCountChanged, cursorBox, &CursorBox::updateLayout);
    connect(cursorBox,SIGNAL(closeRequested()), SIGNAL(trackingPanelCloseRequested()));

    if (type == Enums::PlotType::Time) {
        playerPanel = new PlayPanel(this);
        connect(this, SIGNAL(curvesCountChanged()), playerPanel, SLOT(update()));
    }

    connect(se, &Settings::settingChanged, [this](const QString &key, const QVariant &val){
        Q_UNUSED(val);
        if (key == "plotOctaveAsHistogram") update();
    });
}

QCPPlot::~QCPPlot()
{
    deleteAllCurves(true);

    se->setSetting("axisLabelsVisible", axisLabelsVisible);
    se->setSetting("autoscale-x", !zoom->scaleBounds(Enums::AxisType::atBottom)->isFixed());
    se->setSetting("autoscale-y", !zoom->scaleBounds(Enums::AxisType::atLeft)->isFixed());
    se->setSetting("autoscale-y-slave", !zoom->scaleBounds(Enums::AxisType::atRight)->isFixed());
}

void QCPPlot::startZoom(QMouseEvent*event)
{
    mSelectionRect->setVisible(true);
    mSelectionRect->startSelection(event);
}

void QCPPlot::proceedZoom(QMouseEvent *event)
{
    if (mSelectionRect && mSelectionRect->isActive())
        mSelectionRect->moveSelection(event);
}

void QCPPlot::endZoom(QMouseEvent *event)
{
    if (mSelectionRect && mSelectionRect->isActive())
        mSelectionRect->endSelection(event);
}

void QCPPlot::cancelZoom()
{
    if (mSelectionRect && mSelectionRect->isActive()) {
        mSelectionRect->cancel();
        mSelectionRect->setVisible(false);
        replot();
    }
}

void QCPPlot::addCursorToSecondaryPlots(Cursor *cursor)
{
    if (spectrePlot) spectrePlot->addCursor(cursor);
    if (throughPlot) throughPlot->addCursor(cursor);
    replot();
}

void QCPPlot::removeCursorFromSecondaryPlots(Cursor *cursor)
{
    if (spectrePlot) spectrePlot->removeCursor(cursor);
    if (throughPlot) throughPlot->removeCursor(cursor);
//    replot();
}

void QCPPlot::updateSecondaryPlots()
{
    if (spectrePlot) spectrePlot->update();
    if (throughPlot) throughPlot->update();
}

void QCPPlot::setCurrentCurve(Curve *curve)
{
    if (spectrePlot) spectrePlot->setCurve(curve);
    if (throughPlot) throughPlot->setCurve(curve);
    if (spectrePlot || throughPlot) replot();
}

void QCPPlot::setEventFilter(CanvasEventFilter *filter)
{
    canvasFilter = filter;
    canvasFilter->setZoom(zoom);
    canvasFilter->setPicker(picker);

    connect(canvasFilter, &CanvasEventFilter::canvasDoubleClicked, this, &QCPPlot::canvasDoubleClicked);
    //    connect(canvasFilter, &CanvasEventFilter::hover, this, &QwtPlotImpl::hoverAxis);
    connect(canvasFilter, &CanvasEventFilter::contextMenuRequested, this, &QCPPlot::showContextMenu);
    connect(canvasFilter, &CanvasEventFilter::axisDoubleClicked, this, &QCPPlot::axisDoubleClicked);
    installEventFilter(filter);
}

void QCPPlot::axisDoubleClicked(QCPAxis *axis)
{
    auto type = fromQcpAxis(axis->axisType());
    AxisBoundsDialog dialog(axis, axisParameters.value(type));
    if (dialog.exec()) {
        if (axis->scaleType() == QCPAxis::stLinear) {
            auto ticker = new QCPAxisTicker();

            auto t = dialog.parameters();
            ticker->setScaleStrategy(t.tickStepAutomatic ? QCPAxisTicker::ssMultiples : QCPAxisTicker::ssNone);
            ticker->setTickStepStrategy(t.tickStepAutomatic ? QCPAxisTicker::tssReadability : QCPAxisTicker::tssFixed);
            ticker->setSubTickStepStrategy(t.subTickStepAutomatic ? QCPAxisTicker::tssReadability : QCPAxisTicker::tssFixed);
            ticker->setTickStep(t.tickStep);
            ticker->setSubTickStep(t.subTickStep);

            axis->setTicker(QSharedPointer<QCPAxisTicker>(ticker));
            axisParameters.insert(type, t);
        }

        ZoomStack::zoomCoordinates coords;
        auto axes = axisRect()->axes();
        if (axes.contains(axis)) { //5 axes types
            for (auto ax: axes) {
                if (axis == ax) coords.insert(fromQcpAxis(ax->axisType()), dialog.range());
                else coords.insert(fromQcpAxis(ax->axisType()), ax->range());
            }
            if (colorScale) {
                auto ax = colorScale->axis();
                coords.insert(Enums::AxisType::atColor, ax->range());
            }
        }
        else {//5 axes types
            for (auto ax: axes) {
                coords.insert(fromQcpAxis(ax->axisType()), ax->range());
            }
            coords.insert(Enums::AxisType::atColor, dialog.range());
        }

        zoom->addZoom(coords, true);
    }
}

QCPAxis *QCPPlot::eventTargetAxis(QEvent *event)
{
    if (auto mouseEvent = dynamic_cast<QMouseEvent*>(event)) {
        QList<QCPLayerable*> candidates = layerableListAt(mouseEvent->pos(), false);
        for (int i=0; i<candidates.size(); ++i) {
            if (auto ax = dynamic_cast<QCPAxis*>(candidates.at(i)))
                return ax;
        }
    }
    return nullptr;
}

void QCPPlot::createLegend()
{
    this->axisRect(0)->insetLayout()->setInsetAlignment(0, Qt::AlignBottom | Qt::AlignCenter);

    checkableLegend = new QCPCheckableLegend(this);

    connect(checkableLegend, &QCPCheckableLegend::markedForDelete, this, &QCPPlot::deleteCurveFromLegend);
    connect(checkableLegend, &QCPCheckableLegend::clicked,         this, &QCPPlot::editLegendItem);
    connect(checkableLegend, &QCPCheckableLegend::markedToMove, [=](Curve *c){
        moveCurve(c, c->yAxis() == Enums::AxisType::atLeft ? Enums::AxisType::atRight : Enums::AxisType::atLeft);
        replot();
    });
    connect(checkableLegend, &QCPCheckableLegend::fixedChanged, [=](Curve *c){
        c->switchFixed();
        checkableLegend->updateItem(c, c->commonLegendData());
    });
    connect(checkableLegend, &QCPCheckableLegend::visibilityChanged, [=](Curve *c, bool visible){
        c->setVisible(visible);
        if (auto p = dynamic_cast<QCPAbstractPlottable*>(c)) {
            p->setVisible(visible);
            if (visible) {
                p->addToLegend();
                //мы дожны автомасштабировать график, но только если это сонограмма
                if (dynamic_cast<QCPSpectrogram*>(c)) {
                    zoom->scaleBounds(Enums::AxisType::atBottom)->add(c->xMin(), c->xMax(), true);
                    zoom->scaleBounds(Enums::AxisType::atColor)->add(c->channel->data()->yMin(-1), c->channel->data()->yMax(-1), true);
                    zoom->scaleBounds(Enums::AxisType::atLeft)->add(c->channel->data()->zMin(), c->channel->data()->zMax(), true);
                }
                setCurrentCurve(c);
            }
            else p->removeFromLegend();
        }

//        updateSecondaryPlots({qQNaN(), qQNaN()});
        replot();
    });
}

double QCPPlot::screenToPlotCoordinates(Enums::AxisType axisType, double value) const
{
    return axis(axisType)->pixelToCoord(value);
}

double QCPPlot::plotToScreenCoordinates(Enums::AxisType axisType, double value) const
{
    return axis(axisType)->coordToPixel(value);
}

Range QCPPlot::plotRange(Enums::AxisType axisType) const
{
    auto r = axis(axisType)->range();
    return {r.lower, r.upper};
}

Range QCPPlot::screenRange(Enums::AxisType axisType) const
{
    auto r = axis(axisType)->range();
    return {screenToPlotCoordinates(axisType, r.lower), screenToPlotCoordinates(axisType, r.upper)};
}

void QCPPlot::replot()
{
    QCustomPlot::replot(rpQueuedReplot);
}

void QCPPlot::updateLegend()
{
    for (auto c: m->curves())
        if (auto p = dynamic_cast<QCPAbstractPlottable*>(c)) {
            if (!p->visible()) p->removeFromLegend();
            else p->addToLegend();
        }
}

QPoint QCPPlot::localCursorPosition(const QPoint &globalCursorPosition) const
{
    return mapFromGlobal(globalCursorPosition);
}

void QCPPlot::setAxisScale(Enums::AxisType axisType, Enums::AxisScale scale)
{
    auto a = axis(axisType);
    if (!a) return;

    const bool c = axisParameters.contains(axisType);
    auto &t = axisParameters[axisType];
    t.scale = scale;

    QList<QCPAxis*> axes;
    axes << a;
    if (spectrePlot) axes << spectrePlot->axis(axisType);

    for (auto &ax: axes) {
        switch (scale) {
            case Enums::AxisScale::Linear: {
                ax->setScaleType(QCPAxis::stLinear);
                auto ticker = new QCPAxisTicker();
                if (c) {
                    ticker->setTickStepStrategy(t.tickStepAutomatic ? QCPAxisTicker::tssReadability : QCPAxisTicker::tssFixed);
                    ticker->setSubTickStepStrategy(t.subTickStepAutomatic ? QCPAxisTicker::tssReadability : QCPAxisTicker::tssFixed);
                    ticker->setTickStep(t.tickStep);
                    ticker->setSubTickStep(t.subTickStep);
                }
                ax->setTicker(QSharedPointer<QCPAxisTicker>(ticker));
                break;
            }
            case Enums::AxisScale::Logarithmic: {
                ax->setScaleType(QCPAxis::stLogarithmic);
                ax->setTicker(logTicker);
                break;
            }
            case Enums::AxisScale::ThirdOctave: {
                ax->setScaleType(QCPAxis::stLogarithmic);
                ax->setTicker(octaveTicker);
                break;
            }
        }
    }

    replot();
}

void QCPPlot::setTimeAxisScale(Enums::AxisType axisType, int scale)
{
    if (axisType != Enums::AxisType::atLeft) return;
    auto curves = m->curves();
    if (curves.isEmpty()) return;

    //мы должны извлечь данные из канала: Ttr и Vel
    QStringList description = curves.first()->channel->description().split(",");
    if (description.isEmpty()) return;
    double velocity = 0;
    double traverseTime = 0;
    for (const QString &s: description) {
        if (s.startsWith("Vel=")) velocity = s.midRef(4).toDouble();
        if (s.startsWith("Ttr=")) traverseTime = s.midRef(4).toDouble();
    }
    if (velocity == 0.0) return;
    qDebug()<<velocity << traverseTime;
}

Enums::AxisScale QCPPlot::axisScale(Enums::AxisType axisType) const
{
    return axisParameters.value(axisType).scale;
}

void QCPPlot::setAxisRange(Enums::AxisType axisType, double min, double max)
{
    if (auto a = axis(axisType))
        a->setRange(min, max);
    else if (axisType == Enums::AxisType::atColor && colorScale)
        colorScale->setDataRange({min, max});
}

void QCPPlot::setInfoVisible(bool visible)
{
    //replot only if visibility is changed
    if (infoOverlay->visible() != visible) {
        infoOverlay->setVisible(visible);
        layer("overlay")->replot();
    }
}

void QCPPlot::enableAxis(Enums::AxisType axisType, bool enable)
{
    if (auto a = axis(axisType)) a->setVisible(enable);
}

bool QCPPlot::axisEnabled(Enums::AxisType axisType)
{
    if (auto a = axis(axisType)) return a->visible();
    return false;
}

void QCPPlot::setAxisTitle(Enums::AxisType axisType, const QString &title)
{
    if (auto a = axis(axisType)) a->setLabel(title);
}

QString QCPPlot::axisTitle(Enums::AxisType axisType) const
{
    if (auto a = axis(axisType)) return a->label();
    return QString();
}

void QCPPlot::enableColorBar(Enums::AxisType axisType, bool enable)
{
    if (!colorScale) {
        plotLayout()->setColumnSpacing(0);
        axisRect()->setMinimumMargins(QMargins(0,6,0,0));
        colorScale = new QCPColorScale(this);
        colorScale->setMinimumMargins(QMargins(0,6,0,0));
        plotLayout()->addElement(0, 1, colorScale); // add it to the right of the main axis rect
        colorScale->setType(toQcpAxis(axisType)); // scale shall be vertical bar with tick/axis labels right (actually atRight is already the default)

        QCPMarginGroup *marginGroup = new QCPMarginGroup(this);
        axisRect()->setMarginGroup(QCP::msBottom|QCP::msTop, marginGroup);
        colorScale->setMarginGroup(QCP::msBottom|QCP::msTop, marginGroup);

        colorScale->axis()->setSubTicks(true);
        connect(colorScale->axis(), &QCPAxis::draggingFinished, [=](const QCPRange &newRange){
            ZoomStack::zoomCoordinates coords;
            for (auto axis: axisRect()->axes()) {
                coords.insert(fromQcpAxis(axis->axisType()), axis->range());
            }
            coords.insert(Enums::AxisType::atColor, newRange);
            zoom->addZoom(coords, true);
        });
        connect(colorScale->axis(), &QCPAxis::rangeScaled, this, &QCPPlot::addZoom);

    }
    colorScale->setVisible(enable);
}

void QCPPlot::setColorMap(int colorMap, Curve *curve)
{
    auto gradient = ColorMapFactory::gradient(colorMap);
    if (auto c = dynamic_cast<QCPSpectrogram*>(curve)) c->setGradient(gradient);
    if (colorScale) colorScale->rescaleDataRange(true);
}

void QCPPlot::setColorBarTitle(const QString &title)
{
    if (colorScale) colorScale->setLabel(title);
}

void QCPPlot::importPlot(ImageRenderDialog::PlotRenderOptions options)
{
    axisRect()->setBorderPen(QPen(Qt::black));
//    checkableLegend->widget()->setVisible(true);
    legend->setVisible(true);
    switch (options.legendPosition) {
        case 0: axisRect(0)->insetLayout()->setInsetAlignment(0, Qt::AlignBottom); break;
        case 1: axisRect(0)->insetLayout()->setInsetAlignment(0, Qt::AlignTop); break;
    }
    updateLegend();
    if (options.graphOnly) setSecondaryPlotsVisible(false);
    QString format = options.path.section(".", -1,-1);
    if (!saveRastered(options.path,
                      int(0.0393700787401575 * options.size.width() * options.resolution),
                      int(0.0393700787401575 * options.size.height() * options.resolution),
                      1.0,
                      format.toLatin1().data(),
                      -1,
                      options.resolution))
        QMessageBox::critical(this, "Сохранение рисунка", "Не удалось сохранить график");
    legend->setVisible(false);
    if (options.graphOnly) setSecondaryPlotsVisible(true);
    axisRect()->setBorderPen(Qt::NoPen);
}

void QCPPlot::importPlot(QPrinter &printer, ImageRenderDialog::PlotRenderOptions options)
{
    printer.setOrientation(QPrinter::Landscape);

    QPrintDialog printDialog(&printer, this);
    if (printDialog.exec()) {
        qreal left,right,top,bottom;
        printer.getPageMargins(&left, &top, &right, &bottom, QPrinter::Millimeter);
        printer.setPageMargins(15, 15, 15, bottom, QPrinter::Millimeter);

        //настройка отображения графиков
        axisRect()->setBorderPen(QPen(Qt::black));
        legend->setVisible(true);
        switch (options.legendPosition) {
            case 0: axisRect(0)->insetLayout()->setInsetAlignment(0, Qt::AlignBottom); break;
            case 1: axisRect(0)->insetLayout()->setInsetAlignment(0, Qt::AlignTop); break;
        }

        updateLegend();
        if (options.graphOnly) setSecondaryPlotsVisible(false);

        QCPPainter painter(&printer);
        QRectF pageRect = printer.pageRect(QPrinter::DevicePixel);

        if (plotType != Enums::PlotType::Spectrogram)
            painter.setMode(QCPPainter::pmVectorized);
        painter.setMode(QCPPainter::pmNoCaching);
        painter.setMode(QCPPainter::pmNonCosmetic); // comment this out if you want cosmetic thin lines (always 1 pixel thick independent of pdf zoom level)
        toPainter(&painter, pageRect.width(), pageRect.height());

        //восстанавливаем параметры графиков
        legend->setVisible(false);
        if (options.graphOnly) setSecondaryPlotsVisible(true);
        axisRect()->setBorderPen(Qt::NoPen);
    }
}

Curve *QCPPlot::createCurve(Channel *channel, Enums::AxisType xAxis, Enums::AxisType yAxis)
{
    if (plotType == Enums::PlotType::Spectrogram) {
        auto g = new QCPSpectrogram(channel, axis(xAxis), this->yAxis);

        if (colorScale) g->setColorScale(colorScale);
        return g;
    }

    return new Graph2D(channel, axis(xAxis), axis(yAxis));
}

double QCPPlot::tickDistance(Enums::AxisType axisType) const
{
    auto ax = axis(axisType);

    if (ax && ax->scaleType() != QCPAxis::stLogarithmic) {
        auto v = ax->tickVector();
        if (v.size()>1) return v[1]-v[0];
    }

    return 0.0;
}

bool QCPPlot::isCurve(Selectable *item) const
{
    if (dynamic_cast<QCPAbstractPlottable*>(item))
        return true;
    return false;
}

QCPAxis *QCPPlot::axis(Enums::AxisType axis) const
{
    if (axis == Enums::AxisType::atColor && colorScale)
        return colorScale->axis();
    return axisRect(0)->axis(toQcpAxis(axis));
}

Enums::AxisType QCPPlot::axisType(QCPAxis *axis) const
{
    if (colorScale && axis == colorScale->axis()) return Enums::AxisType::atColor;
    return fromQcpAxis(axis->axisType());
}

bool QCPPlot::canBePlottedOnLeftAxis(Channel *ch, QString *message) const
{
    if (m->isEmpty()) return true;

    //не можем строить временные графики на графике спектров и наоборот
    if (ch->type() == Descriptor::TimeResponse && type() != Enums::PlotType::Time) {
        if (message) *message = "Нельзя строить временные графики на графике спектров";
        return false;
    }
    if (ch->type() != Descriptor::TimeResponse && type() == Enums::PlotType::Time) {
        if (message) *message = "Отсутствуют временные данные";
        return false;
    }

    if (ch->data()->blocksCount()<=1 && plotType == Enums::PlotType::Spectrogram) {
        if (message) *message = "Отсутствуют данные для сонограммы";
        return false;
    }

    if (PhysicalUnits::Units::unitsAreSame(ch->xName(), xName) || xName.isEmpty()) { // тип графика совпадает
        if (m->leftCurvesCount()==0) return true;
        if (yLeftName.isEmpty()) return true;
        if (PhysicalUnits::Units::unitsAreSame(plotType == Enums::PlotType::Spectrogram ? ch->zName(): ch->yName(), yLeftName))
            return true;
        if (message) *message = "Единицы по оси Y не совпадают";
    }
    else if (message) *message = "Единицы по оси X не совпадают";
    return false;
}

bool QCPPlot::canBePlottedOnRightAxis(Channel *ch, QString *message) const
{
    if (m->isEmpty()) return true;

    if (plotType == Enums::PlotType::Spectrogram)
        return false;

    //не можем строить временные графики на графике спектров и наоборот
    if (ch->type() == Descriptor::TimeResponse && type() != Enums::PlotType::Time) {
        if (message) *message = "Нельзя строить временные графики на графике спектров";
        return false;
    }
    if (ch->type() != Descriptor::TimeResponse && type() == Enums::PlotType::Time) {
        if (message) *message = "Отсутствуют временные данные";
        return false;
    }

    if (PhysicalUnits::Units::unitsAreSame(ch->xName(), xName) || xName.isEmpty()) { // тип графика совпадает
        if (m->rightCurvesCount()==0) return true;
        if (yRightName.isEmpty()) return true;
        if (PhysicalUnits::Units::unitsAreSame(ch->yName(), yRightName)) return true;
        if (message) *message = "Единицы по оси Y не совпадают";
    }
    else if (message) *message = "Единицы по оси X не совпадают";
    return false;
}

void QCPPlot::setAxis(Enums::AxisType axis, const QString &name)
{
    switch (axis) {
        case Enums::AxisType::atLeft: yLeftName = name; break;
        case Enums::AxisType::atRight: yRightName = name; break;
        case Enums::AxisType::atBottom: xName = name; break;
        default: break;
    }
}

void QCPPlot::plotChannel(Channel *ch, bool plotOnLeft, int fileIndex)
{
    if (!ch) return;
    //проверяем, построен ли канал на этом графике
    if (m->plotted(ch)) return;

    QString message;
    if ((plotOnLeft && !canBePlottedOnLeftAxis(ch, &message)) || (!plotOnLeft && !canBePlottedOnRightAxis(ch, &message))) {
        QMessageBox::warning(this, QString("Не могу построить канал"),
                             QString("%1.\nСначала очистите график.").arg(message));
        return;
    }
    setInfoVisible(false);

    if (plotType == Enums::PlotType::Spectrogram) {
        //скрываем все до этого построенные каналы
        for (auto c: m->curves()) {
            c->setVisible(false);
            if (auto p = dynamic_cast<QCPAbstractPlottable*>(c)) p->setVisible(false);
            c->updateInLegend();
        }
    }

    if (!ch->populated()) {
        ch->populate();
    }

    setAxis(Enums::AxisType::atBottom, ch->xName());
    enableAxis(Enums::AxisType::atBottom, true);

    int defaultYAxisPresentation = se->getSetting("defaultYAxisPresentation", 0).toInt();

    if (plotType == Enums::PlotType::Spectrogram) {
        yValuesPresentationLeft = DataHolder::ShowAsReals;
        if (m->isEmpty()) {
            if (defaultYAxisPresentation == 0)
                yValuesPresentationRight = ch->data()->yValuesPresentation();
            else
                yValuesPresentationRight = defaultYAxisPresentation;
        }
        ch->data()->setYValuesPresentation(yValuesPresentationRight);
    }
    else {
        // если графиков нет, по умолчанию будем строить амплитуды по первому добавляемому графику
        if (plotOnLeft && m->leftCurvesCount()==0 && !sergeiMode) {
            if (defaultYAxisPresentation == 0)
                yValuesPresentationLeft = ch->data()->yValuesPresentation();
            else
                yValuesPresentationLeft = defaultYAxisPresentation;
            if (plotType == Enums::PlotType::Time)
                yValuesPresentationLeft = DataHolder::ShowAsReals;
        }
        if (!plotOnLeft && m->rightCurvesCount()==0 && !sergeiMode) {
            if (defaultYAxisPresentation == 0)
                yValuesPresentationRight = ch->data()->yValuesPresentation();
            else
                yValuesPresentationRight = defaultYAxisPresentation;
            if (plotType == Enums::PlotType::Time)
                yValuesPresentationRight = DataHolder::ShowAsReals;
        }

        if (plotOnLeft) ch->data()->setYValuesPresentation(yValuesPresentationLeft);
        else ch->data()->setYValuesPresentation(yValuesPresentationRight);
    }

    plotOnLeft = plotOnLeft || plotType == Enums::PlotType::Spectrogram;
    auto axY = plotOnLeft ? Enums::AxisType::atLeft : Enums::AxisType::atRight;

    enableColorBar(Enums::AxisType::atRight, plotType == Enums::PlotType::Spectrogram);

    setAxis(axY, plotType == Enums::PlotType::Spectrogram ? ch->zName() : ch->yName());
    enableAxis(axY, true);

    if (plotType == Enums::PlotType::Spectrogram) {
        setAxis(Enums::AxisType::atRight, ch->yName());
    }


    Curve *g = createCurve(ch, Enums::AxisType::atBottom, axY);
    QColor nextColor = getNextColor();
    QPen pen = g->pen();
    pen.setColor(nextColor);
    pen.setWidth(1);
    g->setPen(pen);

//    PlottedModel::instance().add(ch,g);

    m->addCurve(g, plotOnLeft);
    g->fileNumber = fileIndex;

    if (plotType == Enums::PlotType::Spectrogram)
        setColorMap(colorMap, g);

    if (plotType == Enums::PlotType::Spectrogram) {
        zoom->scaleBounds(Enums::AxisType::atBottom)->add(g->xMin(), g->xMax(), true);
        zoom->scaleBounds(Enums::AxisType::atColor)->add(ch->data()->yMin(-1), ch->data()->yMax(-1), true);
        zoom->scaleBounds(Enums::AxisType::atLeft)->add(ch->data()->zMin(), ch->data()->zMax(), true);
    }
    else {
        if (auto bounds = zoom->scaleBounds(axY))
            bounds->add(g->yMin(), g->yMax());
        zoom->scaleBounds(Enums::AxisType::atBottom)->add(g->xMin(), g->xMax());
    }

    g->setLegend(checkableLegend);
    g->attachTo(this);
    setCurrentCurve(g);

    update();
    updatePlottedIndexes();
    emit channelPlotted(ch);
}

void QCPPlot::setSecondaryPlotsVisible(bool visible)
{
    if (!subLayout) return;

    if (visible) {
        plotLayout()->addElement(0, 2, subLayout);
        subLayout->setVisible(true);
        plotLayout()->setColumnStretchFactors({4,0.1,2});
        replot();
    }
    else {
        plotLayout()->take(subLayout);
        plotLayout()->simplify();
        subLayout->setVisible(false);
        plotLayout()->setColumnStretchFactors({4,0.1});
        replot();
    }
}

QColor QCPPlot::getNextColor()
{
    return colors->getColor();
}

void QCPPlot::addZoom()
{DD;
    auto coords = ZoomStack::zoomCoordinates();
    for (auto axis: axisRect()->axes())
        coords.insert(fromQcpAxis(axis->axisType()), axis->range());
    if (colorScale) {
        auto ax = colorScale->axis();
        coords.insert(Enums::AxisType::atColor, ax->range());
    }

    zoom->addZoom(coords, true);
}

void QCPPlot::dragEnterEvent(QDragEnterEvent *event)
{
    emit focusThisPlot();
    const ChannelsMimeData *myData = qobject_cast<const ChannelsMimeData *>(event->mimeData());
    if (myData) {
        //определяем, можем ли построить все каналы на левой или правой оси
        bool canOnLeft = std::all_of(myData->channels.cbegin(), myData->channels.cend(),
                                     [this](Channel*c){return canBePlottedOnLeftAxis(c);});
        bool canOnRight = std::all_of(myData->channels.cbegin(), myData->channels.cend(),
                                     [this](Channel*c){return canBePlottedOnRightAxis(c);});
        if (canOnLeft || canOnRight)
            event->acceptProposedAction();
    }
}

void QCPPlot::dragMoveEvent(QDragMoveEvent *event)
{
    const ChannelsMimeData *myData = dynamic_cast<const ChannelsMimeData *>(event->mimeData());
    if (myData) {
        //определяем, можем ли построить каналы на левой или правой оси
        //определяем, в какой области мы находимся
        bool plotOnLeft = event->pos().x() <= axisRect()->rect().x() + axisRect()->rect().width()/2;

        if (leftOverlay) leftOverlay->setVisibility(plotOnLeft);
        if (rightOverlay) rightOverlay->setVisibility(!plotOnLeft);

            event->acceptProposedAction();
    }
}

void QCPPlot::dragLeaveEvent(QDragLeaveEvent *event)
{
    Q_UNUSED(event);
    if (leftOverlay) leftOverlay->setVisibility(false);
    if (rightOverlay) rightOverlay->setVisibility(false);
}

void QCPPlot::dropEvent(QDropEvent *event)
{
    const ChannelsMimeData *myData = qobject_cast<const ChannelsMimeData *>(event->mimeData());
    if (myData) {
        bool plotOnLeft = event->pos().x() <= axisRect()->rect().x()+axisRect()->rect().width()/2;
        onDropEvent(plotOnLeft, myData->channels);
        event->acceptProposedAction();
    }
    if (leftOverlay) leftOverlay->setVisibility(false);
    if (rightOverlay) rightOverlay->setVisibility(false);
}

QWidget *QCPPlot::toolBarWidget()
{
    return playerPanel;
}

QWidget *QCPPlot::legendWidget()
{
    return checkableLegend->widget();
}

void QCPPlot::updateActions(int filesCount, int channelsCount)
{
    Q_UNUSED(filesCount);
    Q_UNUSED(channelsCount);
    if (playerPanel)
        playerPanel->setEnabled(!m->isEmpty());
}

void QCPPlot::updatePlottedIndexes()
{DD;
    if (!sergeiMode) m->updatePlottedIndexes();
    emit curvesCountChanged(); //->MainWindow.updateActions
}

void QCPPlot::plotCurvesForDescriptor(FileDescriptor *d, int fileIndex)
{DD;
    if (sergeiMode) {
        deleteAllCurves(false);
        m->updatePlottedIndexes(d, fileIndex);
        const auto plotted = m->plottedIndexes();
        for (const auto &i: plotted) plotChannel(i.ch, i.onLeft, i.fileIndex);
    }
}

void QCPPlot::update()
{DD;
    for (auto c: m->curves()) {
        c->updatePen();
    }
    m->updateTitles();
    updateLabels();
    updateAxesLabels();
    updateLegend();

    updateBounds();

    replot();
}

void QCPPlot::updateBounds()
{DD;
    if (m->leftCurvesCount()==0) {
        zoom->scaleBounds(Enums::AxisType::atLeft)->reset();
        zoom->scaleBounds(Enums::AxisType::atColor)->reset();
    }
    if (m->rightCurvesCount()==0)
        zoom->scaleBounds(Enums::AxisType::atRight)->reset();
    if (m->isEmpty())
        zoom->scaleBounds(Enums::AxisType::atBottom)->reset();

    if (!zoom->scaleBounds(Enums::AxisType::atBottom)->isFixed())
        zoom->scaleBounds(Enums::AxisType::atBottom)->autoscale();
    if (m->leftCurvesCount()>0 && !zoom->scaleBounds(Enums::AxisType::atLeft)->isFixed())
        zoom->scaleBounds(Enums::AxisType::atLeft)->autoscale();
    if (m->leftCurvesCount()>0 && !zoom->scaleBounds(Enums::AxisType::atColor)->isFixed())
        zoom->scaleBounds(Enums::AxisType::atColor)->autoscale();
    if (m->rightCurvesCount()>0 && !zoom->scaleBounds(Enums::AxisType::atRight)->isFixed())
        zoom->scaleBounds(Enums::AxisType::atRight)->autoscale();
}

QMenu *QCPPlot::createMenu(Enums::AxisType axis, const QPoint &pos)
{DD;
    QMenu *menu = new QMenu(this);

    if (axis == Enums::AxisType::atBottom) {
        if (plotType == Enums::PlotType::Spectrogram) {
            menu->addAction("Одинарный курсор", [=](){
                auto cursor = cursors->addSingleCursor(localCursorPosition(pos), Cursor::Style::Cross);
                addCursorToSecondaryPlots(cursor);
            });
        }
        else {
            auto scm = new QMenu("Одинарный курсор", menu);
            scm->addAction("Вертикальный", [=](){
                auto cursor = cursors->addSingleCursor(localCursorPosition(pos), Cursor::Style::Vertical);
                addCursorToSecondaryPlots(cursor);
            });
            scm->addAction("Перекрестье", [=]() {
                auto cursor = cursors->addSingleCursor(localCursorPosition(pos), Cursor::Style::Cross);
                addCursorToSecondaryPlots(cursor);
            });
            menu->addMenu(scm);

            auto dcm = new QMenu("Двойной курсор", menu);
            dcm->addAction("Стандартный", [=](){
                auto cursor = cursors->addDoubleCursor(localCursorPosition(pos), Cursor::Style::Vertical);
                addCursorToSecondaryPlots(cursor);
            });
            dcm->addAction("Режекция", [=](){
                auto cursor = cursors->addRejectCursor(localCursorPosition(pos), Cursor::Style::Vertical);
                addCursorToSecondaryPlots(cursor);
            });
            menu->addMenu(dcm);
        }

        menu->addAction("Гармонический курсор", [=](){
            auto cursor = cursors->addHarmonicCursor(localCursorPosition(pos));
            addCursorToSecondaryPlots(cursor);
        });

        auto axisScaleAct = new QAction("Шкала", menu);
        auto axisScaleMenu = new QMenu(menu);
        axisScaleAct->setMenu(axisScaleMenu);
        auto scaleGroup = new QActionGroup(axisScaleMenu);

        auto linear = new QAction("Линейная", scaleGroup);
        linear->setCheckable(true);
        linear->setData(static_cast<int>(Enums::AxisScale::Linear));
        axisScaleMenu->addAction(linear);

        auto logar = new QAction("Логарифмическая", scaleGroup);
        logar->setCheckable(true);
        logar->setData(static_cast<int>(Enums::AxisScale::Logarithmic));
        axisScaleMenu->addAction(logar);

        auto oct = new QAction("Третьоктава", scaleGroup);
        oct->setCheckable(true);
        oct->setData(static_cast<int>(Enums::AxisScale::ThirdOctave));
        axisScaleMenu->addAction(oct);

        switch (axisScale(axis)) {
            case Enums::AxisScale::Linear: linear->setChecked(true); break;
            case Enums::AxisScale::Logarithmic: logar->setChecked(true); break;
            case Enums::AxisScale::ThirdOctave: oct->setChecked(true); break;
        }

        connect(scaleGroup, &QActionGroup::triggered, [=](QAction *act){
            Enums::AxisScale scale = static_cast<Enums::AxisScale>(act->data().toInt());
            setAxisScale(Enums::AxisType::atBottom, scale);
        });

        menu->addAction(axisScaleAct);

        // определяем, все ли графики представляют временные данные
        if (const auto type = m->curvesDataType();
            type == Descriptor::TimeResponse && axis == Enums::AxisType::atBottom) {
            menu->addAction("Сохранить временной сегмент", [=](){
                auto range = plotRange(axis);

                emit saveTimeSegment(m->plottedDescriptors(), range.min, range.max);
            });
        }
    }

    // Попытка написать шкалу дистанции
    if (axis == Enums::AxisType::atLeft && plotType == Enums::PlotType::Spectrogram) {
        auto axisScaleAct = new QAction("Шкала", menu);
        auto axisScaleMenu = new QMenu(menu);
        axisScaleAct->setMenu(axisScaleMenu);
        auto scaleGroup = new QActionGroup(axisScaleMenu);

        auto time = new QAction("Время", scaleGroup);
        time->setCheckable(true);
        time->setData(0);
        axisScaleMenu->addAction(time);

        auto dist = new QAction("Дистанция", scaleGroup);
        dist->setCheckable(true);
        dist->setData(1);
        axisScaleMenu->addAction(dist);

        int scale = se->getSetting("spectrogramTimeScale", 0).toInt();

        switch (scale) {
            case 0: time->setChecked(true); break;
            case 1: dist->setChecked(true); break;
            default: break;
        }

        connect(scaleGroup, &QActionGroup::triggered, [=](QAction *act){
            int scale = act->data().toInt();
            se->setSetting("spectrogramTimeScale", scale);
        });

        menu->addAction(axisScaleAct);
    }

    bool curvesEmpty = true;
    bool leftCurves = true;
    int *ax = 0;

    if (axis == Enums::AxisType::atLeft && m->leftCurvesCount()>0) {
        curvesEmpty = false;
        ax = &yValuesPresentationLeft;
    }
    if (axis == Enums::AxisType::atRight && m->rightCurvesCount()>0) {
        curvesEmpty = false;
        ax = &yValuesPresentationRight;
        leftCurves = false;
    }
    if (axis == Enums::AxisType::atColor && m->leftCurvesCount()>0) {
        curvesEmpty = false;
        ax = &yValuesPresentationRight;
    }

    if (!curvesEmpty) {
        QAction *a = new QAction("Показывать как");
        QMenu *am = new QMenu(0);
        QActionGroup *ag = new QActionGroup(am);

        QAction *act3 = new QAction("Амплитуды линейные", ag);
        act3->setCheckable(true);
        if (ax && *ax == DataHolder::ShowAsAmplitudes) act3->setChecked(true);
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
            m->setYValuesPresentation(leftCurves, presentation);
            *ax = presentation;
            this->recalculateScale(axis);
            auto sb = zoom->scaleBounds(Enums::AxisType::atBottom);
            auto fixed = sb->isFixed();
            sb->setFixed(true); //блокируем обновление границ шкалы х
            this->update();
            sb->setFixed(fixed, false);
        });

        a->setMenu(am);
        menu->addAction(a);
    }

    if (axis == Enums::AxisType::atColor && m->leftCurvesCount()>0) {
        QAction *a = new QAction("Цветовая шкала");
        QMenu *am = new QMenu(this);
        QActionGroup *ag = new QActionGroup(am);

        const QStringList l = ColorMapFactory::names();
        for (int i=0; i<l.size(); ++i) {
            QAction *act1 = new QAction(l.at(i), ag);
            act1->setCheckable(true);
            if (i == colorMap) act1->setChecked(true);
            act1->setData(i);
            am->addAction(act1);
        }

        connect(ag, &QActionGroup::triggered, [=](QAction*act){
            int map = act->data().toInt();
            if (map != colorMap) {
                colorMap = map;
                setColorMap(map, m->curve(0, true));
                replot();
            }
        });

        a->setMenu(am);
        menu->addAction(a);
    }

    if (m->leftCurvesCount()>0 && m->rightCurvesCount()>0) {
        menu->addSection("Левая и правая оси");
        menu->addAction("Совместить нули левой и правой осей", [=](){
            // 1. Центруем нуль левой оси
            auto range = plotRange(Enums::AxisType::atLeft);
            double s1 = range.min;
            double s2 = range.max;

            range = screenRange(Enums::AxisType::atLeft);

            double p1 = range.min;
            double p2 = range.max;
            double delta = screenToPlotCoordinates(Enums::AxisType::atLeft, (p1+p2)/2.0);

            ZoomStack::zoomCoordinates coords;
            coords.insert(Enums::AxisType::atLeft, {s1-delta, s2-delta});

            // 2. Центруем нуль правой оси
            range = plotRange(Enums::AxisType::atRight);
            s1 = range.min;
            s2 = range.max;

            range = screenRange(Enums::AxisType::atRight);

            p1 = range.min;
            p2 = range.max;
            delta = screenToPlotCoordinates(Enums::AxisType::atRight, (p1+p2)/2.0);

            coords.insert(Enums::AxisType::atRight, {s1-delta, s2-delta});

            range = plotRange(Enums::AxisType::atBottom);
            coords.insert(Enums::AxisType::atBottom, {range.min, range.max});

            zoom->addZoom(coords, true);
        });
        menu->addAction("Совместить диапазоны левой и правой осей", [=](){
            auto range = plotRange(Enums::AxisType::atLeft);
            double s1 = range.min;
            double s2 = range.max;

            range = plotRange(Enums::AxisType::atRight);
            double s3 = range.min;
            double s4 = range.max;
            double s = std::min(s1,s3);
            double ss = std::min(s2,s4);

            ZoomStack::zoomCoordinates coords;
            coords.insert(Enums::AxisType::atLeft, {s, ss});
            coords.insert(Enums::AxisType::atRight, {s, ss});

            range = plotRange(Enums::AxisType::atBottom);
            coords.insert(Enums::AxisType::atBottom, {range.min, range.max});

            zoom->addZoom(coords, true);
        });
    }
    return menu;
}

void QCPPlot::cycleChannels(bool up)
{DD;
    //есть список кривых, возможно, из разных записей. Необходимо для каждой записи
    //получить список индексов, сдвинуть этот список вверх или вниз,
    //а затем удалить имеющиеся кривые и построить сдвинутые, причем соблюдая порядок
    //отображения.
    //Т.о. :
    //- для каждой кривой определяем запись, номер канала. Если возможно, сдвигаем
    //номер канала, запоминаем канал


    m->cycleChannels(up);

    const bool sm = sergeiMode;
    sergeiMode = true;
    deleteAllCurves();
    for (const auto &c: m->plottedIndexes()) {
        plotChannel(c.ch, c.onLeft, c.fileIndex);
    }
    sergeiMode = sm;
}

int QCPPlot::curvesCount(int type) const
{DD;
    return m->size(type);
}

void QCPPlot::deleteAllCurves(bool forceDeleteFixed)
{DD;
    for (int i=m->size()-1; i>=0; --i) {
        Curve *c = m->curve(i);
        if (!c->fixed || forceDeleteFixed) {
            deleteCurve(c, i==0);
        }
    }

    updatePlottedIndexes();
}

void QCPPlot::canvasDoubleClicked(const QPoint &position)
{
    int canvasDoubleClick = se->getSetting("canvasDoubleClick", 1).toInt();
    int canvasDoubleClickCursor = se->getSetting("canvasDoubleClickCursor", 0).toInt();

    if (canvasDoubleClick == 1) {//во временных передвигаем курсор, в спектрах создаем курсор
        if (playerPanel) playerPanel->moveTo(position);
        else {
            auto cursor = cursors->addSingleCursor(position,
                                                   canvasDoubleClickCursor == 0 ? Cursor::Style::Vertical : Cursor::Style::Cross);
            addCursorToSecondaryPlots(cursor);
        }
    }
    else {
        auto cursor = cursors->addSingleCursor(position,
                                               canvasDoubleClickCursor == 0 ? Cursor::Style::Vertical : Cursor::Style::Cross);
        addCursorToSecondaryPlots(cursor);
    }
}

void QCPPlot::deleteCurvesForDescriptor(FileDescriptor *descriptor, QVector<int> indexes)
{DD;
    if (indexes.isEmpty()) {
        for (int i=0; i<descriptor->channelsCount(); ++i) indexes << i;
    }

    for (int i: indexes) {
        if (Curve *curve = m->plotted(descriptor->channel(i)))
            deleteCurve(curve, i==indexes.last());
    }

    updatePlottedIndexes();
}

//не удаляем, если фиксирована
void QCPPlot::deleteCurveFromLegend(Curve *curve)
{DD;
    if (!curve->fixed) {
        deleteCurve(curve, true);
        updatePlottedIndexes();
    }
}

void QCPPlot::deleteCurveForChannelIndex(FileDescriptor *dfd, int channel, bool doReplot)
{DD;
    if (Curve *curve = m->plotted(dfd->channel(channel))) {
        deleteCurve(curve, doReplot);
        updatePlottedIndexes();
    }
}

void QCPPlot::deleteSelectedCurve(Selectable *selected)
{DD;
    if (Curve *curve = dynamic_cast<Curve*>(selected)) {
        deleteCurve(curve, true);
        updatePlottedIndexes();
    }
}

//удаляет кривую, была ли она фиксирована или нет.
//Все проверки проводятся выше
void QCPPlot::deleteCurve(Curve *curve, bool doReplot)
{DD;
    if (!curve) return;
    if (curve->selected()) picker->deselect();

    bool removedFromLeft = true;
    if (m->deleteCurve(curve, &removedFromLeft)) {
//        PlottedModel::instance().remove(curve);
        emit curveDeleted(curve->channel); //->MainWindow.onChannelChanged
        colors->freeColor(curve->pen().color());

        if (plotType != Enums::PlotType::Spectrogram) {
            if (removedFromLeft > 0) {
                zoom->scaleBounds(Enums::AxisType::atLeft)->removeToAutoscale(curve->yMin(), curve->yMax());
            }
            else {
                zoom->scaleBounds(Enums::AxisType::atRight)->removeToAutoscale(curve->yMin(), curve->yMax());
            }
            zoom->scaleBounds(Enums::AxisType::atBottom)->removeToAutoscale(curve->xMin(), curve->xMax());
        }

        curve->detachFrom(this);
        delete curve;
        setCurrentCurve(nullptr);

        if (m->leftCurvesCount()==0) {
            yLeftName.clear();
        }
        if (m->rightCurvesCount()==0 || plotType == Enums::PlotType::Spectrogram) {
            yRightName.clear();
            enableAxis(Enums::AxisType::atRight, false);
        }
        if (m->isEmpty()) xName.clear();
        setInfoVisible(m->size()==0);
        if (doReplot) update();
    }
}

void QCPPlot::showContextMenu(const QPoint &pos, Enums::AxisType axis)
{DD;
    if (m->isEmpty()) return;

    QMenu *menu = createMenu(axis, pos);

    if (!menu->actions().isEmpty())
        menu->exec(pos);
    menu->deleteLater();
}

void QCPPlot::addSelectable(Selectable *item)
{
    if (!selectables.contains(item))
        selectables.append(item);
}

void QCPPlot::removeSelectable(Selectable *item)
{
    selectables.removeOne(item);
}

void QCPPlot::deselect()
{
    for (auto candidate: selectables) {
        candidate->setSelected(false, SelectedPoint());
    }
}

Selected QCPPlot::findSelected(QPoint pos) const
{
    //Ищем элемент под курсором мыши
    Selected selected {nullptr, SelectedPoint()};

    //сначала ищем метки, курсоры и т.д., то есть не кривые
    {
        double minDist = qInf();
        for (auto candidate: selectables) {
            if (isCurve(candidate)) continue;
            double distx = 0.0;
            double disty = 0.0;
            SelectedPoint point;
            if (candidate->underMouse(pos, &distx, &disty, &point)) {
                double dist = 0.0;
                if (distx == qInf()) dist = disty;
                else if (disty == qInf()) dist = distx;
                else dist = sqrt(distx*distx+disty*disty);
                if (!selected.object || dist < minDist) {
                    selected.object = candidate;
                    selected.point = point;
                    minDist = dist;
                }
            }
        }
    }
    if (!selected.object) {
        double minDist = qInf();
        for (auto candidate: selectables) {
            if (!isCurve(candidate)) continue;

            double distx = 0.0;
            double disty = 0.0;
            SelectedPoint point;
            if (candidate->underMouse(pos, &distx, &disty, &point)) {
                double dist = 0.0;
                if (distx == qInf()) dist = disty;
                else if (disty == qInf()) dist = distx;
                else dist = sqrt(distx*distx+disty*disty);
                if (!selected.object || dist < minDist) {
                    selected.object = candidate;
                    selected.point = point;
                    minDist = dist;
                }
            }
        }
    }
    return selected;
}

QString QCPPlot::pointCoordinates(const QPointF &pos)
{DD;
    if (plotType != Enums::PlotType::Spectrogram)
        return smartDouble(pos.x(), tickDistance(Enums::AxisType::atBottom))+", "+
                smartDouble(pos.y(), tickDistance(Enums::AxisType::atLeft));

    bool success = false;
    double y = 0.0;
    if (auto c = model()->curve(0))
        y = c->channel->data()->YforXandZ(pos.x(), pos.y(), success);
    if (success)
        return smartDouble(pos.x(), tickDistance(Enums::AxisType::atBottom))+", "
                +smartDouble(pos.y(), tickDistance(Enums::AxisType::atLeft)) + ", "
                +smartDouble(y, tickDistance(Enums::AxisType::atRight));

    return smartDouble(pos.x(), tickDistance(Enums::AxisType::atBottom))+", "
            +smartDouble(pos.y(), tickDistance(Enums::AxisType::atLeft));
}

void QCPPlot::updateAxesLabels()
{DD;
    if (plotType != Enums::PlotType::Spectrogram) {
        if (m->leftCurvesCount()==0) enableAxis(Enums::AxisType::atLeft, false);
        else {
            enableAxis(Enums::AxisType::atLeft, axisLabelsVisible);
            QString suffix = yValuesPresentationSuffix(yValuesPresentationLeft);
            //        QString text(QString("%1 <small>%2</small>").arg(yLeftName).arg(suffix));
            QString text(QString("%1 %2").arg(yLeftName).arg(suffix));
            setAxisTitle(Enums::AxisType::atLeft, axisLabelsVisible ? text : "");
        }

        if (m->rightCurvesCount()==0) enableAxis(Enums::AxisType::atRight, false);
        else {
            enableAxis(Enums::AxisType::atRight, axisLabelsVisible);
            QString suffix = yValuesPresentationSuffix(yValuesPresentationRight);
            //        QString text(QString("%1 <small>%2</small>").arg(yRightName).arg(suffix));
            QString text(QString("%1 %2").arg(yRightName).arg(suffix));
            setAxisTitle(Enums::AxisType::atRight, axisLabelsVisible ? text : "");
        }
    }
    else {
        enableAxis(Enums::AxisType::atLeft, axisLabelsVisible);
        setAxisTitle(Enums::AxisType::atLeft, axisLabelsVisible ? yLeftName : "");

        //правая ось - цветовая шкала
        enableAxis(Enums::AxisType::atRight, false);
        enableColorBar(Enums::AxisType::atRight, true);
        QString suffix = yValuesPresentationSuffix(yValuesPresentationRight);
        QString text(QString("%1 %2").arg(yRightName).arg(suffix));
        setColorBarTitle(axisLabelsVisible ? text : "");
    }

    if (axisEnabled(Enums::AxisType::atBottom)) {
        setAxisTitle(Enums::AxisType::atBottom, axisLabelsVisible ? xName : "");
    }
    replot();
}

void QCPPlot::removeLabels()
{DD;
    m->removeLabels();
    replot();
}

void QCPPlot::moveCurve(Curve *curve, Enums::AxisType axis)
{DD;
    if (type()==Enums::PlotType::Spectrogram) return;

    if ((axis == Enums::AxisType::atLeft && canBePlottedOnLeftAxis(curve->channel))
        || (axis == Enums::AxisType::atRight && canBePlottedOnRightAxis(curve->channel))) {
        enableAxis(axis, true);
        setAxis(axis, curve->channel->yName());
        curve->setYAxis(axis);

        if (axis == Enums::AxisType::atRight && m->rightCurvesCount()==0)
            yValuesPresentationRight = curve->channel->data()->yValuesPresentation();
        if (axis == Enums::AxisType::atLeft && m->leftCurvesCount()==0)
            yValuesPresentationLeft = curve->channel->data()->yValuesPresentation();

        bool moved = m->moveToOtherAxis(curve);
        if (moved) {
            curve->channel->data()->setYValuesPresentation(axis == Enums::AxisType::atRight ? yValuesPresentationRight
                                                                                   : yValuesPresentationLeft);
        }
        emit curvesCountChanged();

        updateAxesLabels();
        zoom->moveToAxis(axis, curve->channel->data()->yMin(), curve->channel->data()->yMax());
    }
    else QMessageBox::warning(this, "Не могу поменять ось", "Эта ось уже занята графиком другого типа!");
}

QString QCPPlot::yValuesPresentationSuffix(int yValuesPresentation) const
{DD;
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

void QCPPlot::saveSpectrum(double zVal)
{DD;
    QVector<double> vals;
    if (qIsNaN(zVal)) {
        //сохранение для всех курсоров
        for (auto c: cursors->cursors()) {
            if (c->type() != Cursor::Type::Single) continue;

            vals << c->currentPosition().y();
        }
    }
    else {
        vals << zVal;
    }
    emit saveHorizontalSlice(vals);
}

void QCPPlot::saveThroughput(double xVal)
{DD;
    QVector<double> vals;
    if (qIsNaN(xVal)) {
        //сохранение для всех курсоров
        for (auto c: cursors->cursors()) {
            if (c->type() != Cursor::Type::Single) continue;

            vals << c->currentPosition().x();
        }
    }
    else {
        vals << xVal;
    }
    emit saveVerticalSlice(vals);
}

void QCPPlot::updateLabels()
{
    for (auto curve: m->curves()) {
        curve->updateLabels();
    }
}

void QCPPlot::removeCursor(Selectable *selected)
{
    if (auto cursor = cursors->cursorFor(selected)) {
        removeCursorFromSecondaryPlots(cursor);
        cursors->removeCursor(cursor);
    }
}

void QCPPlot::recalculateScale(Enums::AxisType axis)
{DD;
    auto bounds = zoom->scaleBounds(axis);
    if (!bounds) return;

    bounds->reset();

    const bool left = axis == Enums::AxisType::atColor || axis == Enums::AxisType::atLeft;
    const auto count = left ? m->leftCurvesCount() : m->rightCurvesCount();

    for (int i=0; i<count; ++i) {
        auto curve = m->curve(i, left);
        bounds->add(curve->yMin(), curve->yMax());
    }
}

void QCPPlot::switchLabelsVisibility()
{DD;
    axisLabelsVisible = !axisLabelsVisible;
    updateAxesLabels();
}

void QCPPlot::updateLegends()
{DD;
    m->updateTitles();
    updateLegend();
}

void QCPPlot::savePlot() /*SLOT*/
{DD;
    ImageRenderDialog dialog(true, type()==Enums::PlotType::Spectrogram, nullptr);
    if (dialog.exec()) {
        importPlot(dialog.getRenderOptions());
    }
}

void QCPPlot::copyToClipboard(bool useDialog) /*SLOT*/
{DD;
    QTemporaryFile file(QDir::tempPath()+"/DeepSeaBase-XXXXXX.bmp");
    if (file.open()) {
        temporaryFiles->add(file.fileName());

        QString fileName = file.fileName();
        file.close();

        ImageRenderDialog::PlotRenderOptions options = ImageRenderDialog::defaultRenderOptions();

        if (useDialog) {
            ImageRenderDialog dialog(false, type()==Enums::PlotType::Spectrogram, 0);
            if (dialog.exec()) options = dialog.getRenderOptions();
        }
        options.path = fileName;
        importPlot(options);

        QImage img;
        if (img.load(fileName))
            qApp->clipboard()->setImage(img);
        else {
            QMessageBox::critical(0, "Копирование рисунка", "Не удалось скопировать рисунок");
            LOG(ERROR)<<"Could not load image from"<<fileName;
        }

    }
    else QMessageBox::critical(0, "Копирование рисунка", "Не удалось создать временный файл");
}

void QCPPlot::print() /*SLOT*/
{DD;
    QPrinter printer;
    if (printer.isValid()) {
        auto options = ImageRenderDialog::defaultRenderOptions();
        options.graphOnly = true;
        importPlot(printer, options);
    }
    else
        QMessageBox::warning(this, "Deepsea Base", "Не удалось найти подходящий принтер");
}

void QCPPlot::switchInteractionMode()
{DD;
    if (interactionMode == Enums::InteractionMode::ScalingInteraction) {
        interactionMode = Enums::InteractionMode::DataInteraction;
    }
    else {
        interactionMode = Enums::InteractionMode::ScalingInteraction;
    }
}

void QCPPlot::switchCursorBox()
{DD;
    if (cursorBox) cursorBox->setVisible(!cursorBox->isVisible());
}

void QCPPlot::toggleAutoscale(Enums::AxisType axis, bool toggled)
{DD;
    if (auto b = zoom->scaleBounds(axis)) b->setFixed(!toggled);

    replot();
}

void QCPPlot::autoscale(Enums::AxisType axis)
{DD;
    zoom->autoscale(axis);
}

void QCPPlot::editLegendItem(Curve *curve)
{DD;
    if (curve->type == Curve::Type::Spectrogram) return; //у спектрограммы нет свойств кривой

    CurvePropertiesDialog dialog(curve, this);
    if (cursorBox) connect(&dialog, SIGNAL(curveChanged(Curve*)), cursorBox, SLOT(updateLayout()));
    dialog.exec();
}

void QCPPlot::onDropEvent(bool plotOnLeft, const QVector<Channel*> &channels)
{DD;
    //посылаем сигнал о том, что нужно построить эти каналы. Список каналов попадает
    //в mainWindow и возможно будет расширен за счет нажатого Ctrl
    //далее эти каналы попадут обратно в plot.
    emit needPlotChannels(plotType == Enums::PlotType::Spectrogram || plotOnLeft, channels);
}
