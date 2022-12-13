#include "secondaryplot.h"

#include "qcpplot.h"
#include "cursor.h"
#include "curve.h"
#include "dataholder.h"
#include "fileformats/filedescriptor.h"

QColor SecondaryPlot::firstFreeColor()
{
    for (int i=0; i<12; i++) {
        if (!secondaryColors[i].taken) {
            secondaryColors[i].taken = true;
            return secondaryColors[i].color;
        }
    }
    return Qt::gray;
}

void SecondaryPlot::freeColor(QColor color)
{
    for (int i=0; i<12; i++) {
        if (secondaryColors[i].color == color) secondaryColors[i].taken = false;
    }
}

SecondaryPlot::SecondaryPlot(QCPPlot *parent, const QString &title, QCPLayoutGrid *subLayout)
    : QObject(parent), m_parent{parent}
{
    m_axisRect = new QCPAxisRect(parent);

    m_layout = new QCPLayoutGrid;
    subLayout->addElement(subLayout->elementCount(), 0, m_layout);

    m_title = new QCPTextElement(parent, title);
    m_legend = new QCPLegend;
    m_legend->setOrientation(Qt::Horizontal);
    m_legend->setVisible(true);

    m_layout->addElement(0,0, m_title);
    m_layout->addElement(1,0, m_axisRect);
    m_axisRect->insetLayout()->addElement(m_legend, Qt::AlignBottom);

    QList<QCPAxis*> allAxes;
    allAxes << m_axisRect->axes();
    for (QCPAxis *axis: allAxes)
    {
      axis->setLayer("axes");
      axis->grid()->setLayer("grid");
    }
}

void SecondaryPlot::clear()
{
    for (auto &graph: m_graphs) {
        graph->setVisible(false);
    }
}

void SecondaryPlot::update()
{
    if (m_curve) {
        for (const auto &[cursor, graph] : asKeyValueRange(m_graphs)) {
            graph->setData(xData(cursor), yData(cursor), true);
            for (auto axis: m_axisRect->axes()) axis->rescale(true);
            graph->setName(value(cursor));
            graph->setVisible(true);
        }
    }
}

QCPAxis *SecondaryPlot::axis(Enums::AxisType axis) const
{
    return m_axisRect->axis(toQcpAxis(axis));
}

void SecondaryPlot::addCursor(Cursor *cursor)
{
    if (!m_graphs.contains(cursor)) {
        auto graph = m_parent->addGraph(m_axisRect->axis(QCPAxis::atBottom), m_axisRect->axis(QCPAxis::atLeft));
        graph->removeFromLegend();

        double val = qQNaN();
        if (m_curve) {
            auto zIndex = m_curve->channel->data()->nearestZ(cursor->currentPosition().y());
            if (zIndex >= 0) val = m_curve->channel->data()->zValue(zIndex);
        }

        graph->setName(QString("%1 с").arg(val));
        graph->setPen(firstFreeColor());
        cursor->setColor(graph->pen().color().darker());
        m_graphs.insert(cursor, graph);
        if (m_legend) graph->addToLegend(m_legend);
    }
    update();
}

void SecondaryPlot::removeCursor(Cursor *cursor)
{
    if (auto graph = m_graphs.take(cursor)) {
        freeColor(graph->pen().color());
        graph->removeFromLegend(m_legend);
        m_parent->removeGraph(graph);
    }
    update();
}

void SecondaryPlot::setCurve(Curve *curve)
{
    if (m_curve != curve) {
        m_curve = curve;
        update();
    }
}

SpectrePlot::SpectrePlot(QCPPlot *parent, const QString &title, QCPLayoutGrid *subLayout)
    : SecondaryPlot(parent, title, subLayout)
{

}

QVector<double> SpectrePlot::xData(Cursor *cursor) const
{
    Q_UNUSED(cursor);
    return m_curve->channel->data()->xValues();
}

QVector<double> SpectrePlot::yData(Cursor *cursor) const
{
    auto zIndex = m_curve->channel->data()->nearestZ(cursor->currentPosition().y());
    if (zIndex < 0) zIndex = 0;
    return m_curve->channel->data()->yValues(zIndex);
}

QString SpectrePlot::value(Cursor *cursor) const
{
    auto zIndex = m_curve->channel->data()->nearestZ(cursor->currentPosition().y());
    if (zIndex < 0) zIndex = 0;
    return QString("%1 с").arg(m_curve->channel->data()->zValue(zIndex));
}

ThroughPlot::ThroughPlot(QCPPlot *parent, const QString &title, QCPLayoutGrid *subLayout)
    : SecondaryPlot(parent, title, subLayout)
{

}

QVector<double> ThroughPlot::xData(Cursor *cursor) const
{
    Q_UNUSED(cursor);
    return m_curve->channel->data()->zValues();
}

QVector<double> ThroughPlot::yData(Cursor *cursor) const
{
    QVector<double> data(m_curve->channel->data()->blocksCount());
    double xIndex = m_curve->channel->data()->nearest(cursor->currentPosition().x());
    if (xIndex < 0) xIndex = 0;
    for (int i=0; i<m_curve->channel->data()->blocksCount(); ++i)
        data[i] = m_curve->channel->data()->yValue(xIndex, i);
    return data;
}

QString ThroughPlot::value(Cursor *cursor) const
{
    double xIndex = m_curve->channel->data()->nearest(cursor->currentPosition().x());
    if (xIndex < 0) xIndex = 0;
    return QString("%1 Гц").arg(m_curve->channel->data()->xValue(xIndex));
}
