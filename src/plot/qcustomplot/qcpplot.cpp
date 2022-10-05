#include "qcpplot.h"
#include "plot/plot.h"
#include "plot/zoomstack.h"
#include "plot/plotmodel.h"
#include "graph2d.h"
#include "plot/canvaseventfilter.h"
#include "axisboundsdialog.h"
#include "checkablelegend.h"
#include "mousecoordinates.h"
#include "settings.h"
#include "channelsmimedata.h"
#include "qcpaxisoverlay.h"

QCPAxis::AxisType toQcpAxis(Enums::AxisType type) {
    return static_cast<QCPAxis::AxisType>(type);
}

Enums::AxisType fromQcpAxis(QCPAxis::AxisType type) {
    return static_cast<Enums::AxisType>(type);
}

QCPPlot::QCPPlot(Plot *plot, QWidget *parent) : QCustomPlot(parent), parent(plot)
{
    setAcceptDrops(true);

    linTicker.reset(new QCPAxisTicker);
    logTicker.reset(new QCPAxisTickerLog);
    logTicker->setLogBase(2);

    leftOverlay = new QCPLeftAxisOverlay(this);
    rightOverlay = new QCPRightAxisOverlay(this);

    oldCursor = cursor();

    addLayer("mouse");
    layer("mouse")->setMode(QCPLayer::lmBuffered);

    mouseCoordinates = new MouseCoordinates(this);
    mouseCoordinates->setLayer("mouse");

    setNotAntialiasedElement(QCP::aeScatters , true);
    setNoAntialiasingOnDrag(true);

    setInteractions(QCP::iRangeDrag|
                    QCP::iRangeZoom|
                    QCP::iSelectPlottables|
                    QCP::iSelectItems|
                    QCP::iSelectOther);
    setSelectionRectMode(QCP::srmZoom);
    setMouseTracking(true);
    setPlottingHints( QCP::phCacheLabels | /*QCP::phFastPolylines |*/ QCP::phImmediateRefresh);

    connect(this, &QCustomPlot::axisDoubleClick, [=](QCPAxis *axis, QCPAxis::SelectablePart part, QMouseEvent *event){
        Q_UNUSED(part);
        Q_UNUSED(event);
        auto range = axis->range();
        auto type = fromQcpAxis(axis->axisType());
        AxisBoundsDialog dialog(range.lower, range.upper, type);
        if (dialog.exec()) {
            ZoomStack::zoomCoordinates coords;
            for (auto ax: axisRect()->axes()) {
                if (ax == axis) coords.coords.insert(type, {dialog.leftBorder(), dialog.rightBorder()});
                else coords.coords.insert(fromQcpAxis(ax->axisType()), {ax->range().lower, ax->range().upper});
            }
            plot->zoom->addZoom(coords, true);
        }
    });
    axisRect()->setRangeDragAxes({xAxis, yAxis, yAxis2});
    axisRect()->setRangeZoomAxes({xAxis, yAxis, yAxis2});
    connect(axisRect(), &QCPAxisRect::axesRangeScaled, this, &QCPPlot::addZoom);
    connect(axisRect(), &QCPAxisRect::draggingFinished, this, &QCPPlot::addZoom);
    connect(this, &QCustomPlot::mouseMove, mouseCoordinates, &MouseCoordinates::update);
    for (auto ax: axisRect()->axes()) {
        ax->setSubTicks(true);
        connect(ax, &QCPAxis::contextMenuRequested, [=](const QPoint &pos, QCPAxis::AxisType type){
            plot->showContextMenu(pos, static_cast<Enums::AxisType>(type));
        });
        connect(ax, &QCPAxis::draggingFinished, [=](const QCPRange &newRange){
            auto coords = ZoomStack::zoomCoordinates();
            for (auto axis: axisRect()->axes()) {
                if (axis == ax) coords.coords.insert(fromQcpAxis(axis->axisType()), {newRange.lower, newRange.upper});
                else coords.coords.insert(fromQcpAxis(axis->axisType()), {axis->range().lower, axis->range().upper});
            }
            qDebug() << coords.coords;

            plot->zoom->addZoom(coords, true);
        });
        connect(ax, &QCPAxis::rangeScaled, this, &QCPPlot::addZoom);
    }
}

QCPPlot::~QCPPlot()
{
    delete canvasFilter;
}

void QCPPlot::setEventFilter(CanvasEventFilter *filter)
{
    canvasFilter = filter;
        canvasFilter->setZoom(parent->zoom);
    //    canvasFilter->setDragZoom(dragZoom);
    //    canvasFilter->setWheelZoom(wheelZoom);
    //    canvasFilter->setAxisZoom(axisZoom);
    //    canvasFilter->setPlotZoom(plotZoom);
        canvasFilter->setPicker(parent->picker);

        connect(canvasFilter, SIGNAL(canvasDoubleClicked(QPoint)), this, SIGNAL(canvasDoubleClicked(QPoint)));
    //    connect(canvasFilter, &CanvasEventFilter::hover, this, &QwtPlotImpl::hoverAxis);
        connect(canvasFilter, &CanvasEventFilter::contextMenuRequested, parent, &Plot::showContextMenu);
        installEventFilter(filter);

    axisRect(0)->installEventFilter(filter);
    for (auto ax: axisRect(0)->axes()) ax->installEventFilter(filter);
}

