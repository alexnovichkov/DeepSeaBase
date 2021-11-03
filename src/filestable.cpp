#include "filestable.h"
#include "logging.h"

FilesTable::FilesTable(QWidget *parent) : QTreeView(parent)
{DD;
    setAcceptDrops(true);
    setDragDropMode(QAbstractItemView::DropOnly);
    setDropIndicatorShown(false);
}


