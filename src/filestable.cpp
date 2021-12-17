#include "filestable.h"
#include "logging.h"
#include "model.h"

FilesTable::FilesTable(QWidget *parent) : QTreeView(parent)
{DD;
    setAcceptDrops(true);
    setDragDropMode(QAbstractItemView::DropOnly);
    setDropIndicatorShown(false);

    setRootIsDecorated(false);
    setSortingEnabled(true);
    sortByColumn(MODEL_COLUMN_INDEX, Qt::AscendingOrder);

    setContextMenuPolicy(Qt::CustomContextMenu);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
}


