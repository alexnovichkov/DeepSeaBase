/**
 * \file tabwidget.cpp
 * QTabWidget with context menu
 *
 * \b Project: Qoobar
 * \author Alex Novichkov
 * \date 11 Aug 2011
 *
 * Copyright (C) 2011  Alex Novichkov
 *
 * This file is part of Qoobar.
 *
 * Qoobar is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Qoobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "tabwidget.h"
#include <QtWidgets>

TabBar::TabBar(QWidget *parent) : QTabBar(parent), index(-1)
{
    setAcceptDrops(true);

    setTabsClosable(true);
    connect(this, SIGNAL(tabCloseRequested(int)), this, SIGNAL(closeTab(int)));
    setSelectionBehaviorOnRemove(QTabBar::SelectPreviousTab);
    setMovable(true);

    editor = new QLineEdit(this);
    editor->hide();

    connect(editor, SIGNAL(editingFinished()), this, SLOT(editTabName()));
}

void TabBar::mouseDoubleClickEvent(QMouseEvent * event)
{
    if (event->button()==Qt::LeftButton) {
        index = tabAt(event->pos());

        if (index==-1) {
            Q_EMIT newTab();
        }
        else {
            QRect rect = tabRect(index);

            if( rect.contains(event->pos()))
            {
                editor->setGeometry(rect.adjusted(2, 2, -2, -2));
                QString oldTabName = tabText(index);
                editor->setText(oldTabName);
                editor->selectAll();
                editor->show();
                editor->setFocus();
            }
        }
    }
    QTabBar::mouseDoubleClickEvent(event);
}

void TabBar::contextMenuEvent(QContextMenuEvent * event)
{
    QMenu menu;
    menu.addAction("Новая вкладка", this, SIGNAL(newTab()), QKeySequence::AddTab);
    QPoint position = event->pos();
    index = tabAt(position);

    if (index != -1) {
        menu.addAction("Закрыть вкладку",this, SLOT(closeTab()), QKeySequence::Close);
        menu.addAction("Закрыть другие вкладки", this, SLOT(closeOtherTabs()));
        menu.addSeparator();
        menu.addAction("Переименовать...", this, SLOT(renameTab()));
    }
    menu.exec(event->globalPos());
}

void TabBar::closeTab()
{
    Q_EMIT closeTab(index);
}

void TabBar::closeOtherTabs()
{
    Q_EMIT closeOtherTabs(index);
}

void TabBar::renameTab()
{
    Q_EMIT renameTab(index);
}

void TabBar::editTabName()
{
    if( index < 0 ) return;
    QString oldText = tabText(index);
    QString text = editor->text();
    editor->hide();
    if (text.isEmpty()) return;

    if (oldText!=text) {
        setTabText(index, text);
        Q_EMIT tabTextChanged(text);
    }
}






TabWidget::TabWidget(QWidget *parent) : QTabWidget(parent)
    , m_tabBar(new TabBar(this))
{
    connect(m_tabBar, SIGNAL(newTab()), this, SIGNAL(newTab()));
    connect(m_tabBar, SIGNAL(closeTab(int)), this, SIGNAL(closeTab(int)));
    connect(m_tabBar, SIGNAL(closeOtherTabs(int)), this, SIGNAL(closeOtherTabs(int)));
    connect(m_tabBar, SIGNAL(renameTab(int)), this, SIGNAL(renameTab(int)));
    connect(m_tabBar, SIGNAL(tabTextChanged(QString)), this, SIGNAL(tabTextChanged(QString)));

    setTabBar(m_tabBar);

    QToolButton *newTabButton = new QToolButton();
    QAction *a = new QAction(QIcon(":/icons/newTab.png"),"Новая вкладка",this);
    connect(a,SIGNAL(triggered()),this,SIGNAL(newTab()));
    newTabButton->setDefaultAction(a);
    newTabButton->setAutoRaise(true);
    setCornerWidget(newTabButton);

    setTabsClosable(true);
    setDocumentMode(true);
    setMovable(true);
}
