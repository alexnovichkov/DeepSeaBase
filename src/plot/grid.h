#ifndef GRID_H
#define GRID_H

#include <qwt_plot_grid.h>

class QwtPlot;

class Grid : public QwtPlotGrid
{
public:
    Grid(QwtPlot *parent = 0);
    void adaptToPrinter();
    void adaptToPicture();
    void restore();
};

#endif // GRID_H
