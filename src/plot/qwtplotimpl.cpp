#include "qwtplotimpl.h"

#include <QPrintDialog>

#include <qwt_plot_opengl_canvas.h>
#include <qwt_plot_canvas.h>
//#include <qwt_legend.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_curve.h>
#include <qwt_symbol.h>
#include <qwt_scale_widget.h>
#include <qwt_scale_map.h>
#include <qwt_scale_engine.h>
#include <qwt_color_map.h>
#include <qwt_plot_zoomer.h>
#include <qwt_picker.h>
#include <qwt_picker_machine.h>
#include <qwt_plot_renderer.h>
#include <qwt_plot_layout.h>
#include <qwt_plot_panner.h>
#include <qwt_plot_magnifier.h>
#include <qwt_text.h>
#include <qwt_plot_item.h>

#include "settings.h"
#include "fileformats/filedescriptor.h"
#include "plotinfooverlay.h"
#include "scaledraw.h"
#include "logscaleengine.h"
#include "grid.h"
#include "legend.h"
#include "curve.h"
#include "plot.h"
#include "channelsmimedata.h"
#include "logging.h"
#include "qwtlinecurve.h"
#include "qwtbarcurve.h"
#include "qwtspectrocurve.h"
#include "canvaseventfilter.h"
#include "picker.h"
#include "dragzoom.h"
#include "wheelzoom.h"
#include "axiszoom.h"
#include "plotzoom.h"
#include "colormapfactory.h"
#include "selectable.h"
#include "plottracker.h"

QwtPlotImpl::QwtPlotImpl(Plot *plot, QWidget *parent) : QwtPlot(parent), parent(plot)
{
    _canvas = new QwtPlotCanvas(this);
    _canvas->setFocusIndicator(QwtPlotAbstractCanvas::CanvasFocusIndicator);
    _canvas->setFrameStyle(QFrame::NoFrame);
    plotLayout()->setCanvasMargin(0);
    setCanvas(_canvas);
    setCanvasBackground(Qt::white);

    setAutoReplot(true);
    setAcceptDrops(true);

    infoOverlay = new PlotInfoOverlay(this);
    infoOverlay->setVisible(true);

    setAxisScaleDraw(QwtAxis::XBottom, new ScaleDraw());
    setAxisScaleDraw(QwtAxis::YLeft, new ScaleDraw());
    setAxisScaleDraw(QwtAxis::YRight, new ScaleDraw());

    axisWidget(QwtAxis::XBottom)->setMouseTracking(true);
    axisWidget(QwtAxis::YLeft)->setMouseTracking(true);
    axisWidget(QwtAxis::YRight)->setMouseTracking(true);

    tracker = new PlotTracker(plot);

    leftOverlay = new LeftAxisOverlay(this);
    rightOverlay = new RightAxisOverlay(this);

    grid = new Grid(this);

    dragZoom = new DragZoom(this);
    wheelZoom = new WheelZoom(this);
    plotZoom = new PlotZoom(this);
    axisZoom = new AxisZoom(this);
    connect(axisZoom, &AxisZoom::hover, this, &QwtPlotImpl::hoverAxis);
}

QwtPlotImpl::~QwtPlotImpl()
{
    delete grid;
    delete dragZoom;
    delete wheelZoom;
    delete axisZoom;
    delete canvasFilter;
    delete tracker;
}

void QwtPlotImpl::createLegend()
{
    CheckableLegend *leg = new CheckableLegend();
    connect(leg, SIGNAL(clicked(QwtPlotItem*)),this,SLOT(editLegendItem(QwtPlotItem*)));
    connect(leg, SIGNAL(markedForDelete(QwtPlotItem*)), this, SLOT(deleteCurveFromLegend(QwtPlotItem*)));
    connect(leg, &CheckableLegend::markedToMove, this, [this](QwtPlotItem*curve) {
        if (Curve *c = dynamic_cast<Curve*>(curve))
            parent->moveCurve(c, c->yAxis() == Enums::AxisType::atLeft ? Enums::AxisType::atRight : Enums::AxisType::atLeft);
    });
    connect(leg, SIGNAL(fixedChanged(QwtPlotItem*)), this, SLOT(fixCurve(QwtPlotItem*)));
    insertLegend(leg, QwtPlot::RightLegend);
}

