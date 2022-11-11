#include "channeltablemodel.h"
#include <QApplication>
#include "logging.h"
#include "fileformats/filedescriptor.h"
#include "channelsmimedata.h"

ChannelTableModel::ChannelTableModel(QObject *parent) : QAbstractTableModel(parent),
    descriptor(0)
{DD;
    channelsCount = 0;

    uFont = qobject_cast<QApplication*>(qApp)->font();
    bFont = uFont;
    bFont.setBold(true);
}

ChannelTableModel::~ChannelTableModel()
{DD;
    descriptor = 0;
    channelsCount = 0;
    indexes.clear();
}

Channel *ChannelTableModel::channel(int index)
{DD;
    if (!descriptor) return nullptr;

    if (index >= 0 && index < channelsCount)
        return descriptor->channel(index);

    return nullptr;
}

QVector<Channel *> ChannelTableModel::selectedChannels() const
{DD;
    QVector<Channel *> result;
    if (descriptor)
        for (int i: indexes) {
            result << descriptor->channel(i);
        }
    return result;
}

void ChannelTableModel::setYName(const QString &yName)
{DD;
    for(int i: indexes) {
        setData(index(i,1), yName, Qt::EditRole);
        //TODO: проверить, нужен ли код
//        if (descriptor->channel(i)->yName() == yName) continue;
//        descriptor->channel(i)->setYName(yName);
//        descriptor->setChanged(true);
//        QModelIndex idx = ;
//        emit dataChanged(idx, idx, QVector<int>()<<Qt::EditRole<<Qt::DisplayRole);
    }
}

void ChannelTableModel::clear()
{DD;
    setDescriptor(nullptr);
}

void ChannelTableModel::setDescriptor(FileDescriptor *dfd)
{DD;
    //if (descriptor == dfd) return;
    beginResetModel();
    descriptor = dfd;
    indexes.clear();
    channelsCount = descriptor?descriptor->channelsCount():0;
    endResetModel();
    emit modelChanged();
}

void ChannelTableModel::setSelected(const QVector<int> &indexes)
{DD;
    this->indexes = indexes;
    emit modelChanged();
}

void ChannelTableModel::onChannelChanged(Channel *ch)
{DD;
    int i = ch->index();
    if (i != -1) {
        emit dataChanged(index(i,0),index(i,columnCount(QModelIndex())));
//        emit headerDataChanged(Qt::Horizontal,0,0);
    }
}

int ChannelTableModel::rowCount(const QModelIndex &parent) const
{DD;
    Q_UNUSED(parent);
    return channelsCount;
}

int ChannelTableModel::columnCount(const QModelIndex &parent) const
{DD;
    Q_UNUSED(parent);
    if (descriptor) return descriptor->columnsCount();
    return 0;
}

QVariant ChannelTableModel::data(const QModelIndex &index, int role) const
{DD;
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
//        case Qt::ForegroundRole:
//            if (descriptor->channel(row)->plotted() && column==0)
//                return QColor(Qt::white);
//            break;
//        case Qt::BackgroundRole:
//            if (descriptor->channel(row)->plotted() && column == 0)
//                return descriptor->channel(row)->color();
//            break;
        case Qt::FontRole:
            if (descriptor->channel(row)->plotted() && column == 0) {
                QFont font;
                font.setBold(true);
                return font;
            }
            break;
//        case Qt::CheckStateRole:
//            if (column == 0)
//                return descriptor->channel(row)->plotted()?Qt::Checked:Qt::Unchecked;
//            break;
        default: break;
    }

    return QVariant();
}

bool ChannelTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{DD;
    if (!index.isValid()) return false;

    const int row = index.row();
    const int column = index.column();

    if (row<0 || row>=channelsCount) return false;

    Channel *ch = descriptor->channel(row);

    bool success = false;

    switch (role) {
        case Qt::EditRole: {
            QString p, descr;
            if (column == 3) {//description
                p = "description";
                descr = "такое описание канала";
            }
            else if (column == 1) {//ед.изм.
                p = "yname";
                descr = "такую единицу измерения";
            }
            else if (column == 0) {//имя
                p = "name";
                descr = "такое название канала";
            }
            if (!p.isEmpty()) {
                if (ch->dataDescription().get(p) != value) {
                    ch->dataDescription().put(p, value);
                    descriptor->setChanged(true);
                    ch->setChanged(true);
                    emit dataChanged(index, index, {Qt::DisplayRole});
                    emit legendsChanged();
                    success = true;
                    emit maybeUpdateChannelProperty(row, descr, p, value.toString());
                }
            }
            break;
        }
    }
    return success;
}

QVariant ChannelTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{DD;
    if (orientation == Qt::Vertical)
        return QAbstractItemModel::headerData(section, orientation, role);

    switch (role) {
        case Qt::DisplayRole:
            if (descriptor) return descriptor->channelHeader(section);
            break;
        default: ;
    }

    return QAbstractItemModel::headerData(section, orientation, role);
}

Qt::ItemFlags ChannelTableModel::flags(const QModelIndex &index) const
{DD;
    const int col = index.column();
    auto result = Qt::ItemIsSelectable | Qt::ItemNeverHasChildren;
    if (col == 0) result |= Qt::ItemIsEditable/*| Qt::ItemIsUserCheckable*/ | Qt::ItemIsEnabled |
                Qt::ItemIsDragEnabled;
    if (col == 1 || col == 3) result |= Qt::ItemIsEditable | Qt::ItemIsEnabled;
    return result;
}


QStringList ChannelTableModel::mimeTypes() const
{DD;
    return QStringList()<<"application/listofchannels";
}

QMimeData *ChannelTableModel::mimeData(const QModelIndexList &indexes) const
{DD;
    if (indexes.isEmpty()) return 0;

    ChannelsMimeData *mimeData = new ChannelsMimeData(selectedChannels());
    return mimeData;
}
