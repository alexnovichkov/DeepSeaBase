#include "model.h"

#include <QtCore>
#include <QMessageBox>
#include <QApplication>
#include "logging.h"
#include <QIcon>
#include "fileformats/abstractformatfactory.h"
#include "plot/plottedmodel.h"

Model::Model(QObject *parent) : QAbstractTableModel(parent)
{DD;
    uFont = qobject_cast<QApplication*>(qApp)->font();
    bFont = uFont;
    bFont.setBold(true);
}

F Model::file(int i)
{DD;
    if (i<0 || i>=descriptors.size()) return nullptr;
    return descriptors[i];
}

void Model::addFiles(const QList<F> &files, bool silent)
{DD;
    LOG(INFO) << QString("Добавление %1 файлов").arg(files.size());
    if (files.isEmpty()) return;
    beginInsertRows(QModelIndex(), descriptors.size(), descriptors.size()+files.size()-1);
    descriptors.append(files);
    endInsertRows();
    if (!silent) emit modelChanged();
}

void Model::setSelected(const QVector<int> &indexes)
{DD;
    this->indexes = indexes;
    emit modelChanged();
}

QList<FileDescriptor *> Model::selectedFiles(const QVector<Descriptor::DataType> &types) const
{DD;
    QList<FileDescriptor *> files;
    for (int i: qAsConst(indexes)) {
        if (types.contains(descriptors.at(i)->type()) || types.isEmpty())
            files << descriptors.at(i).get();
    }
    return files;
}

void Model::setChannelProperty(int channel, const QString &property, const QString &value)
{DD;
    for (int i: qAsConst(indexes)) {
        if (Channel *ch = descriptors[i]->channel(channel)) {
            if (ch->dataDescription().get(property) != value) {
                ch->dataDescription().put(property, value);
                ch->setChanged(true);
                descriptors[i]->setChanged(true);
                auto ind = index(i, MODEL_COLUMN_SAVE);
                emit dataChanged(ind, ind, {Qt::DecorationRole});
            }
        }
    }
}

void Model::updateFile(FileDescriptor *file, int column)
{DD;
    int idx;
    if (contains(file, &idx))
        updateFile(idx, column);
}

void Model::updateFile(int idx, int column)
{DD;
    if (column == -1) {
        QModelIndex id1 = index(idx, 0);
        QModelIndex id2 = index(idx, MODEL_COLUMNS_COUNT-1);
        if (id1.isValid() && id2.isValid()) emit dataChanged(id1,id2);
    }
    else {
        QModelIndex id = index(idx, column);
        if (id.isValid()) emit dataChanged(id, id);
    }
}

void Model::clear()
{DD;
    //выделяем все файлы
    indexes.resize(size());
    std::iota(indexes.begin(), indexes.end(), 0);
    //удаляем выделенные файлы
    deleteSelectedFiles();
}

void Model::deleteSelectedFiles()
{DD;
    beginResetModel();
    for (int i = indexes.size()-1; i>=0; --i) {
        int toDelete = indexes.at(i);
        if (toDelete >= 0 && toDelete < descriptors.size()) {
            maybeDeleteFile(toDelete);
            descriptors.removeAt(toDelete);
        }
    }
    indexes.clear();

    endResetModel();
    emit modelChanged();
}

void Model::invalidateCurve(Channel* channel)
{DD;
    int row;
    if (contains(channel->descriptor(), &row)) {
        QModelIndex idx = index(row, MODEL_COLUMN_FILENAME);
        if (idx.isValid())
            emit dataChanged(idx, idx, {Qt::FontRole});
    }
}

void Model::save()
{DD;
    for (int i=0; i<descriptors.size(); ++i) {
        descriptors[i]->write();
        emit dataChanged(index(i,0), index(i,MODEL_COLUMNS_COUNT-1));
    }
}

void Model::discardChanges()
{DD;
    for (auto f: qAsConst(descriptors)) {
        f->setChanged(false);
        f->setDataChanged(false);
        for (int j=0, count = f->channelsCount(); j<count; ++j) {
            f->channel(j)->setChanged(false);
            f->channel(j)->setDataChanged(false);
        }
    }
}

bool Model::changed() const
{DD;
    for (const auto &f: descriptors)
        if (f->changed() || f->dataChanged()) return true;
    return false;
}

