#include "timeplot.h"

#include "linecurve.h"
#include "fileformats/filedescriptor.h"
#include "unitsconverter.h"
#include "plotmodel.h"

TimePlot::TimePlot(QWidget *parent) : Plot(Plot::PlotType::Time, parent)
{

}


Curve *TimePlot::createCurve(const QString &legendName, Channel *channel)
{
    return new TimeCurve(legendName, channel);
}


bool TimePlot::canBePlottedOnLeftAxis(Channel *ch) const
{
    //не можем строить временные графики на графике спектров
    if (ch->type() != Descriptor::TimeResponse) return false;

    if (PhysicalUnits::Units::unitsAreSame(ch->xName(), xName) || xName.isEmpty()) { // тип графика совпадает
        if (m->leftCurvesCount()==0 || yLeftName.isEmpty()
            || PhysicalUnits::Units::unitsAreSame(ch->yName(), yLeftName))
            return true;
    }
    return false;
}

bool TimePlot::canBePlottedOnRightAxis(Channel *ch) const
{
    //не можем строить временные графики на графике спектров
    if (ch->type() != Descriptor::TimeResponse) return false;

    if (PhysicalUnits::Units::unitsAreSame(ch->xName(), xName) || xName.isEmpty()) { // тип графика совпадает
        if (m->rightCurvesCount()==0 || yRightName.isEmpty()
            || PhysicalUnits::Units::unitsAreSame(ch->yName(), yRightName))
            return true;
    }
    return false;
}
