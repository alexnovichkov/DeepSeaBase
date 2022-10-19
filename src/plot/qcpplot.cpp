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
    logTicker.reset(new QCPAxisTickerLog);
    logTicker->setLogBase(2);
    octaveTicker.reset(new QCPAxisTickerOctave);

    leftOverlay = new QCPLeftAxisOverlay(this);
    rightOverlay = new QCPRightAxisOverlay(this);

    oldCursor = cursor();

    infoOverlay = new QCPInfoOverlay(this);
    infoOverlay->setVisible(true);

    addLayer("mouse");
    layer("mouse")->setMode(QCPLayer::lmBuffered);

    mouseCoordinates = new MouseCoordinates(this);
    mouseCoordinates->setLayer("mouse");

    setNotAntialiasedElement(QCP::aeScatters , true);
    setNoAntialiasingOnDrag(true);

    setInteractions(QCP::iRangeDrag|
                    QCP::iRangeZoom);
    setSelectionRectMode(QCP::srmZoom);
    setMouseTracking(true);
    setPlottingHints( QCP::phCacheLabels | /*QCP::phFastPolylines |*/ QCP::phImmediateRefresh);

    connect(this, &QCustomPlot::axisDoubleClick, [=](QCPAxis *axis, QCPAxis::SelectablePart part, QMouseEvent *event) {
        Q_UNUSED(part);
        Q_UNUSED(event);
        auto range = axis->range();
        auto type = fromQcpAxis(axis->axisType());
        AxisBoundsDialog dialog(range.lower, range.upper, type);
        if (dialog.exec()) {
            ZoomStack::zoomCoordinates coords;
            auto axes = axisRect()->axes();
            if (axes.contains(axis)) { //5 axes types
                for (auto ax: axes) {
                    if (axis == ax) coords.coords.insert(fromQcpAxis(ax->axisType()), {dialog.leftBorder(), dialog.rightBorder()});
                    else coords.coords.insert(fromQcpAxis(ax->axisType()), {ax->range().lower, ax->range().upper});
                }
                if (colorScale) {
                    auto ax = colorScale->axis();
                    coords.coords.insert(Enums::AxisType::atColor, {ax->range().lower, ax->range().upper});
                }
            }
            else {//5 axes types
                for (auto ax: axes) {
                    coords.coords.insert(fromQcpAxis(ax->axisType()), {ax->range().lower, ax->range().upper});
                }
                coords.coords.insert(Enums::AxisType::atColor, {dialog.leftBorder(), dialog.rightBorder()});
            }

            plot->zoom->addZoom(coords, true);
        }
    });
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
        connect(axis, &QCPAxis::contextMenuRequested, [=](const QPoint &pos, QCPAxis::AxisType type){
            plot->showContextMenu(pos, static_cast<Enums::AxisType>(type));
        });
        connect(axis, &QCPAxis::draggingFinished, [=](const QCPRange &newRange){
            ZoomStack::zoomCoordinates coords;
            for (auto ax: axisRect()->axes()) {
                if (axis == ax) coords.coords.insert(fromQcpAxis(axis->axisType()), {newRange.lower, newRange.upper});
                else coords.coords.insert(fromQcpAxis(ax->axisType()), {ax->range().lower, ax->range().upper});
            }
            if (colorScale) {
                auto ax = colorScale->axis();
                coords.coords.insert(Enums::AxisType::atColor, {ax->range().lower, ax->range().upper});
            }

            plot->zoom->addZoom(coords, true);
        });
        connect(axis, &QCPAxis::rangeScaled, this, &QCPPlot::addZoom);
    }

    setEventFilter(new CanvasEventFilter(plot));

    if (plot->type() == Enums::PlotType::Spectrogram) {
        spectreRect = new QCPAxisRect(this);
        throughRect = new QCPAxisRect(this);

        QCPLayoutGrid *subLayout = new QCPLayoutGrid;
        plotLayout()->addElement(0, 2, subLayout);

        spectreTitle = new QCPTextElement(this, "Спектр");
        throughTitle = new QCPTextElement(this, "Проходная");

        subLayout->addElement(0,0,spectreTitle);
        subLayout->addElement(1,0,spectreRect);
        subLayout->addElement(2,0,throughTitle);
        subLayout->addElement(3,0,throughRect);
        plotLayout()->setColumnStretchFactors({4,0.1,2});
        QList<QCPAxis*> allAxes;
        allAxes << spectreRect->axes() << throughRect->axes();
        for (QCPAxis *axis: allAxes)
        {
          axis->setLayer("axes");
          axis->grid()->setLayer("grid");
        }
        spectreGraph = addGraph(spectreRect->axis(QCPAxis::atBottom), spectreRect->axis(QCPAxis::atLeft));
        throughGraph = addGraph(throughRect->axis(QCPAxis::atBottom), throughRect->axis(QCPAxis::atLeft));
        spectreGraph->removeFromLegend();
        throughGraph->removeFromLegend();
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
    }
}