void Model::copyToLegend()
{DD;
    const QList<FileDescriptor *> records = selectedFiles();
    for (FileDescriptor *f: records) {
        f->setLegend(QFileInfo(f->fileName()).completeBaseName());
        updateFile(f, MODEL_COLUMN_LEGEND);
    }
    emit legendsChanged();
}

Model::~Model()
{DD;
    //clear();

    for (int i = descriptors.size()-1; i>=0; --i) {
        maybeDeleteFile(i);
    }

    indexes.clear();

    //emit modelChanged();
}

void Model::maybeDeleteFile(int index)
{DD;
    QString name = descriptors[index]->fileName();
    descriptors[index].reset();
    App->maybeDelFile(name);
}

bool Model::contains(const QString &fileName, int *index) const
{DD;
    for (int i=0; i<descriptors.size(); ++i) {
        if (descriptors[i]->fileName() == fileName) {
            if (index) *index = i;
            return true;
        }
    }
    if (index) *index = -1;
    return false;
}

bool Model::contains(const F& file, int *index) const
{DD;
    int ind = descriptors.indexOf(file);
    if (index) *index = ind;

    return ind != -1;
}

bool Model::contains(FileDescriptor *file, int *index) const
{DD;
    for (int i=0; i<descriptors.size(); ++i) {
        if (descriptors[i].get() == file) {
            if (index) *index = i;
            return true;
        }
    }
    if (index) *index = -1;
    return false;
}


int Model::rowCount(const QModelIndex &parent) const
{DD;
    Q_UNUSED(parent);
    return descriptors.size();
}

int Model::columnCount(const QModelIndex &parent) const
{DD;
    Q_UNUSED(parent);
    return MODEL_COLUMNS_COUNT;
}

QVariant Model::data(const QModelIndex &index, int role) const
{DD;
    if (!index.isValid()) return QVariant();

    const int row = index.row();
    const int column = index.column();

    if (row<0 || row>=descriptors.size()) return QVariant();

    const F &d = descriptors[row];
    if (!d) return QVariant();
    switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole: {
            switch (column) {
                case MODEL_COLUMN_INDEX: return row+1;
                case MODEL_COLUMN_FILENAME: return QFileInfo(d->fileName()).completeBaseName();
                case MODEL_COLUMN_DATETIME: return d->dateTime();
                case MODEL_COLUMN_TYPE: return d->typeDisplay();
                case MODEL_COLUMN_SIZE: return d->roundedSize();
                case MODEL_COLUMN_XNAME: return d->xName();
                case MODEL_COLUMN_XSTEP: return d->xStep();
                case MODEL_COLUMN_CHANNELSCOUNT: return d->channelsCount();
                case MODEL_COLUMN_DESCRIPTION: return d->dataDescription().toStringList("description", false).join("; ");
                case MODEL_COLUMN_LEGEND: return d->legend();
                default: return QVariant();
            }
            break;
        }
        case Qt::ToolTipRole: {
            switch (column) {
                //case MODEL_COLUMN_INDEX: return row+1;
                //case MODEL_COLUMN_FILENAME: return QFileInfo(d->fileName()).completeBaseName();
                //case MODEL_COLUMN_DATETIME: return d->dateTime();
                //case MODEL_COLUMN_TYPE: return d->typeDisplay();
                //case MODEL_COLUMN_SIZE: return d->roundedSize();
                //case MODEL_COLUMN_XNAME: return d->xName();
                //case MODEL_COLUMN_XSTEP: return d->xStep();
                //case MODEL_COLUMN_CHANNELSCOUNT: return d->channelsCount();
                case MODEL_COLUMN_DESCRIPTION: return d->dataDescription().toStringList("description", true).join("\n");
                case MODEL_COLUMN_LEGEND: return d->legend();
                default: return QVariant();
            }
            break;
        }
        case Qt::FontRole: {
            if (column == MODEL_COLUMN_FILENAME) {
                if (PlottedModel::instance().plotted(d.get())) {
                    QFont font;
                    font.setBold(true);
                    return font;
                }
            }
            break;
        }
        case Qt::DecorationRole: {
            if (column == MODEL_COLUMN_SAVE) {
                if (d->changed() || d->dataChanged()) return QIcon(":/icons/disk.png");
            }
            if (column == MODEL_COLUMN_FILENAME) return QIcon(d->icon());
            break;
        }
    }

    return QVariant();
}


