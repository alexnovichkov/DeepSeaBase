#include "channelstable.h"
#include <QtDebug>
#include <QMouseEvent>

ChannelsTable::ChannelsTable(QWidget *parent) : QTableView(parent)
{

}


void ChannelsTable::startDrag(Qt::DropActions supportedActions)
{
    if (enableDragging) QTableView::startDrag(supportedActions);
}


void ChannelsTable::mousePressEvent(QMouseEvent *event)
{
    QModelIndex i = indexAt(event->pos());
    if (!i.isValid())
        enableDragging = false;
    else
        enableDragging = selectionModel()->isSelected(i);
    QTableView::mousePressEvent(event);
}
