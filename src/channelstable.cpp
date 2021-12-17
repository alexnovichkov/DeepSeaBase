#include "channelstable.h"
#include <QtDebug>
#include <QMouseEvent>
#include "logging.h"

ChannelsTable::ChannelsTable(QWidget *parent) : QTableView(parent)
{DD;
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragOnly);
//    setSelectionMode(QAbstractItemView::ExtendedSelection);
//    setItemDelegateForColumn(1, new HtmlDelegate);
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