void QCPPlot::updateSecondaryPlots(const QPointF &value)
{DD;
    static QPointF oldValue;
    if (!qIsNaN(value.x())) oldValue = value;

    Curve *curve = nullptr;
    for (auto c: parent->model()->curves()) {
        if (auto plottable = dynamic_cast<QCPAbstractPlottable*>(c); plottable->visible()) {
            curve = c;
            break;
        }
    }
    if (!curve) {
        if (spectreGraph) spectreGraph->setVisible(false);
        if (throughGraph) throughGraph->setVisible(false);
        if (spectreTitle) spectreTitle->setText("Спектр");
        if (throughTitle) throughTitle->setText("Проходная");
    }
    else {
        if (spectreRect) {
            spectreGraph->setVisible(true);
            auto zIndex = curve->channel->data()->nearestZ(oldValue.y());
            if (zIndex < 0) zIndex = 0;
            QVector<double> data = curve->channel->data()->yValues(zIndex);
            QVector<double> xData = curve->channel->data()->xValues();
            spectreGraph->setData(xData, data, true);
            for (auto axis: spectreRect->axes()) axis->rescale(true);
            spectreGraph->rescaleAxes();
            spectreTitle->setText(QString("Спектр %1 %2")
                                  .arg(curve->channel->data()->zValue(zIndex))
                                  .arg(curve->channel->zName()));
        }
        if (throughRect) {
            throughGraph->setVisible(true);
            QVector<double> data;
            double xIndex = curve->channel->data()->nearest(oldValue.x());
            if (xIndex < 0) xIndex = 0;
            for (int i=0; i<curve->channel->data()->blocksCount(); ++i)
                data << curve->channel->data()->yValue(xIndex, i);
            QVector<double> xData = curve->channel->data()->zValues();
            throughGraph->setData(xData, data, true);
            throughGraph->rescaleAxes();
            throughTitle->setText(QString("Проходная %1 %2")
                                  .arg(curve->channel->data()->xValue(xIndex))
                                  .arg(curve->channel->xName()));
        }
    }
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
    installEventFilter(filter);
}

QCPAxis *QCPPlot::eventTargetAxis(QEvent *event, QObject *target)
{
    Q_UNUSED(target);
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
    legend->setFillOrder(QCPLayoutGrid::FillOrder::foColumnsFirst);
    legend->setWrap(6);
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
            if (visible) p->addToLegend();
            else p->removeFromLegend();
        }

        updateSecondaryPlots({qQNaN(), qQNaN()});
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

