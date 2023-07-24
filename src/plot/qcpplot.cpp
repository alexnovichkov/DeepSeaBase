#include "qcpplot.h"
#include "plot/plot.h"
#include "plot/zoomstack.h"
#include "plot/plotmodel.h"
#include "graph2d.h"
#include "qcpspectrogram.h"
#include "plot/canvaseventfilter.h"
#include "axisboundsdialog.h"
#include "checkablelegend.h"
#include "mousecoordinates.h"
#include "settings.h"
#include "channelsmimedata.h"
#include "qcpaxisoverlay.h"
#include "qcpaxistickeroctave.h"
#include "logging.h"
#include "plot/colormapfactory.h"
#include "qcpinfooverlay.h"
#include "qcpflowlegend.h"
#include "secondaryplot.h"


QCPAxis::AxisType toQcpAxis(Enums::AxisType type) {
    return static_cast<QCPAxis::AxisType>(type);
}

Enums::AxisType fromQcpAxis(QCPAxis::AxisType type) {
    return static_cast<Enums::AxisType>(type);
}

QCPPlot::QCPPlot(Plot *plot, QWidget *parent) : QCustomPlot(parent), parent(plot)
{
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);

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

    setInteractions(QCP::iRangeDrag|
                    QCP::iRangeZoom);
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

            plot->zoom->addZoom(coords, true);
        });
        connect(axis, &QCPAxis::rangeScaled, this, &QCPPlot::addZoom);
    }

    setEventFilter(new CanvasEventFilter(plot));

    if (plot->type() == Enums::PlotType::Spectrogram) {
        subLayout = new QCPLayoutGrid;
        plotLayout()->addElement(0, 2, subLayout);

        spectrePlot = new SpectrePlot(this, "Спектр", subLayout);
        throughPlot = new ThroughPlot(this, "Проходная", subLayout);
        plotLayout()->setColumnStretchFactors({4,0.1,2});
    }
}

QCPPlot::~QCPPlot()
{
    delete canvasFilter;
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
    replot();
}

void QCPPlot::setEventFilter(CanvasEventFilter *filter)
{
    canvasFilter = filter;
    canvasFilter->setZoom(parent->zoom);
    canvasFilter->setPicker(parent->picker);

    connect(canvasFilter, SIGNAL(canvasDoubleClicked(QPoint)), this, SIGNAL(canvasDoubleClicked(QPoint)));
    //    connect(canvasFilter, &CanvasEventFilter::hover, this, &QwtPlotImpl::hoverAxis);
    connect(canvasFilter, &CanvasEventFilter::contextMenuRequested, parent, &Plot::showContextMenu);
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

        parent->zoom->addZoom(coords, true);
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
    legend->setVisible(false);

    checkableLegend = new QCPCheckableLegend(this);
    parent->legend = checkableLegend->widget();


    connect(checkableLegend, &QCPCheckableLegend::markedForDelete,
            parent, &Plot::deleteCurveFromLegend);
    connect(checkableLegend, &QCPCheckableLegend::clicked,
            parent, &Plot::editLegendItem);
    connect(checkableLegend, &QCPCheckableLegend::markedToMove, [=](Curve *c){
        parent->moveCurve(c, c->yAxis() == Enums::AxisType::atLeft ? Enums::AxisType::atRight : Enums::AxisType::atLeft);
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
                    parent->zoom->scaleBounds(Enums::AxisType::atBottom)->add(c->xMin(), c->xMax(), true);
                    parent->zoom->scaleBounds(Enums::AxisType::atColor)->add(c->channel->data()->yMin(-1), c->channel->data()->yMax(-1), true);
                    parent->zoom->scaleBounds(Enums::AxisType::atLeft)->add(c->channel->data()->zMin(), c->channel->data()->zMax(), true);
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
    for (auto c: parent->model()->curves())
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
    auto curves = parent->model()->curves();
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

void QCPPlot::setAxisRange(Enums::AxisType axisType, double min, double max, double step)
{
    Q_UNUSED(step);
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
            parent->zoom->addZoom(coords, true);
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

        if (parent->type() != Enums::PlotType::Spectrogram)
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
    if (parent->type() == Enums::PlotType::Spectrogram) {
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

void QCPPlot::addZoom()
{DD;
    auto coords = ZoomStack::zoomCoordinates();
    for (auto axis: axisRect()->axes())
        coords.insert(fromQcpAxis(axis->axisType()), axis->range());
    if (colorScale) {
        auto ax = colorScale->axis();
        coords.insert(Enums::AxisType::atColor, ax->range());
    }

    parent->zoom->addZoom(coords, true);
}

void QCPPlot::dragEnterEvent(QDragEnterEvent *event)
{
    parent->focusPlot();
    const ChannelsMimeData *myData = qobject_cast<const ChannelsMimeData *>(event->mimeData());
    if (myData) {
        //определяем, можем ли построить все каналы на левой или правой оси
        bool canOnLeft = std::all_of(myData->channels.cbegin(), myData->channels.cend(),
                                     [this](Channel*c){return parent->canBePlottedOnLeftAxis(c);});
        bool canOnRight = std::all_of(myData->channels.cbegin(), myData->channels.cend(),
                                     [this](Channel*c){return parent->canBePlottedOnRightAxis(c);});
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
        parent->onDropEvent(plotOnLeft, myData->channels);
        event->acceptProposedAction();
    }
    if (leftOverlay) leftOverlay->setVisibility(false);
    if (rightOverlay) rightOverlay->setVisibility(false);
}
