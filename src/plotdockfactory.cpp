#include "plotdockfactory.h"

#include <QMenu>
#include <QLineEdit>
#include <QInputDialog>
#include "logging.h"

PlotTitleBar::PlotTitleBar(ads::CDockAreaWidget *parent) : ads::CDockAreaTitleBar(parent)
{DDD;

}
void PlotTitleBar::mouseDoubleClickEvent(QMouseEvent *event)
{DDD;
    if (event->button()==Qt::LeftButton)
        Q_EMIT newPlot();
    event->accept();
}

void PlotTitleBar::contextMenuEvent(QContextMenuEvent *event)
{DDD;
    event->accept();
    QMenu menu;
    menu.addAction("Новый график", this, SIGNAL(newPlot()), QKeySequence::AddTab);
    menu.exec(event->globalPos());
}

PlotDockTab::PlotDockTab(ads::CDockWidget *parent) : ads::CDockWidgetTab(parent)
{DDD;
    editor = new QLineEdit(this);
    editor->hide();

    connect(editor, SIGNAL(editingFinished()), this, SLOT(setPlotName()));
}

void PlotDockTab::mouseDoubleClickEvent(QMouseEvent *event)
{DDD;
    if (event->button()==Qt::LeftButton) {
        event->accept();

        QRect rect = this->rect();

        editor->setGeometry(rect.adjusted(2, 2, -2, -2));
        QString oldTabName = text();
        editor->setText(oldTabName);
        editor->selectAll();
        editor->show();
        editor->setFocus();
    }
}

void PlotDockTab::contextMenuEvent(QContextMenuEvent *event)
{DDD;
    event->accept();

    QMenu menu;

    menu.addAction("Закрыть график",this, SIGNAL(closeRequested()), QKeySequence::Close);
    menu.addAction("Закрыть другие графики", this, SIGNAL(closeOtherTabsRequested()));
    menu.addSeparator();
    menu.addAction("Переименовать...", this, SLOT(renamePlot()));

    menu.exec(event->globalPos());
}

void PlotDockTab::renamePlot()
{DDD;
    QString oldTabName = text();

    QString newTabName=QInputDialog::getText(this,
                                             tr("Переименование вкладки"),
                                             tr("Задайте новое имя"),
                                             QLineEdit::Normal,
                                             oldTabName);

    if (!newTabName.isEmpty()) {
        setText(newTabName);
    }
}

void PlotDockTab::setPlotName()
{DDD;
    QString oldText = text();
    QString newText = editor->text();
    editor->hide();
    if (!newText.isEmpty())
        setText(newText);
}

ads::CDockWidgetTab *PlotDockFactory::createDockWidgetTab(ads::CDockWidget *DockWidget) const
{DDD;
    auto tab = new PlotDockTab(DockWidget);
    return tab;
}

ads::CDockAreaTitleBar *PlotDockFactory::createDockAreaTitleBar(ads::CDockAreaWidget *DockArea) const
{DDD;
    auto titleBar = new PlotTitleBar(DockArea);
    QObject::connect(titleBar, &PlotTitleBar::newPlot, receiver, &MainWindow::addPlotTabbed);

    auto addTabButton = new QToolButton(DockArea);
    addTabButton->setToolTip(QObject::tr("Добавить график вкладкой"));
    addTabButton->setIcon(QIcon(":/icons/list-add.png"));
    addTabButton->setShortcut(QKeySequence("Ctrl+T"));
    addTabButton->setAutoRaise(true);
    int index = titleBar->indexOf(titleBar->tabBar());
    titleBar->insertWidget(index + 1, addTabButton);
    QObject::connect(addTabButton, &QToolButton::clicked, receiver, &MainWindow::addPlotTabbed);

    return titleBar;
}
