#include "model.h"

#include <QtCore>
#include <QMessageBox>
#include <QApplication>
#include "logging.h"
#include <QIcon>
#include "fileformats/formatfactory.h"
#include "mainwindow.h"

Model::Model(QObject *parent) : QAbstractTableModel(parent)
{DD;
    uFont = qobject_cast<QApplication*>(qApp)->font();
    bFont = uFont;
    bFont.setBold(true);
}

//const F & Model::file(int i) const
//{DD;
//    if (i<0 || i>=descriptors.size()) return F();
//    return descriptors[i];
//}

F Model::file(int i)
{
    if (i<0 || i>=descriptors.size()) return F();
    return descriptors[i];
}

void Model::addFiles(const QList<F> &files)
{DD;
    if (files.isEmpty()) return;
    beginInsertRows(QModelIndex(), descriptors.size(), descriptors.size()+files.size()-1);
    descriptors.append(files);
    endInsertRows();
    emit modelChanged();
}

void Model::deleteFiles()
{DD;
    beginResetModel();

    for (int i = indexes.size()-1; i>=0; --i) {
        int toDelete = indexes.at(i);
        if (toDelete >= 0 && toDelete < descriptors.size()) {
            descriptors.removeAt(toDelete);
        }
    }
    indexes.clear();

    endResetModel();
    emit modelChanged();
}

void Model::setSelected(const QList<int> &indexes)
{DD;
    this->indexes = indexes;
    emit modelChanged();
}

QList<FileDescriptor *> Model::selectedFiles(Descriptor::DataType type) const
{DD;
    QList<FileDescriptor *> files;
    for (int i: indexes) {
        if (descriptors.at(i)->type() == type)
            files << descriptors.at(i).get();
    }
    return files;
}

QList<FileDescriptor *> Model::selectedFiles() const
{DD;
    QList<FileDescriptor *> files;
    for (int i: indexes) {
        files << descriptors.at(i).get();
    }
    return files;
}

QList<FileDescriptor *> Model::selectedFiles(const QVector<Descriptor::DataType> &types) const
{DD;
    QList<FileDescriptor *> files;
    for (int i: indexes) {
        if (types.contains(descriptors.at(i)->type()))
            files << descriptors.at(i).get();
    }
    return files;
}

//void Model::setDataDescription(FileDescriptor *file, const DataDescription &data)
//{DD;
//    int row;
//    if (!contains(file, &row)) return;

//    file->setDataDescription(data);
//    auto i = index(row, MODEL_COLUMN_DESCRIPTION);
//    emit dataChanged(i, i, QVector<int>()<<Qt::DisplayRole);
//    i = index(row, MODEL_COLUMN_FILENAME);
//    emit dataChanged(i, i, QVector<int>()<<Qt::DecorationRole);
//}
void Model::setDataDescription(int selectionIndex, const DataDescription &data)
{DD;
    int row = indexes.at(selectionIndex);

    descriptors[row]->setDataDescription(data);
    auto i = index(row, MODEL_COLUMN_DESCRIPTION);
    emit dataChanged(i, i, QVector<int>()<<Qt::DisplayRole);
    i = index(row, MODEL_COLUMN_FILENAME);
    emit dataChanged(i, i, QVector<int>()<<Qt::DecorationRole);
}

void Model::setChannelDescription(int channel, const QString &description)
{DD;
    for (int i: indexes) {
        if (Channel *ch = descriptors[i]->channel(channel)) {
            if (ch->description() != description) {
                ch->setDescription(description);
                descriptors[i]->setChanged(true);
                emit dataChanged(index(i, MODEL_COLUMN_SAVE),
                                 index(i, MODEL_COLUMN_SAVE),
                                 QVector<int>()<<Qt::DecorationRole);
            }
        }
    }
}

