#ifndef PLOTINFOOVERLAY_H
#define PLOTINFOOVERLAY_H

#include "qwt_plot_textlabel.h"
#include "qwt_text.h"

class PlotInfoOverlay : public QwtPlotTextLabel
{
public:
    PlotInfoOverlay(QwtPlot *parent);
private:
    QwtPlot *parent;
};

#endif // PLOTINFOOVERLAY_H