void QwtPlotImpl::setEventFilter(CanvasEventFilter *filter)
{
    canvasFilter = filter;
    canvasFilter->setZoom(parent->zoom);
    canvasFilter->setDragZoom(dragZoom);
    canvasFilter->setWheelZoom(wheelZoom);
    canvasFilter->setAxisZoom(axisZoom);
    canvasFilter->setPlotZoom(plotZoom);
    canvasFilter->setPicker(parent->picker);

    connect(canvasFilter, SIGNAL(canvasDoubleClicked(QPoint)), this, SIGNAL(canvasDoubleClicked(QPoint)));
    connect(canvasFilter, &CanvasEventFilter::hover, this, &QwtPlotImpl::hoverAxis);
    connect(canvasFilter, &CanvasEventFilter::contextMenuRequested, parent, &Plot::showContextMenu);

    canvas()->installEventFilter(filter);
    for (int ax = 0; ax < QwtAxis::AxisPositions; ax++) {
        axisWidget(ax)->installEventFilter(filter);
        axisWidget(ax)->setFocusPolicy(Qt::StrongFocus);
    }
}

Enums::AxisType QwtPlotImpl::eventTargetAxis(QEvent *event, QObject *target)
{
    Q_UNUSED(event);
    if (target == canvas()) return Enums::AxisType::atInvalid;

    for (int a = 0; a < QwtAxis::AxisPositions; a++) {
        if (target == axisWidget(a)) {
            return toAxisType(a);
        }
    }

    return Enums::AxisType::atInvalid;
}

double QwtPlotImpl::screenToPlotCoordinates(Enums::AxisType axis, double value) const
{
    return invTransform(::toQwtAxisType(axis), value);
}

double QwtPlotImpl::plotToScreenCoordinates(Enums::AxisType axis, double value) const
{
    return transform(::toQwtAxisType(axis), value);
}

Range QwtPlotImpl::plotRange(Enums::AxisType axis) const
{
    auto map = canvasMap(toQwtAxisType(axis));
    return {map.s1(), map.s2()};
}

Range QwtPlotImpl::screenRange(Enums::AxisType axis) const
{
    auto map = canvasMap(toQwtAxisType(axis));
    return {map.p1(), map.p2()};
}

void QwtPlotImpl::replot()
{
    QwtPlot::replot();
}

void QwtPlotImpl::updateAxes()
{
    QwtPlot::updateAxes();
}

void QwtPlotImpl::updateLegend()
{
    QwtPlot::updateLegend();
}

QPoint QwtPlotImpl::localCursorPosition(const QPoint &globalCursorPosition) const
{
    return _canvas->mapFromGlobal(globalCursorPosition);
}

void QwtPlotImpl::setAxisScale(Enums::AxisType axis, Enums::AxisScale scale)
{
    if (scale == Enums::AxisScale::Linear)
        setAxisScaleEngine(toQwtAxisType(axis), new QwtLinearScaleEngine());
    else if (scale == Enums::AxisScale::Logarithmic)
        setAxisScaleEngine(toQwtAxisType(axis), new LogScaleEngine(2));
}

void QwtPlotImpl::setAxisRange(Enums::AxisType axis, double min, double max, double step)
{
    QwtPlot::setAxisScale(toQwtAxisType(axis), min, max, step);
}

void QwtPlotImpl::setInfoVisible(bool visible)
{
    infoOverlay->setVisible(visible);
}

void QwtPlotImpl::enableAxis(Enums::AxisType axis, bool enable)
{
    setAxisVisible(toQwtAxisType(axis), enable);
}

void QwtPlotImpl::enableColorBar(Enums::AxisType axis, bool enable)
{
    axisWidget(toQwtAxisType(axis))->setColorBarEnabled(enable);
}

void QwtPlotImpl::setColorMap(Enums::AxisType axis, Range range, int colorMap, Curve *curve)
{
    axisWidget(toQwtAxisType(axis))->setColorMap(QwtInterval(range.min, range.max),
                                                 ColorMapFactory::map(colorMap));
    if (QwtSpectroCurve *c = dynamic_cast<QwtSpectroCurve *>(curve)) {
        c->setColorInterval(range.min, range.max);
        c->setColorMap(ColorMapFactory::map(colorMap));
    }
}

void QwtPlotImpl::setColorMap(int colorMap, Curve *curve)
{
    if (QwtSpectroCurve *c = dynamic_cast<QwtSpectroCurve *>(curve)) {
        c->setColorMap(ColorMapFactory::map(colorMap));
    }
}

bool QwtPlotImpl::axisEnabled(Enums::AxisType axis)
{
    return isAxisVisible(toQwtAxisType(axis));
}

void QwtPlotImpl::setAxisTitle(Enums::AxisType axis, const QString &title)
{
    QwtText text(title, QwtText::RichText);
    QwtPlot::setAxisTitle(toQwtAxisType(axis), text);
}

QString QwtPlotImpl::axisTitle(Enums::AxisType axis) const
{
    return QwtPlot::axisTitle(toQwtAxisType(axis)).text();
}

