#ifndef TIMEPLOT_H
#define TIMEPLOT_H

#include "plot.h"
class QAction;
class PlayPanel;

class TimePlot : public Plot
{
public:
    TimePlot(QWidget *parent = 0);
    ~TimePlot();

    // Plot interface
public:
//    virtual Curve *createCurve(const QString &legendName, Channel *channel, Enums::AxisType xAxis, Enums::AxisType yAxis) override;
    virtual QWidget *toolBarWidget() override;
    virtual void updateActions(int filesCount, int channelsCount) override;

protected:
    virtual bool canBePlottedOnLeftAxis(Channel *ch, QString *message = nullptr) const override;
    virtual bool canBePlottedOnRightAxis(Channel *ch, QString *message = nullptr) const override;
private:
    PlayPanel *playerPanel = nullptr;
};

#endif // TIMEPLOT_H
