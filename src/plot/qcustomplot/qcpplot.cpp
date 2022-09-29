#include "qcpplot.h"
#include "plot/plot.h"
#include "plot/zoomstack.h"
#include "plot/plotmodel.h"
#include "graph2d.h"
#include "plot/canvaseventfilter.h"

QCPAxis::AxisType toQcpAxis(Enums::AxisType type) {
    return static_cast<QCPAxis::AxisType>(type);
}

Enums::AxisType fromQcpAxis(QCPAxis::AxisType type) {
    return static_cast<Enums::AxisType>(type);
}

QCPPlot::QCPPlot(Plot *plot, QWidget *parent) : QCustomPlot(parent), parent(plot)
{
    setInteractions(QCP::iRangeDrag|
                    QCP::iRangeZoom|
                    QCP::iSelectPlottables|
                    QCP::iSelectLegend|
                    QCP::iSelectItems|
                    QCP::iSelectOther);
    for (auto ax: axisRect()->axes()) {
        connect(ax, &QCPAxis::contextMenuRequested, [=](const QPoint &pos, QCPAxis::AxisType type){
            plot->showContextMenu(pos, static_cast<Enums::AxisType>(type));
        });
        //
//        connect(ax, qOverload<const QCPRange&>(&QCPAxis::rangeChanged), [=](const QCPRange &newRange){
//            auto coords = ZoomStack::zoomCoordinates();
//            coords.coords.insert(fromQcpAxis(ax->axisType()), {newRange.lower, newRange.upper});
//            plot->zoom->addZoom(coords, false);
//        });
        connect(ax, &QCPAxis::draggingFinished, [=](const QCPRange &newRange){
            auto coords = ZoomStack::zoomCoordinates();
            for (auto axis: axisRect()->axes()) {
                if (axis == ax) coords.coords.insert(fromQcpAxis(axis->axisType()), {newRange.lower, newRange.upper});
                else coords.coords.insert(fromQcpAxis(axis->axisType()), {axis->range().lower, axis->range().upper});
            }

            plot->zoom->addZoom(coords, true);
        });
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
    QCustomPlot::replot();
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
    if (scale == Enums::AxisScale::Linear)
        a->setScaleType(QCPAxis::stLinear);
    else if (scale == Enums::AxisScale::Logarithmic)
        a->setScaleType(QCPAxis::stLogarithmic);
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
    QString format = fileName.section(".", -1,-1);
    if (!saveRastered(fileName, size.width(), size.height(), -1, format.toLatin1().data(), -1, resolution))
        QMessageBox::critical(this, "Сохранение рисунка", "Не удалось сохранить график");
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

void QCPPlot::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
        case Qt::Key_Backspace: {
            parent->zoom->zoomBack();
            break;
        }
    }
}
