#ifndef CUSTOMDOCKFACTORY_H
#define CUSTOMDOCKFACTORY_H

#include <QToolButton>

#include "DockComponentsFactory.h"
#include "DockAreaWidget.h"
#include "DockAreaTabBar.h"

class MainWindow;
class QLineEdit;

#include "DockAreaTitleBar.h"
#include "DockWidget.h"
#include "DockWidgetTab.h"

class TitleBar : public ads::CDockAreaTitleBar
{
    Q_OBJECT
Q_SIGNALS:
    void newTab();
public:
    TitleBar(ads::CDockAreaWidget* parent);
protected:
    void mouseDoubleClickEvent(QMouseEvent * event);
    void contextMenuEvent(QContextMenuEvent * event);
};

class DockTab : public ads::CDockWidgetTab
{
    Q_OBJECT
Q_SIGNALS:
    void tabTextChanged(const QString &text);
public:
    DockTab(ads::CDockWidget* parent);
protected:
    void mouseDoubleClickEvent(QMouseEvent * event) override;
    void contextMenuEvent(QContextMenuEvent * event) override;
private Q_SLOTS:
//    void closeTab();
//    void closeOtherTabs();
    void renameTab();
    void showFileHandler();
    void setTabName();
private:
    int index;
    int pressedTab;
    QLineEdit *editor;
};


class TabDockFactory : public ads::CDockComponentsFactory
{
public:
    TabDockFactory(MainWindow *receiver);
//    ads::CDockAreaTabBar* createDockAreaTabBar(ads::CDockAreaWidget* DockArea) const override
//    {
//        auto tabBar = new CTabBar(DockArea);

//        return tabBar;
//    }
    virtual ads::CDockWidgetTab* createDockWidgetTab(ads::CDockWidget* DockWidget) const override;
    ads::CDockAreaTitleBar* createDockAreaTitleBar(ads::CDockAreaWidget* DockArea) const override;
private:
    MainWindow *receiver;
};



#endif // CUSTOMDOCKFACTORY_H
