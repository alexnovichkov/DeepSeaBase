#include "channeltablemodel.h"
#include <QApplication>

ChannelTableModel::ChannelTableModel(QObject *parent) : QAbstractTableModel(parent),
    descriptor(0)
{
    channelsCount = 0;

    uFont = qobject_cast<QApplication*>(qApp)->font();
    bFont = uFont;
    bFont.setBold(true);
}

Channel *ChannelTableModel::channel(int index)
{
    if (!descriptor) return 0;

    if (index >= 0 && index < channelsCount)
        return descriptor->channel(index);

    return 0;
}

void ChannelTableModel::clear()
{
    beginResetModel();
    descriptor = 0;
    indexes.clear();
    endResetModel();
}


int ChannelTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return channelsCount;
}

int ChannelTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    if (descriptor) return descriptor->columnsCount();
    return 0;
}

QVariant ChannelTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();

    const int row = index.row();
    const int column = index.column();

    if (row<0 || row>=channelsCount) return QVariant();

    if (!descriptor) return QVariant();

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        return descriptor->channel(row)->info(column);
    }
    else if (role == Qt::ForegroundRole) {
        if (descriptor->channel(row)->checkState() == Qt::Checked) return QColor(Qt::white);
        //return QColor(Qt::black);
    }
    else if (role == Qt::BackgroundRole) {
        if (column == 0)
            return descriptor->channel(row)->color();
        //return QColor(Qt::white);
    }
    else if (role == Qt::FontRole) {
        if (column == 0)
            return (descriptor->channel(row)->checkState() == Qt::Checked ? bFont : uFont);
    }
    if (role == Qt::CheckStateRole) {
        if (column == 0) return descriptor->channel(row)->checkState();
    }
    return QVariant();
}

bool ChannelTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{

}

QVariant ChannelTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical)
        return QAbstractItemModel::headerData(section, orientation, role);
    if (!descriptor)
        return section+1;

    switch (role) {
        case Qt::DisplayRole: return descriptor->channelHeader(section);
            break;
        case Qt::CheckStateRole: {
            if (!descriptor) return QAbstractItemModel::headerData(section, orientation, role);
            const int plotted = descriptor->plottedCount();
            return (plotted==0 ? Qt::Unchecked : (plotted==channelsCount?Qt::Checked:Qt::PartiallyChecked));
        }
        default: return QAbstractItemModel::headerData(section, orientation, role);
    }

    return QAbstractItemModel::headerData(section, orientation, role);
}

//bool ChannelTableModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
//{
//}


Qt::ItemFlags ChannelTableModel::flags(const QModelIndex &index) const
{
    const int col = index.column();
    if (col == 0) {
        return Qt::ItemIsSelectable | Qt::ItemIsEditable
                | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren;
    }
    if (col == 1 || col == 3) return Qt::ItemIsSelectable | Qt::ItemIsEditable
            | Qt::ItemIsEnabled | Qt::ItemNeverHasChildren;
    return Qt::ItemIsSelectable | Qt::ItemNeverHasChildren;

    //return QAbstractTableModel::flags(index);
}
