#ifndef TIMEPLOT_H
#define TIMEPLOT_H

#include "plot.h"

class TimePlot : public Plot
{
public:
    TimePlot(QWidget *parent = 0);

    // Plot interface
public:
    virtual Curve *createCurve(const QString &legendName, Channel *channel) override;

    // Plot interface
protected:
    virtual bool canBePlottedOnLeftAxis(Channel *ch) const override;
    virtual bool canBePlottedOnRightAxis(Channel *ch) const override;
};

#endif // TIMEPLOT_H
