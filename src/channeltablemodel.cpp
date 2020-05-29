#include "channeltablemodel.h"
#include <QApplication>
#include "logging.h"
#include "fileformats/filedescriptor.h"

ChannelTableModel::ChannelTableModel(QObject *parent) : QAbstractTableModel(parent),
    descriptor(0)
{
    channelsCount = 0;

    uFont = qobject_cast<QApplication*>(qApp)->font();
    bFont = uFont;
    bFont.setBold(true);
}

Channel *ChannelTableModel::channel(int index)
{DD;
    if (!descriptor) return 0;

    if (index >= 0 && index < channelsCount)
        return descriptor->channel(index);

    return 0;
}

QVector<int> ChannelTableModel::plotted() const
{DD;
    QVector<int>  list;
    for (int i=0; i<channelsCount; ++i)
        if (descriptor->channel(i)->plotted()>0)
            list << i;
    return list;
}

void ChannelTableModel::setYName(const QString &yName)
{DD;
    foreach(int i, indexes) {
        setData(index(i,1), yName, Qt::EditRole);
//        if (descriptor->channel(i)->yName() == yName) continue;
//        descriptor->channel(i)->setYName(yName);
//        descriptor->setChanged(true);
//        QModelIndex idx = ;
//        emit dataChanged(idx, idx, QVector<int>()<<Qt::EditRole<<Qt::DisplayRole);
    }
}

void ChannelTableModel::clear()
{DD;
    beginResetModel();
    descriptor = 0;
    channelsCount = 0;
    indexes.clear();
    endResetModel();
}

void ChannelTableModel::deleteCurves()
{DD;
    if (!descriptor) return;
    const int count = descriptor->channelsCount();
    for (int i=0; i<count; ++i) {
        if (descriptor->channel(i)->plotted()>0) {
            setData(index(i,0),Qt::Unchecked,Qt::CheckStateRole);
        }
    }
}

void ChannelTableModel::plotChannels(const QVector<int> &toPlot, bool plotOnRight)
{DD;
    if (!descriptor) return;
    QVector<int> to = toPlot;
    if (to.isEmpty()) {
        for (int i=0; i<channelsCount; ++i) {
            to << i;
        }
    }
    foreach (int i,to) {
        if (i<channelsCount) {
            if (descriptor->channel(i)->plotted()==0) {
                descriptor->channel(i)->setPlotted(plotOnRight?2:1);
                setData(index(i,0),Qt::Checked,Qt::CheckStateRole);
            }
        }
    }
}

void ChannelTableModel::setDescriptor(FileDescriptor *dfd)
{DD;
    //if (descriptor == dfd) return;
    beginResetModel();
    descriptor = dfd;
    indexes.clear();
    channelsCount = descriptor->channelsCount();
    endResetModel();
}

void ChannelTableModel::onCurveChanged(Channel *ch)
{
    int i = ch->index();
    if (i != -1) {
        emit dataChanged(index(i,0),index(i,0));
        emit headerDataChanged(Qt::Horizontal,0,0);
    }
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

    switch (role) {
        case Qt::DisplayRole:
            return descriptor->channel(row)->info(column, false);
            break;
        case Qt::EditRole:
            return descriptor->channel(row)->info(column, true);
            break;
        case Qt::ForegroundRole:
            if (descriptor->channel(row)->plotted() && column==0)
                return QColor(Qt::white);
            break;
        case Qt::BackgroundRole:
            if (descriptor->channel(row)->plotted() && column == 0)
                return descriptor->channel(row)->color();
            break;
        case Qt::FontRole:
            if (descriptor->channel(row)->plotted() && column == 0)
                return bFont;
            break;
        case Qt::CheckStateRole:
            if (column == 0)
                return descriptor->channel(row)->plotted()?Qt::Checked:Qt::Unchecked;
            break;
        default: break;
    }

    return QVariant();
}

bool ChannelTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid()) return false;

    const int row = index.row();
    const int column = index.column();

    if (row<0 || row>=channelsCount) return false;

    Channel *ch = descriptor->channel(row);

    bool success = false;

    switch (role) {
        case Qt::EditRole: {
            if (column == 3) {//description
                if (ch->description() != value.toString()) {
                    ch->setDescription(value.toString());
                    descriptor->setChanged(true);
                    ch->setChanged(true);
                    emit dataChanged(index, index, QVector<int>()<<Qt::DisplayRole);
                    success = true;
                }
                emit maybeUpdateChannelDescription(row, value.toString());
            }
            else if (column == 1) {//ед.изм.
                if (value.toString() != ch->yName()) {
                    ch->setYName(value.toString());
                    descriptor->setChanged(true);
                    ch->setChanged(true);
                    emit dataChanged(index, index, QVector<int>()<<Qt::DisplayRole);
                    success = true;
                }
            }
            else if (column == 0) {//имя
                if (ch->name() != value.toString()) {
                    ch->setName(value.toString());
                    descriptor->setChanged(true);
                    ch->setChanged(true);
                    emit dataChanged(index, index, QVector<int>()<<Qt::DisplayRole);
                    emit updateLegends();
                    success = true;
                }
                emit maybeUpdateChannelName(row, value.toString());

            }
            break;
        }
        case Qt::CheckStateRole: {
            if (column == 0) {
                const Qt::CheckState state = Qt::CheckState(value.toInt());

                if (state == Qt::Checked) {
                    if (ch->plotted()==0) ch->setPlotted(1);
                    emit maybePlot(row);
                }
                else if (state == Qt::Unchecked) {
                    emit deleteCurve(row);
                }
                success = true;
            }
            break;
        }
    }
    return success;
}

QVariant ChannelTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical)
        return QAbstractItemModel::headerData(section, orientation, role);


    switch (role) {
        case Qt::DisplayRole:
            if (descriptor) return descriptor->channelHeader(section);
            break;
        case Qt::CheckStateRole: {
            if (!descriptor) return QAbstractItemModel::headerData(section, orientation, role);
            const int plotted = descriptor->plottedCount();
            return (plotted==0 ? Qt::Unchecked : (plotted==channelsCount?Qt::Checked:Qt::PartiallyChecked));
        }
        default: ;
    }

    return QAbstractItemModel::headerData(section, orientation, role);
}

bool ChannelTableModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
{
    if (orientation == Qt::Vertical || !descriptor || section!=0)
        return QAbstractItemModel::setHeaderData(section, orientation, value, role);

    if (role == Qt::CheckStateRole) {
        Qt::CheckState state = Qt::CheckState(value.toInt());
        switch (state) {
            case Qt::Checked: plotChannels(QVector<int>(), false);
                return true;
                break;
            case Qt::Unchecked: deleteCurves();
                return true;
                break;
            default: break;
        }
    }

    return QAbstractItemModel::setHeaderData(section, orientation, value, role);
}


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
}
