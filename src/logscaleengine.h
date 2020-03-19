#ifndef LOGSCALEENGINE_H
#define LOGSCALEENGINE_H

#include "qwt_scale_engine.h"

class LogScaleEngine : public QwtLogScaleEngine
{
    // QwtScaleEngine interface
public:
    virtual void autoScale(int maxNumSteps, double &x1, double &x2, double &stepSize) const;
    virtual QwtScaleDiv divideScale(double x1, double x2, int maxMajorSteps, int maxMinorSteps, double stepSize) const;
};

#endif // LOGSCALEENGINE_H
