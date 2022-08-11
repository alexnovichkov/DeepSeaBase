#include "grid.h"

#include <QColor>
#include "logging.h"

Grid::Grid(QwtPlot *parent) : QwtPlotGrid()
{DDD;
    enableXMin(true);
    restore();
    if (parent) attach(parent);
}

void Grid::adaptToPrinter()
{DDD;
    setMajorPen(QColor(0,0,0),0,Qt::DotLine);
    setMinorPen(QColor(0,0,0),0,Qt::DotLine);
}

void Grid::adaptToPicture()
{DDD;

}

void Grid::restore()
{DDD;
    setMajorPen(QColor(150,150,150,150),0,Qt::DotLine);
    setMinorPen(QColor(150,150,150,150),0,Qt::DotLine);
}
