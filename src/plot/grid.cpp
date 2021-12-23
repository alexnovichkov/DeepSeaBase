#include "grid.h"

#include <QColor>

Grid::Grid(QwtPlot *parent) : QwtPlotGrid()
{
    enableXMin(true);
    restore();
    if (parent) attach(parent);
}

void Grid::adaptToPrinter()
{
    setMajorPen(QColor(0,0,0),0,Qt::DotLine);
    setMinorPen(QColor(0,0,0),0,Qt::DotLine);
}

void Grid::adaptToPicture()
{

}

void Grid::restore()
{
    setMajorPen(QColor(150,150,150,150),0,Qt::DotLine);
    setMinorPen(QColor(150,150,150,150),0,Qt::DotLine);
}
