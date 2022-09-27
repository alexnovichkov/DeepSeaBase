#include "qcpplot.h"

#include "plot/zoomstack.h"
#include "plot/plotmodel.h"

QCPAxis::AxisType toQcpAxis(Enums::AxisType type) {
    return static_cast<QCPAxis::AxisType>(type);
}

QCPPlot::QCPPlot(Enums::PlotType type, QWidget *parent) : QCustomPlot(parent), plotType(type)
{
    m = new PlotModel(this);

    //zoom = new ZoomStack(this);
}

void QCPPlot::toggleAutoscale(Enums::AxisType axis, bool toggled)
{
    switch (axis) {
        case Enums::AxisType::atBottom: // x axis
            zoom->horizontalScaleBounds->setFixed(!toggled);
            break;
        case Enums::AxisType::atLeft: // y axis
            zoom->verticalScaleBounds->setFixed(!toggled);
            break;
        case Enums::AxisType::atRight: // y slave axis
            zoom->verticalScaleBoundsSlave->setFixed(!toggled);
            break;
        default:
            break;
        }
}

void QCPPlot::autoscale(Enums::AxisType axis)
{
    zoom->autoscale(axis, type()==Enums::PlotType::Spectrogram);
}

void QCPPlot::setScale(Enums::AxisType id, double min, double max, double step)
{
    setRightScale(id, min, max);

    this->axisRect(0)->axis(toQcpAxis(id))->setRange(min, max);

    // qwt: setAxisScale(toQwtAxisType(id), min, max, step);
}

void QCPPlot::removeLabels()
{
    m->removeLabels();
    replot();
}

void QCPPlot::cycleChannels(bool up)
{
 /*   //есть список кривых, возможно, из разных записей. Необходимо для каждой записи
    //получить список индексов, сдвинуть этот список вверх или вниз,
    //а затем удалить имеющиеся кривые и построить сдвинутые, причем соблюдая порядок
    //отображения.
    //Т.о. :
    //- для каждой кривой определяем запись, номер канала. Если возможно, сдвигаем
    //номер канала, запоминаем канал
    m->cycleChannels(up);

    sergeiMode = true;
    deleteAllCurves();
    for (const auto &c: m->plottedIndexes()) {
        plotChannel(c.ch, c.onLeft, c.fileIndex);
    }
    sergeiMode = false;*/
}

void QCPPlot::deleteAllCurves(bool forceDeleteFixed)
{

}

void QCPPlot::setRightScale(Enums::AxisType id, double min, double max)
{
    Q_UNUSED(id);
    Q_UNUSED(min);
    Q_UNUSED(max);
}