void QCPPlot::updateAxes()
{
    //QCustomPlot::axisRect(0)->update()
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

    if (scale == Enums::AxisScale::Linear) {
        a->setScaleType(QCPAxis::stLinear);
        a->setTicker(linTicker);
        if (spectreGraph) {
            spectreGraph->keyAxis()->setScaleType(QCPAxis::stLinear);
            spectreGraph->keyAxis()->setTicker(linTicker);
        }
    }
    else if (scale == Enums::AxisScale::Logarithmic) {
        a->setScaleType(QCPAxis::stLogarithmic);
        if (parent->type() == Enums::PlotType::Octave)
            a->setTicker(octaveTicker);
        else
            a->setTicker(logTicker);
        if (spectreGraph) {
            spectreGraph->keyAxis()->setScaleType(QCPAxis::stLogarithmic);
            if (parent->type() == Enums::PlotType::Octave)
                spectreGraph->keyAxis()->setTicker(octaveTicker);
            else
                spectreGraph->keyAxis()->setTicker(logTicker);
        }
    }

    replot();
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
    infoOverlay->setVisible(visible);
    layer("overlay")->replot();
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

        //color bar has its own axis, so connect all the signals
        connect(colorScale->axis(), &QCPAxis::contextMenuRequested, [=](const QPoint &pos, QCPAxis::AxisType type){
            Q_UNUSED(type);
            parent->showContextMenu(pos, Enums::AxisType::atColor);
        });

        colorScale->axis()->setSubTicks(true);
        connect(colorScale->axis(), &QCPAxis::draggingFinished, [=](const QCPRange &newRange){
            ZoomStack::zoomCoordinates coords;
            for (auto axis: axisRect()->axes()) {
                coords.coords.insert(fromQcpAxis(axis->axisType()), {axis->range().lower, axis->range().upper});
            }
            coords.coords.insert(Enums::AxisType::atColor, {newRange.lower, newRange.upper});
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

void QCPPlot::importPlot(const QString &fileName, const QSize &size, int resolution)
{
    legend->setVisible(true);
    updateLegend();
    QString format = fileName.section(".", -1,-1);
    if (!saveRastered(fileName, int(0.0393700787401575 * size.width() * resolution),
                      int(0.0393700787401575 * size.height() * resolution), 1.0, format.toLatin1().data(), -1, resolution))
        QMessageBox::critical(this, "Сохранение рисунка", "Не удалось сохранить график");
    legend->setVisible(false);
}

void QCPPlot::importPlot(QPrinter &printer, const QSize &size, int resolution)
{
    Q_UNUSED(size);
    Q_UNUSED(resolution);

    printer.setOrientation(QPrinter::Landscape);

    QPrintDialog printDialog(&printer, this);
    if (printDialog.exec()) {
        qreal left,right,top,bottom;
        printer.getPageMargins(&left, &top, &right, &bottom, QPrinter::Millimeter);
        printer.setPageMargins(15, 15, 15, bottom, QPrinter::Millimeter);

        //настройка отображения графиков
        legend->setVisible(true);
        updateLegend();

        QCPPainter painter(&printer);
        QRectF pageRect = printer.pageRect(QPrinter::DevicePixel);

        painter.setMode(QCPPainter::pmVectorized);
        painter.setMode(QCPPainter::pmNoCaching);
        painter.setMode(QCPPainter::pmNonCosmetic); // comment this out if you want cosmetic thin lines (always 1 pixel thick independent of pdf zoom level)
        toPainter(&painter, pageRect.width(), pageRect.height());

        //восстанавливаем параметры графиков
        legend->setVisible(false);
    }
}

Curve *QCPPlot::createCurve(const QString &legendName, Channel *channel, Enums::AxisType xAxis, Enums::AxisType yAxis)
{
    if (parent->type() == Enums::PlotType::Spectrogram) {
        auto g = new QCPSpectrogram(legendName, channel, axis(xAxis), this->yAxis);

        if (colorScale) g->setColorScale(colorScale);
        return g;
    }

    return new Graph2D(legendName, channel, axis(xAxis), axis(yAxis));
}

Selected QCPPlot::findObject(QPoint pos) const
{
    QList<QVariant> details;
//    QList<QCPLayerable*> candidates = layerableListAt(pos, false, &details);
    auto candidates = parent->getSelectables();

    auto isCurve = [](auto item){if (dynamic_cast<QCPAbstractPlottable*>(item)) return true; return false;};

    //Ищем элемент под курсором мыши
    Selected selected {nullptr, SelectedPoint()};

    //сначала ищем метки, курсоры и т.д., то есть не кривые
    {
        double minDist = qInf();
        for (auto candidate: candidates) {
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
        for (auto candidate: candidates) {
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

void QCPPlot::deselect()
{
    QList<QVariant> details;
    auto candidates = parent->getSelectables();
    for (auto candidate: candidates) {
            candidate->setSelected(false, SelectedPoint());
    }
}

QCPAxis *QCPPlot::axis(Enums::AxisType axis) const
{
    return axisRect(0)->axis(toQcpAxis(axis));
}

void QCPPlot::addZoom()
{DD;
    auto coords = ZoomStack::zoomCoordinates();
    for (auto axis: axisRect()->axes())
        coords.coords.insert(fromQcpAxis(axis->axisType()), {axis->range().lower, axis->range().upper});
    if (colorScale) {
        auto ax = colorScale->axis();
        coords.coords.insert(Enums::AxisType::atColor, {ax->range().lower, ax->range().upper});
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