Enums::AxisType QCPPlot::eventTargetAxis(QEvent *event, QObject *target)
{
    if (auto mouseEvent = dynamic_cast<QMouseEvent*>(event)) {

        QList<QVariant> details;
        QList<QCPLayerable*> candidates = layerableListAt(mouseEvent->pos(), false, &details);
        if (candidates.isEmpty()) return Enums::AxisType::atInvalid;
        for (int i=0; i<candidates.size(); ++i) {
            if (auto ax = dynamic_cast<QCPAxis*>(candidates.at(i)))
                qDebug() << candidates.at(i) << details.at(i);
        }

    }
    return Enums::AxisType::atInvalid;
}



void QCPPlot::createLegend()
{
    this->axisRect(0)->insetLayout()->setInsetAlignment(0, Qt::AlignBottom | Qt::AlignCenter);
    legend->setFillOrder(QCPLayoutGrid::FillOrder::foColumnsFirst);
    legend->setWrap(6);
    legend->setVisible(false);

    checkableLegend = new QCPCheckableLegend(this);
    parent->legend = checkableLegend->widget();


    connect(checkableLegend, &QCPCheckableLegend::markedForDelete, [=](QCPAbstractPlottable *plottable){
        if (Curve *c = dynamic_cast<Curve *>(plottable))
            parent->deleteCurveFromLegend(c);
    });
    connect(checkableLegend, &QCPCheckableLegend::clicked, [=](QCPAbstractPlottable *plottable){
        if (Curve *c = dynamic_cast<Curve *>(plottable))
            parent->editLegendItem(c);
    });
    connect(checkableLegend, &QCPCheckableLegend::markedToMove, [=](QCPAbstractPlottable *plottable){
        if (Curve *c = dynamic_cast<Curve *>(plottable)) {
            parent->moveCurve(c, c->yAxis() == Enums::AxisType::atLeft ? Enums::AxisType::atRight : Enums::AxisType::atLeft);
            replot();
        }
    });
    connect(checkableLegend, &QCPCheckableLegend::fixedChanged, [=](QCPAbstractPlottable *plottable){
        if (Curve *c = dynamic_cast<Curve *>(plottable)) {
            c->switchFixed();
            checkableLegend->updateItem(plottable, c->commonLegendData());
        }
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

void QCPPlot::updateAxes()
{
    //QCustomPlot::axisRect(0)->update()
}

void QCPPlot::updateLegend()
{
}

QPoint QCPPlot::localCursorPosition(const QPoint &globalCursorPosition) const
{
    return mapFromGlobal(globalCursorPosition);
}

void QCPPlot::setAxisScale(Enums::AxisType axisType, Enums::AxisScale scale)
{
    auto a = axis(axisType);
    if (!a) return;

    if (scale == Enums::AxisScale::Linear) {
        a->setScaleType(QCPAxis::stLinear);
        a->setTicker(linTicker);
    }
    else if (scale == Enums::AxisScale::Logarithmic) {
        a->setScaleType(QCPAxis::stLogarithmic);
        a->setTicker(logTicker);
    }

    replot();
}

void QCPPlot::setAxisRange(Enums::AxisType axisType, double min, double max, double step)
{
    Q_UNUSED(step);
    if (auto a = axis(axisType)) a->setRange(min, max);
}

void QCPPlot::setInfoVisible(bool visible)
{
    //TODO: infoOverlay
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
    //TODO: spectrogram
}

void QCPPlot::setColorMap(Enums::AxisType axisType, Range range, int colorMap, Curve *curve)
{
    //TODO: spectrogram
}

void QCPPlot::setColorMap(int colorMap, Curve *curve)
{
    //TODO: spectrogram
}

void QCPPlot::importPlot(const QString &fileName, const QSize &size, int resolution)
{
    legend->setVisible(true);
    QString format = fileName.section(".", -1,-1);
    if (!saveRastered(fileName, int(0.0393700787401575 * size.width() * resolution),
                      int(0.0393700787401575 * size.height() * resolution), 1.0, format.toLatin1().data(), -1, resolution))
        QMessageBox::critical(this, "Сохранение рисунка", "Не удалось сохранить график");
    legend->setVisible(false);
}

void QCPPlot::importPlot(QPrinter &printer, const QSize &size, int resolution)
{
    //TODO: print support
}

void QCPPlot::setInteractionMode(Enums::InteractionMode mode)
{
    Q_UNUSED(mode);
}

Curve *QCPPlot::createCurve(const QString &legendName, Channel *channel, Enums::AxisType xAxis, Enums::AxisType yAxis)
{
    auto g = new Graph2D(legendName, channel, axis(xAxis), axis(yAxis));

    return g;
}

Selected QCPPlot::findObject(QPoint pos) const
{
    return Selected();
}

void QCPPlot::deselect()
{
}

QCPAxis *QCPPlot::axis(Enums::AxisType axis) const
{
    return axisRect(0)->axis(toQcpAxis(axis));
}

void QCPPlot::addZoom()
{
    auto coords = ZoomStack::zoomCoordinates();
    for (auto axis: axisRect()->axes())
        coords.coords.insert(fromQcpAxis(axis->axisType()), {axis->range().lower, axis->range().upper});
    parent->zoom->addZoom(coords, true);
}

void QCPPlot::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
        case Qt::Key_Backspace: {
            parent->zoom->zoomBack();
            break;
        }
        default: QCustomPlot::keyPressEvent(event);
    }
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
