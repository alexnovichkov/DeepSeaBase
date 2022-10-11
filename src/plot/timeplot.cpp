#include "timeplot.h"

#include "fileformats/filedescriptor.h"
#include "unitsconverter.h"
#include "plotmodel.h"
#include <QAction>
#include "playpanel.h"
#include "picker.h"
#include "logging.h"
#include "canvaseventfilter.h"
#include "qcustomplot/qcpplot.h"

TimePlot::TimePlot(QWidget *parent) : Plot(Enums::PlotType::Time, parent)
{DDD;
    playerPanel = new PlayPanel(this);
    connect(this, SIGNAL(curvesCountChanged()), playerPanel, SLOT(update()));
    connect(m_plot, SIGNAL(canvasDoubleClicked(QPoint)), playerPanel, SLOT(moveTo(QPoint)));
}

TimePlot::~TimePlot()
{DDD;
    //delete playerPanel;
}

//Curve *TimePlot::createCurve(const QString &legendName, Channel *channel, Enums::AxisType xAxis, Enums::AxisType yAxis)
//{DDD;
//    auto result = new GraphTime(legendName, channel, axis(xAxis), axis(yAxis));
//    result->setXAxis(xAxis);
//    result->setYAxis(yAxis);
//    return result;
//}

QWidget *TimePlot::toolBarWidget()
{DDD;
    return playerPanel;
}

void TimePlot::updateActions(int filesCount, int channelsCount)
{DDD;
    Q_UNUSED(filesCount);
    Q_UNUSED(channelsCount);

    if (playerPanel) {
        const bool hasCurves = curvesCount()>0;
        playerPanel->setEnabled(hasCurves);
    }
}

bool TimePlot::canBePlottedOnLeftAxis(Channel *ch, QString *message) const
{DDD;
    //не можем строить временные графики на графике спектров
    if (ch->type() != Descriptor::TimeResponse) {
        if (message) *message = "Отсутствуют временные данные";
        return false;
    }


    if (PhysicalUnits::Units::unitsAreSame(ch->xName(), xName) || xName.isEmpty()) { // тип графика совпадает
        if (m->leftCurvesCount()==0 || yLeftName.isEmpty()
            || PhysicalUnits::Units::unitsAreSame(ch->yName(), yLeftName))
            return true;
        else if (message) *message = "Единицы по оси Y не совпадают";
    }
    else if (message) *message = "Единицы по оси X не совпадают";
    return false;
}

bool TimePlot::canBePlottedOnRightAxis(Channel *ch, QString *message) const
{DDD;
    //не можем строить временные графики на графике спектров
    if (ch->type() != Descriptor::TimeResponse) {
        if (message) *message = "Отсутствуют временные данные";
        return false;
    }

    if (PhysicalUnits::Units::unitsAreSame(ch->xName(), xName) || xName.isEmpty()) { // тип графика совпадает
        if (m->rightCurvesCount()==0 || yRightName.isEmpty()
            || PhysicalUnits::Units::unitsAreSame(ch->yName(), yRightName))
            return true;
        else if (message) *message = "Единицы по оси Y не совпадают";
    }
    else if (message) *message = "Единицы по оси X не совпадают";
    return false;
}