void Model::setChannelName(int channel, const QString &name)
{DD;
    for (int i: indexes) {
        if (Channel *ch = descriptors[i]->channel(channel)) {
            if (ch->name() != name) {
                ch->setName(name);
                descriptors[i]->setChanged(true);
                emit dataChanged(index(i, MODEL_COLUMN_SAVE),
                                 index(i, MODEL_COLUMN_SAVE),
                                 QVector<int>()<<Qt::DecorationRole);
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
    beginResetModel();
    //for (const auto &f: descriptors) qDebug()<<f.use_count()<<f->fileName();
    descriptors.clear();
    indexes.clear();
    endResetModel();
    emit modelChanged();
}

void Model::invalidateCurve(Channel* channel)
{DD;
    channel->setPlotted(0);
    channel->setColor(QColor());

    int row;
    if (contains(channel->descriptor(), &row)) {
        QModelIndex idx = index(row, MODEL_COLUMN_FILENAME);
        if (idx.isValid())
            emit dataChanged(idx, idx, QVector<int>()<<Qt::FontRole);
    }
}

void Model::save()
{DD;
    for (int i=0; i<descriptors.size(); ++i) {
        auto f = descriptors[i];
        f->write();
        emit dataChanged(index(i,0), index(i,MODEL_COLUMNS_COUNT-1));
    }
}

void Model::discardChanges()
{DD;
    for (auto f: descriptors) {
        f->setChanged(false);
        f->setDataChanged(false);
        for (int j=0; j<f->channelsCount(); ++j) {
            f->channel(j)->setChanged(false);
            f->channel(j)->setDataChanged(false);
        }
    }
}

bool Model::changed() const
{DD;
    return std::any_of(descriptors.cbegin(), descriptors.cend(), [](const F &d){
        return d->changed() || d->dataChanged();
    });
//    for (auto & d: descriptors) {
//        if (d->changed() || d->dataChanged()) return true;
//    }
//    return false;
}

Model::~Model()
{DD;
    clear();
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
{
    Q_UNUSED(parent);
    return descriptors.size();
}

int Model::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return MODEL_COLUMNS_COUNT;
}

QVariant Model::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();

    const int row = index.row();
    const int column = index.column();

    if (row<0 || row>=descriptors.size()) return QVariant();

    const F &d = descriptors[row];
    if (!d) return QVariant();

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (column) {
            case MODEL_COLUMN_INDEX: return row+1;
            case MODEL_COLUMN_FILENAME: return QFileInfo(d->fileName()).completeBaseName();
            case MODEL_COLUMN_DATETIME: return d->dateTime();
            case MODEL_COLUMN_TYPE: return d->typeDisplay();
            case MODEL_COLUMN_SIZE: return d->roundedSize();
            case MODEL_COLUMN_XNAME: return d->xName();
            case MODEL_COLUMN_XSTEP: return d->xStep();
            case MODEL_COLUMN_CHANNELSCOUNT: return d->channelsCount();
            case MODEL_COLUMN_DESCRIPTION: return d->dataDescription().toStringList("description").join(";");
            case MODEL_COLUMN_LEGEND: return d->legend();
            default: return QVariant();
        }
    }
    else if (role == Qt::ForegroundRole) {

    }
    else if (role == Qt::FontRole) {
        if (column == MODEL_COLUMN_FILENAME) {
            if (d->hasCurves()) {
                QFont font;// = qApp->font(); //qDebug()<<font;
                font.setBold(true);//qDebug()<<font;
                return font;
            }
        }
        QVariant();
    }
    else if (role == Qt::DecorationRole) {
        if (column == MODEL_COLUMN_SAVE) {
            return (d->changed() || d->dataChanged())? QIcon(":/icons/disk.png") : QVariant();
        }
    }
    return QVariant();
}


bool Model::setData(const QModelIndex &index, const QVariant &value, int role)
{
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
{
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
{
    if (!index.isValid()) return QAbstractTableModel::flags(index) | Qt::ItemIsDropEnabled;

    const int col = index.column();
    if (col==MODEL_COLUMN_DATETIME
        || col==MODEL_COLUMN_XSTEP
        || col==MODEL_COLUMN_LEGEND)
        return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;

    return QAbstractTableModel::flags(index);
}


Qt::DropActions Model::supportedDropActions() const
{
    return Qt::CopyAction;
}


bool Model::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(action);
    Q_UNUSED(row);
    Q_UNUSED(column);
    Q_UNUSED(parent);

    if (data->hasUrls()) return true;
    return false;
}

bool Model::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    if (!canDropMimeData(data, action, row, column, parent))
        return false;

    if (action == Qt::IgnoreAction)
        return true;

    if (row != -1) return false;


    QStringList filters = FormatFactory::allSuffixes(true);
    QList<QUrl> urlList = data->urls();
    QStringList filesToAdd;
    Q_FOREACH (const QUrl &url, urlList) {
        QString s=url.toLocalFile();
        QFileInfo f(s);
        if (f.isDir() || filters.contains(f.suffix().toLower()))
            processDir(s, filesToAdd, true);
    }
    if (filesToAdd.isEmpty()) return false;

    emit needAddFiles(filesToAdd);
    return true;
}
