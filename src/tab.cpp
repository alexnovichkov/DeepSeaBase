#include "tab.h"

#include <QtWidgets>

#include "logging.h"

#include "headerview.h"
#include "filestable.h"
#include "model.h"
#include "channeltablemodel.h"
#include "sortfiltermodel.h"
#include "filterheaderview.h"
#include "fileformats/filedescriptor.h"
#include "filehandler.h"

Tab::Tab(QWidget *parent) : QSplitter(parent)
{DD;
    fileHandler = new FileHandler(this);
}

void Tab::filesSelectionChanged(const QItemSelection &newSelection, const QItemSelection &oldSelection)
{DD;
    Q_UNUSED(oldSelection);
    if (newSelection.isEmpty()) filesTable->selectionModel()->setCurrentIndex(QModelIndex(), QItemSelectionModel::NoUpdate);

    QVector<int> indexes;

    const QModelIndexList list = filesTable->selectionModel()->selection().indexes();
    for (const QModelIndex &i: list) indexes << sortModel->mapToSource(i).row();

    std::sort(indexes.begin(), indexes.end());
    indexes.erase(std::unique(indexes.begin(), indexes.end()), indexes.end());

    model->setSelected(indexes);
    if (indexes.isEmpty()) {
        channelModel->clear();
        filePathLabel->clear();
    }
}

void Tab::channelsSelectionChanged(const QItemSelection &newSelection, const QItemSelection &oldSelection)
{DD;
    Q_UNUSED(oldSelection);
    if (newSelection.isEmpty()) channelsTable->selectionModel()->setCurrentIndex(QModelIndex(), QItemSelectionModel::NoUpdate);

    QVector<int> indexes;

    const QModelIndexList list = channelsTable->selectionModel()->selection().indexes();
    for (const QModelIndex &i: list) indexes << i.row();

    std::sort(indexes.begin(), indexes.end());
    indexes.erase(std::unique(indexes.begin(), indexes.end()), indexes.end());

    channelModel->setSelected(indexes);
}
