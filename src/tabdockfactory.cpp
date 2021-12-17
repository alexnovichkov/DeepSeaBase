#include "tabdockfactory.h"
#include "tab.h"
#include "filehandlerdialog.h"
#include "logging.h"

#include <QLineEdit>
#include <QMenu>
#include <QInputDialog>

TitleBar::TitleBar(ads::CDockAreaWidget *parent) : ads::CDockAreaTitleBar(parent)
{
}

void TitleBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button()==Qt::LeftButton)
        Q_EMIT newTab();
    event->accept();
}

void TitleBar::contextMenuEvent(QContextMenuEvent *event)
{
    event->accept();
    QMenu menu;
    menu.addAction("Новая вкладка", this, SIGNAL(newTab()), QKeySequence::AddTab);
    menu.exec(event->globalPos());
}

DockTab::DockTab(ads::CDockWidget *parent) : ads::CDockWidgetTab(parent)
{
    editor = new QLineEdit(this);
    editor->hide();

    connect(editor, SIGNAL(editingFinished()), this, SLOT(setTabName()));
}

void DockTab::mouseDoubleClickEvent(QMouseEvent *event)
{
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

void DockTab::contextMenuEvent(QContextMenuEvent *event)
{
    event->accept();

    QMenu menu;

    menu.addAction("Закрыть вкладку",this, SIGNAL(closeRequested()), QKeySequence::Close);
    menu.addAction("Закрыть другие вкладки", this, SIGNAL(closeOtherTabsRequested()));
    menu.addSeparator();
    menu.addAction("Переименовать...", this, SLOT(renameTab()));
    menu.addSeparator();
    menu.addAction("Отслеживаемые файлы и папки...", this, SLOT(showFileHandler()));

    menu.exec(event->globalPos());
}

void DockTab::renameTab()
{
    QString oldText = text();

    QString newText=QInputDialog::getText(this,
                                             tr("Переименование вкладки"),
                                             tr("Задайте новое имя"),
                                             QLineEdit::Normal,
                                             oldText);

    if (newText != oldText && !newText.isEmpty())
        emit tabTextChanged(newText);
}

void DockTab::showFileHandler()
{
    if (auto t = qobject_cast<Tab *>(this->dockWidget()->widget()))
    {
        FileHandlerDialog dialog(t->fileHandler, this);
        dialog.exec();
    }
}

void DockTab::setTabName()
{
    QString oldText = text();
    QString newText = editor->text();
    editor->hide();

    if (newText != oldText && !newText.isEmpty())
        emit tabTextChanged(newText);
}

TabDockFactory::TabDockFactory(MainWindow *receiver)
    : ads::CDockComponentsFactory(), receiver(receiver)
{}

ads::CDockWidgetTab *TabDockFactory::createDockWidgetTab(ads::CDockWidget *DockWidget) const
{
    auto tab = new DockTab(DockWidget);
    QObject::connect(tab, &DockTab::tabTextChanged, DockWidget, &ads::CDockWidget::setWindowTitle);
    return tab;
}

ads::CDockAreaTitleBar *TabDockFactory::createDockAreaTitleBar(ads::CDockAreaWidget *DockArea) const
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
