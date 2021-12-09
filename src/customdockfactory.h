#ifndef CUSTOMDOCKFACTORY_H
#define CUSTOMDOCKFACTORY_H

#include <QToolButton>

#include "DockComponentsFactory.h"
#include "DockAreaWidget.h"
#include "DockAreaTabBar.h"
#include "mainwindow.h"
#include "tabwidget.h"

class CCustomDockFactory : public ads::CDockComponentsFactory
{
public:
    CCustomDockFactory(MainWindow *receiver)
        : ads::CDockComponentsFactory(), receiver(receiver)
    {}
//    ads::CDockAreaTabBar* createDockAreaTabBar(ads::CDockAreaWidget* DockArea) const override
//    {
//        auto tabBar = new CTabBar(DockArea);

//        return tabBar;
//    }
    virtual ads::CDockWidgetTab* createDockWidgetTab(ads::CDockWidget* DockWidget) const override
    {
        auto tab = new DockTab(DockWidget);
        return tab;
    }
    ads::CDockAreaTitleBar* createDockAreaTitleBar(ads::CDockAreaWidget* DockArea) const override
    {
        auto titleBar = new TitleBar(DockArea);
        QObject::connect(titleBar, &TitleBar::newTab, receiver, &MainWindow::createNewTab);

        auto addTabButton = new QToolButton(DockArea);
        addTabButton->setToolTip(QObject::tr("Добавить вкладку"));
        addTabButton->setIcon(QIcon(":/icons/list-add.png"));
        addTabButton->setShortcut(QKeySequence("Ctrl+T"));
        addTabButton->setAutoRaise(true);
        int Index = titleBar->indexOf(titleBar->tabBar());
        titleBar->insertWidget(Index + 1, addTabButton);
        QObject::connect(addTabButton, &QToolButton::clicked, receiver, &MainWindow::createNewTab);

        return titleBar;
    }
private:
    MainWindow *receiver;
};



#endif // CUSTOMDOCKFACTORY_H
