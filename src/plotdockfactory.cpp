#include "plotdockfactory.h"

#include <QMenu>
#include <QLineEdit>
#include <QInputDialog>

PlotTitleBar::PlotTitleBar(ads::CDockAreaWidget *parent) : ads::CDockAreaTitleBar(parent)
{

}
void PlotTitleBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button()==Qt::LeftButton)
        Q_EMIT newPlot();
    event->accept();
}

void PlotTitleBar::contextMenuEvent(QContextMenuEvent *event)
{
    event->accept();
    QMenu menu;
    menu.addAction("Новый график", this, SIGNAL(newPlot()), QKeySequence::AddTab);
    menu.exec(event->globalPos());
}

PlotDockTab::PlotDockTab(ads::CDockWidget *parent) : ads::CDockWidgetTab(parent)
{
    editor = new QLineEdit(this);
    editor->hide();

    connect(editor, SIGNAL(editingFinished()), this, SLOT(setPlotName()));
}

void PlotDockTab::mouseDoubleClickEvent(QMouseEvent *event)
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

void PlotDockTab::contextMenuEvent(QContextMenuEvent *event)
{
    event->accept();

    QMenu menu;

    menu.addAction("Закрыть график",this, SIGNAL(closeRequested()), QKeySequence::Close);
    menu.addAction("Закрыть другие графики", this, SIGNAL(closeOtherTabsRequested()));
    menu.addSeparator();
    menu.addAction("Переименовать...", this, SLOT(renamePlot()));

    menu.exec(event->globalPos());
}

void PlotDockTab::renamePlot()
{
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
{
    QString oldText = text();
    QString newText = editor->text();
    editor->hide();
    if (!newText.isEmpty())
        setText(newText);
}
