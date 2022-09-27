#include "octaveplot.h"
#include "logging.h"
#include "plotinterface.h"

OctavePlot::OctavePlot(QWidget *parent) : Plot(Enums::PlotType::Octave, parent)
{DDD;
    //by default for Octave curves
    xScaleIsLogarithmic = true;
    m_plot->setAxisScale(Enums::AxisType::atBottom, Enums::AxisScale::Logarithmic);
}