void QwtPlotImpl::hoverAxis(Enums::AxisType axis, int hover)
{
    if (ScaleDraw * scale = dynamic_cast<ScaleDraw*>(axisScaleDraw(toQwtAxisType(axis)))) {
        if (scale->hover != hover) {
            scale->hover = hover;
            axisWidget(toQwtAxisType(axis))->update();
        }
    }
}

void QwtPlotImpl::importPlot(const QString &fileName, const QSize &size, int resolution)
{
    setAutoReplot(false);
    QwtPlotRenderer renderer;
    renderer.setDiscardFlag(QwtPlotRenderer::DiscardBackground);
    renderer.setLayoutFlag(QwtPlotRenderer::FrameWithScales);

    QFont axisfont = axisFont(QwtAxis::YLeft);
    axisfont.setPointSize(axisfont.pointSize()-1);
    for (int i=0; i<QwtPlot::axisCnt; ++i)
        if (isAxisVisible(i)) setAxisFont(i, axisfont);

//    for (Curve *curve: curves) {
//        QPen pen = curve->pen();
//        if (pen.width()<2) pen.setWidth(2);
//        pen.setColor(pen.color().lighter(120));
//        curve->setPen(pen);
//    }

    insertLegend(new QwtLegend(), QwtPlot::BottomLegend);


    QString format = fileName.section(".", -1,-1);
    renderer.renderDocument(this, fileName, format, size, resolution);


    axisfont.setPointSize(axisfont.pointSize()+1);
    for (int i=0; i<QwtPlot::axisCnt; ++i)
        if (isAxisVisible(i)) setAxisFont(i, axisfont);

//    for (Curve *curve: curves) {
//        curve->setPen(curve->oldPen);
//    }

    createLegend();
    setAutoReplot(true);
}

void QwtPlotImpl::importPlot(QPrinter &printer, const QSize &size, int resolution)
{
    Q_UNUSED(size);
    Q_UNUSED(resolution);

    setAutoReplot(false);
    printer.setOrientation(QPrinter::Landscape);

    QPrintDialog printDialog(&printer, this);
    if (printDialog.exec()) {
        qreal left,right,top,bottom;
        printer.getPageMargins(&left, &top, &right, &bottom, QPrinter::Millimeter);
        printer.setPageMargins(15, 15, 15, bottom, QPrinter::Millimeter);

        //настройка отображения графиков
        QFont axisfont = axisFont(QwtAxis::YLeft);
        axisfont.setPointSize(axisfont.pointSize()-2);
        for (int i=0; i<QwtPlot::axisCnt; ++i)
            if (isAxisVisible(i)) setAxisFont(i, axisfont);

        //настройка линий сетки
        grid->adaptToPrinter();

        insertLegend(new QwtLegend(), QwtPlot::BottomLegend);

        QwtPlotRenderer renderer;
        renderer.setDiscardFlag(QwtPlotRenderer::DiscardBackground);
        renderer.setLayoutFlag(QwtPlotRenderer::FrameWithScales);
        renderer.renderTo(this, printer);

        //восстанавливаем параметры графиков
        axisfont.setPointSize(axisfont.pointSize()+2);
        for (int i=0; i<QwtPlot::axisCnt; ++i)
            if (isAxisVisible(i)) setAxisFont(i, axisfont);
        grid->restore();

        createLegend();
    }

    setAutoReplot(true);
}

void QwtPlotImpl::setInteractionMode(Enums::InteractionMode mode)
{
    if (_canvas) _canvas->setFocusIndicator(mode == Enums::InteractionMode::ScalingInteraction?
                                              QwtPlotCanvas::CanvasFocusIndicator:
                                                QwtPlotCanvas::ItemFocusIndicator);
}

Curve *QwtPlotImpl::createCurve(const QString &legendName, Channel *channel, Enums::AxisType xAxis, Enums::AxisType yAxis)
{
    Curve *result = nullptr;

    if (channel->data()->blocksCount() > 1) result = new QwtSpectroCurve(legendName, channel);

    else if (channel->octaveType() != 0) {
        if (Settings::getSetting("plotOctaveAsHistogram", false).toBool())
            result = new QwtBarCurve(legendName, channel);
    }

    else result = new QwtLineCurve(legendName, channel);

    result->setXAxis(xAxis);
    result->setYAxis(yAxis);

    return result;
}

bool isCurve(QwtPlotItem *i)
{
    auto rtti = i->rtti();
    return (rtti == QwtPlotItem::Rtti_PlotCurve || rtti == QwtPlotItem::Rtti_PlotSpectrogram
            || rtti == QwtPlotItem::Rtti_PlotHistogram);
}

