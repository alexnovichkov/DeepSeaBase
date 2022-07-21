#include "timeplot.h"

#include "linecurve.h"
#include "fileformats/filedescriptor.h"
#include "unitsconverter.h"
#include "plotmodel.h"
#include <QAction>
#include "playpanel.h"
#include "picker.h"
#include "logging.h"

TimePlot::TimePlot(QWidget *parent) : Plot(Plot::PlotType::Time, parent)
{
//    m_playAct = new QAction("Открыть панель плеера", this);
//    m_playAct->setIcon(QIcon(":/icons/play.png"));
//    m_playAct->setCheckable(true);
//    //TODO: отвязать плеер от графика, чтобы не было нескольких плееров в одной программе
//    connect(m_playAct, &QAction::triggered, this, &TimePlot::switchPlayerVisibility);

    playerPanel = new PlayPanel(this);
//    playerPanel->setVisible(false);
//    connect(playerPanel,SIGNAL(closeRequested()), m_playAct, SLOT(toggle()));
    connect(this, SIGNAL(curvesCountChanged()), playerPanel, SLOT(update()));
}

TimePlot::~TimePlot()
{
    //delete playerPanel;
}

void TimePlot::switchPlayerVisibility()
{DD;
    playerPanel->switchVisibility();
}

Curve *TimePlot::createCurve(const QString &legendName, Channel *channel)
{
    return new TimeCurve(legendName, channel);
}

QWidget *TimePlot::toolBarWidget()
{
    return playerPanel;
}

void TimePlot::updateActions(int filesCount, int channelsCount)
{
    Q_UNUSED(filesCount);
    Q_UNUSED(channelsCount);

    if (playerPanel) {
        const bool hasCurves = curvesCount()>0;
        playerPanel->setEnabled(hasCurves);
    }
}

bool TimePlot::canBePlottedOnLeftAxis(Channel *ch, QString *message) const
{
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
{
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