bool Model::setData(const QModelIndex &index, const QVariant &value, int role)
{DD;
    if (role != Qt::EditRole) return false;
    if (!index.isValid()) return false;

    const int row = index.row();
    const int column = index.column();

    if (row<0 || row>=descriptors.size()) return false;

    const F &d = descriptors[row];
    if (!d->fileExists()) {
//        QMessageBox::warning(this,"Не могу изменить данные","Такого файла уже нет");
        return false;
    }

    switch (column) {
        case MODEL_COLUMN_LEGEND: /*legend*/ {
            if (d->setLegend(value.toString())) {
                emit legendsChanged();
                emit dataChanged(index, index, QVector<int>()<<Qt::DisplayRole);
                return true;
            }
            break;
        }
        case MODEL_COLUMN_XSTEP: /*шаг по оси х*/ {
            if (d->xStep() != value.toDouble()) {
                d->setXStep(value.toDouble());
                emit dataChanged(index, index, QVector<int>()<<Qt::DisplayRole);
                emit plotNeedsUpdate();
//                plot->update();
                return true;
            }
            break;
        }
        case MODEL_COLUMN_DATETIME: /*date*/ {
            QDateTime dt = value.toDateTime();
            if (dt.isValid()) {
                if (dt.date().year()<1950) dt = dt.addYears(100);
                if (d->setDateTime(dt)) {
                    emit dataChanged(index, index, QVector<int>()<<Qt::DisplayRole);
                    return true;
                }
            }
            break;
        }
        default: break;
    }
    return false;
}

QVariant Model::headerData(int section, Qt::Orientation orientation, int role) const
{DD;
    if (orientation == Qt::Vertical)
        return QAbstractItemModel::headerData(section, orientation, role);

    if (role == Qt::DisplayRole) {
        switch (section) {
            case MODEL_COLUMN_INDEX: return "№";
            case MODEL_COLUMN_FILENAME: return "Файл";
            case MODEL_COLUMN_DATETIME: return "Дата";
            case MODEL_COLUMN_TYPE: return "Тип";
            case MODEL_COLUMN_SIZE: return "Размер";
            case MODEL_COLUMN_XNAME: return "Ось X";
            case MODEL_COLUMN_XSTEP: return "Шаг";
            case MODEL_COLUMN_CHANNELSCOUNT: return "Каналы";
            case MODEL_COLUMN_DESCRIPTION: return "Описание";
            case MODEL_COLUMN_LEGEND: return "Легенда";
            default: break;
        }
    }
    return QVariant();
}

Qt::ItemFlags Model::flags(const QModelIndex &index) const
{DD;
    if (!index.isValid()) return QAbstractTableModel::flags(index) | Qt::ItemIsDropEnabled;

    const int col = index.column();
    if (col==MODEL_COLUMN_DATETIME
        || col==MODEL_COLUMN_XSTEP
        || col==MODEL_COLUMN_LEGEND)
        return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;

    return QAbstractTableModel::flags(index);
}


Qt::DropActions Model::supportedDropActions() const
{DD;
    return Qt::CopyAction;
}


bool Model::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const
{DD;
    Q_UNUSED(action);
    Q_UNUSED(row);
    Q_UNUSED(column);
    Q_UNUSED(parent);

    return (data->hasUrls());
}

bool Model::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{DD;
    if (!canDropMimeData(data, action, row, column, parent))
        return false;

    if (action == Qt::IgnoreAction)
        return true;

    if (row != -1) return false;


    QStringList filters = App->formatFactory->allSuffixes(true);
    const QList<QUrl> urlList = data->urls();
    QStringList filesToAdd;
    for (const QUrl &url: urlList) {
        QString s=url.toLocalFile();
        QFileInfo f(s);
        if (f.isDir() || filters.contains(f.suffix().toLower()))
            processDir(s, filesToAdd, true, App->formatFactory->allSuffixes());
    }
    if (filesToAdd.isEmpty()) return false;

    emit needAddFiles(filesToAdd);
    return true;
}
