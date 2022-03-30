#include "channelstable.h"
#include <QMouseEvent>
#include "logging.h"

#include <QMenu>
#include <QInputDialog>

ChannelsTable::ChannelsTable(QWidget *parent) : QTableView(parent)
{DD;
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragOnly);

    editYNameAct = new QAction("Изменить ед. измерения", this);
    connect(editYNameAct,SIGNAL(triggered()), SLOT(editYName()));

    connect(this, &QTableView::customContextMenuRequested, [=](){
        QMenu menu(this);
        int column = currentIndex().column();
        if (column == 1) {
            menu.addAction(editYNameAct);
            menu.exec(QCursor::pos());
        }
        else if (column == 0) {
            menu.addAction(parentActions.value("plot"));
            menu.addAction(parentActions.value("plotRight"));
            menu.addAction(parentActions.value("exportWav"));
            menu.addAction(parentActions.value("moveUp"));
            menu.addAction(parentActions.value("moveDown"));
            menu.addSeparator();
            menu.addAction(parentActions.value("delete"));
            menu.addAction(parentActions.value("deleteBatch"));
            menu.addAction(parentActions.value("copy"));
            menu.addAction(parentActions.value("move"));
            menu.exec(QCursor::pos());
        }
    });
}

void ChannelsTable::addAction(const QString &name, QAction *action)
{
    parentActions.insert(name, action);
}


void ChannelsTable::startDrag(Qt::DropActions supportedActions)
{DD;
    if (enableDragging) QTableView::startDrag(supportedActions);
}


void ChannelsTable::mousePressEvent(QMouseEvent *event)
{DD;
    QModelIndex i = indexAt(event->pos());
    if (!i.isValid())
        enableDragging = false;
    else
        enableDragging = selectionModel()->isSelected(i);
    QTableView::mousePressEvent(event);
}

void ChannelsTable::editYName()
{DD;
    if (!selectionModel()->hasSelection()) return;

    QString newYName = QInputDialog::getText(this, "Новая единица измерения", "Введите новую единицу");
    if (newYName.isEmpty()) return;

    emit yNameChanged(newYName);
}
