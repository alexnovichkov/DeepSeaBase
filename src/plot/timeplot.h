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
    virtual Curve *createCurve(const QString &legendName, Channel *channel) override;
    QAction *m_playAct = nullptr;
    virtual QAction *playAct() override {return m_playAct;}

    // Plot interface
protected:
    virtual bool canBePlottedOnLeftAxis(Channel *ch, QString *message = nullptr) const override;
    virtual bool canBePlottedOnRightAxis(Channel *ch, QString *message = nullptr) const override;
private slots:
    void switchPlayerVisibility();
private:
    PlayPanel *playerPanel = nullptr;
};

#endif // TIMEPLOT_H