Selected QwtPlotImpl::findObject(QPoint pos) const
{
    //Ищем элемент под курсором мыши
    Selected selected {nullptr, SelectedPoint()};

    //сначала ищем метки, курсоры и т.д., то есть не кривые
    {
        double minDist = qInf();
        const auto allItems = itemList();
        for (auto item: allItems) {
            if (auto selectable = dynamic_cast<Selectable*>(item)) {
                if (isCurve(item)) continue;
                double distx = 0.0;
                double disty = 0.0;
                SelectedPoint point;
                if (selectable->underMouse(pos, &distx, &disty, &point)) {
                    double dist = 0.0;
                    if (distx == qInf()) dist = disty;
                    else if (disty == qInf()) dist = distx;
                    else dist = sqrt(distx*distx+disty*disty);
                    if (!selected.object || dist < minDist) {
                        selected.object = selectable;
                        selected.point = point;
                        minDist = dist;
                    }
                }
            }
        }
    }
    if (!selected.object) {
        double minDist = qInf();
        const auto allItems = itemList();
        for (auto item: allItems) {
            if (auto selectable = dynamic_cast<Selectable*>(item)) {
                if (!isCurve(item)) continue;
                double distx = 0.0;
                double disty = 0.0;
                SelectedPoint point;
                if (selectable->underMouse(pos, &distx, &disty, &point)) {
                    double dist = 0.0;
                    if (distx == qInf()) dist = disty;
                    else if (disty == qInf()) dist = distx;
                    else dist = sqrt(distx*distx+disty*disty);
                    if (!selected.object || dist < minDist) {
                        selected.object = selectable;
                        selected.point = point;
                        minDist = dist;
                    }
                }
            }
        }
    }
    return selected;
}

void QwtPlotImpl::deselect()
{
    const auto allItems = itemList();
    for (auto item: allItems) {
        if (auto selectable = dynamic_cast<Selectable*>(item)) {
            //if (selectable != currentSelected)
                selectable->setSelected(false, SelectedPoint());
        }
    }
}

void QwtPlotImpl::editLegendItem(QwtPlotItem *item)
{
    if (Curve *c = dynamic_cast<Curve *>(item))
        parent->editLegendItem(c);
}

void QwtPlotImpl::deleteCurveFromLegend(QwtPlotItem *item)
{
    if (Curve *c = dynamic_cast<Curve *>(item))
        parent->deleteCurveFromLegend(c);
}

void QwtPlotImpl::fixCurve(QwtPlotItem *curve)
{
    if (Curve *c = dynamic_cast<Curve*>(curve)) {
        c->switchFixed();
        updateLegend();
    }
}

void QwtPlotImpl::dropEvent(QDropEvent *event)
{
    const ChannelsMimeData *myData = qobject_cast<const ChannelsMimeData *>(event->mimeData());
    if (myData) {
        int w = 0;
        if (auto axis = axisWidget(QwtAxis::YLeft); axis->isVisible())
            w = axis->width();
        bool plotOnLeft = event->pos().x() <= w + canvas()->rect().x()+canvas()->rect().width()/2;
        parent->onDropEvent(plotOnLeft, myData->channels);
        event->acceptProposedAction();
    }
    leftOverlay->setVisibility(false);
    rightOverlay->setVisibility(false);
}

void QwtPlotImpl::dragEnterEvent(QDragEnterEvent *event)
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

void QwtPlotImpl::dragMoveEvent(QDragMoveEvent *event)
{
    const ChannelsMimeData *myData = dynamic_cast<const ChannelsMimeData *>(event->mimeData());
    if (myData) {
        //определяем, можем ли построить каналы на левой или правой оси
        //определяем, в какой области мы находимся
        int w = 0;
        if (auto axis = axisWidget(QwtAxis::YLeft); axis->isVisible())
            w = axis->width();
        bool plotOnLeft = event->pos().x() <= w + canvas()->rect().x()+canvas()->rect().width()/2;
//        bool can = true;
//        for (auto c: myData->channels) {
//            if ((plotOnLeft && !canBePlottedOnLeftAxis(c)) || (!plotOnLeft && !canBePlottedOnRightAxis(c))) {
//                can = false;
//                break;
//            }
//        }
        leftOverlay->setVisibility(plotOnLeft);
        rightOverlay->setVisibility(!plotOnLeft);
//        if (can)
            event->acceptProposedAction();
    }
}

void QwtPlotImpl::dragLeaveEvent(QDragLeaveEvent *event)
{DDD;
    Q_UNUSED(event);
    leftOverlay->setVisibility(false);
    rightOverlay->setVisibility(false);
}
