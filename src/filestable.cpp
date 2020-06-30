#include "filestable.h"


FilesTable::FilesTable(QWidget *parent) : QTreeView(parent)
{
    setAcceptDrops(true);
    setDragDropMode(QAbstractItemView::DropOnly);
    setDropIndicatorShown(false);
}


