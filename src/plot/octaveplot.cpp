#include "octaveplot.h"
#include "barcurve.h"
#include "linecurve.h"
#include "logscaleengine.h"
#include "fileformats/filedescriptor.h"
#include "logging.h"

OctavePlot::OctavePlot(QWidget *parent) : Plot(Plot::PlotType::Octave, parent)
{DDD;
    //by default for Octave curves
    xScaleIsLogarithmic = true;
    setAxisScaleEngine(xBottomAxis, new LogScaleEngine(2));
}

