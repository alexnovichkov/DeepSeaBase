#ifndef PLOTDOCKFACTORY_H
#define PLOTDOCKFACTORY_H

#include <QToolButton>

#include "DockComponentsFactory.h"
#include "DockAreaWidget.h"
#include "DockAreaTabBar.h"
#include "DockAreaTitleBar.h"
#include "DockWidget.h"
#include "DockWidgetTab.h"
#include "mainwindow.h"

class QLineEdit;

class PlotTitleBar : public ads::CDockAreaTitleBar
{
    Q_OBJECT
Q_SIGNALS:
    void newPlot();
public:
    PlotTitleBar(ads::CDockAreaWidget* parent);
protected:
    void mouseDoubleClickEvent(QMouseEvent * event);
    void contextMenuEvent(QContextMenuEvent * event);
};

class PlotDockTab : public ads::CDockWidgetTab
{
    Q_OBJECT
Q_SIGNALS:
    void newPlot();
    void closePlot(int index);
    void closeOtherPlots(int index);
    void renamePlot(int index);

    void textChanged(const QString &text);
public:
    PlotDockTab(ads::CDockWidget* parent);
protected:
    void mouseDoubleClickEvent(QMouseEvent * event) override;
    void contextMenuEvent(QContextMenuEvent * event) override;
private Q_SLOTS:
    void renamePlot();
    void setPlotName();
private:
    QLineEdit *editor;
};

class PlotDockFactory : public ads::CDockComponentsFactory
{
public:
    PlotDockFactory(MainWindow *receiver)
        : ads::CDockComponentsFactory(), receiver(receiver)
    {}
//    ads::CDockAreaTabBar* createDockAreaTabBar(ads::CDockAreaWidget* DockArea) const override
//    {
//        auto tabBar = new CTabBar(DockArea);

//        return tabBar;
//    }
    virtual ads::CDockWidgetTab* createDockWidgetTab(ads::CDockWidget* DockWidget) const override
    {
        auto tab = new PlotDockTab(DockWidget);
        return tab;
    }
    ads::CDockAreaTitleBar* createDockAreaTitleBar(ads::CDockAreaWidget* DockArea) const override
    {
        auto titleBar = new PlotTitleBar(DockArea);
        QObject::connect(titleBar, &PlotTitleBar::newPlot, receiver, &MainWindow::addPlot);

        auto addTabButton = new QToolButton(DockArea);
        addTabButton->setToolTip(QObject::tr("Добавить график"));
        addTabButton->setIcon(QIcon(":/icons/list-add.png"));
        addTabButton->setShortcut(QKeySequence("Ctrl+T"));
        addTabButton->setAutoRaise(true);
        int index = titleBar->indexOf(titleBar->tabBar());
        titleBar->insertWidget(index + 1, addTabButton);
        QObject::connect(addTabButton, &QToolButton::clicked, receiver, &MainWindow::addPlot);

        return titleBar;
    }
private:
    MainWindow *receiver;
};



#endif // PLOTDOCKFACTORY_H
