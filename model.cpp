#include "model.h"

#include <QtCore>
#include <QMessageBox>
#include <QApplication>

Model::Model(QObject *parent) : QAbstractTableModel(parent)
{
    uFont = qobject_cast<QApplication*>(qApp)->font();
    bFont = uFont;
    bFont.setBold(true);
}

FileDescriptor *Model::file(int i)
{
    if (i<0 || i>=descriptors.size()) return 0;
    return descriptors[i];
}

FileDescriptor *Model::find(const QString &fileName)
{
    for (int i=0; i<descriptors.size(); ++i) {
        if (descriptors[i]->fileName() == fileName) {
            return descriptors[i];
        }
    }
    return 0;
}

int Model::rowOfFile(FileDescriptor *file) const
{
    return descriptors.indexOf(file,0);
}

void Model::addFiles(const QList<FileDescriptor *> &files)
{
    beginInsertRows(QModelIndex(), descriptors.size(), descriptors.size()+files.size()-1);
    descriptors.append(files);
    endInsertRows();
}

void Model::deleteFiles()
{
    beginResetModel();

    for (int i = indexes.size()-1; i>=0; --i) {
        delete descriptors[indexes.at(i)];
        descriptors.removeAt(indexes.at(i));
    }
    indexes.clear();

    endResetModel();
}

QList<FileDescriptor *> Model::selectedFiles() const
{
    QList<FileDescriptor *> files;
    foreach (int i, indexes)
        files << descriptors.at(i);
    return files;
}

void Model::setDataDescriptor(FileDescriptor *file, const DescriptionList &data)
{
    int row = descriptors.indexOf(file);
    if (row<0) return;

    file->setDataDescriptor(data);
    auto i = index(row, 8);
    emit dataChanged(i, i, QVector<int>()<<Qt::DisplayRole);
}

void Model::setChannelDescription(int channel, const QString &description)
{
    foreach (int i, indexes) {
        if (Channel *ch = descriptors[i]->channel(channel)) {
            if (ch->description() != description) {
                ch->setDescription(description);
                descriptors[i]->setChanged(true);
            }
        }
    }
}

void Model::setChannelName(int channel, const QString &name)
{
    foreach (int i, indexes) {
        if (Channel *ch = descriptors[i]->channel(channel)) {
            if (ch->name() != name) {
                ch->setName(name);
                descriptors[i]->setChanged(true);
            }
        }
    }
}

void Model::updateFile(FileDescriptor *file, int column)
{
    QModelIndex id = modelIndexOfFile(file, column);
    if (id.isValid()) emit dataChanged(id, id);
}

void Model::updateFile(FileDescriptor *file)
{
    QModelIndex id1 = modelIndexOfFile(file, 0);
    QModelIndex id2 = modelIndexOfFile(file, 9);
    if (id1.isValid() && id2.isValid()) emit dataChanged(id1,id2);
}

void Model::clear()
{
    beginResetModel();
    qDeleteAll(descriptors);
    descriptors.clear();
    indexes.clear();
    endResetModel();
}

void Model::invalidateGraphs()
{
    foreach (FileDescriptor *f, descriptors) {
        for(int i=0; i<f->channelsCount(); ++i) {
            f->channel(i)->setCheckState(Qt::Unchecked);
            f->channel(i)->setColor(QColor());
        }
    }
    emit dataChanged(index(0, 1), index(size()-1, 1), QVector<int>()<<Qt::FontRole);
}

void Model::invalidateGraph(FileDescriptor *file, int channel)
{
    file->channel(channel)->setCheckState(Qt::Unchecked);
    file->channel(channel)->setColor(QColor());
    QModelIndex idx = modelIndexOfFile(file, 1);
    emit dataChanged(idx, idx, QVector<int>()<<Qt::FontRole);
}

void Model::save()
{
    foreach (FileDescriptor *f, descriptors) {
        f->write();
        f->writeRawFile();
    }
}

QModelIndex Model::modelIndexOfFile(FileDescriptor *f, int column)
{
    int row = rowOfFile(f);
    if (row<0) return QModelIndex();
    return index(row, column);
}

Model::~Model()
{
    qDeleteAll(descriptors);
}

bool Model::contains(const QString &fileName, int *index) const
{
    for (int i=0; i<descriptors.size(); ++i) {
        if (descriptors[i]->fileName() == fileName) {
            if (index) *index = i;
            return true;
        }
    }
    if (index) *index = -1;
    return false;
}

bool Model::contains(FileDescriptor *file, int *index) const
{
    if (int ind = descriptors.indexOf(file) >= 0) {
        if (index) *index = ind;
        return true;
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
    return 10;
}

QVariant Model::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();

    const int row = index.row();
    const int column = index.column();

    if (row<0 || row>=descriptors.size()) return QVariant();

    FileDescriptor *d = descriptors[row];

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (column) {
            case 0: return row+1;
            case 1: return QFileInfo(d->fileName()).completeBaseName();
            case 2: return d->dateTime();
            case 3: return d->typeDisplay();
            case 4: return d->sizeDisplay();
            case 5: return d->xName();
            case 6: return d->xStep();
            case 7: return d->channelsCount();
            case 8: return d->dataDescriptorAsString();
            case 9: return d->legend();
            default: return QVariant();
        };
    }
    else if (role == Qt::ForegroundRole) {

    }
    else if (role == Qt::FontRole) {
        if (column == 1)
            return (d->hasGraphs() ? bFont : uFont);
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

    FileDescriptor *d = descriptors[row];
    if (!d->fileExists()) {
//        QMessageBox::warning(this,"Не могу изменить данные","Такого файла уже нет");
        return false;
    }

    switch (column) {
        case 9: /*legend*/ {
            if (d->setLegend(value.toString())) {
                emit legendsChanged();
                emit dataChanged(index, index, QVector<int>()<<Qt::DisplayRole);
                return true;
            }
            break;
        }
        case 6: /*шаг по оси х*/ {
            if (d->xStep() != value.toDouble()) {
                d->setXStep(value.toDouble());
                emit dataChanged(index, index, QVector<int>()<<Qt::DisplayRole);
                emit plotNeedsUpdate();
//                plot->update();
                return true;
            }
            break;
        }
        case 2: /*date*/ {
            QDateTime dt = QDateTime::fromString(value.toString(), "dd.MM.yy hh:mm:ss");
            if (dt.isValid()) {
                d->setDateTime(dt);
                emit dataChanged(index, index, QVector<int>()<<Qt::DisplayRole);
                return true;
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
            case 0: return "№";
            case 1: return "Файл";
            case 2: return "Дата";
            case 3: return "Тип";
            case 4: return "Размер";
            case 5: return "Ось X";
            case 6: return "Шаг";
            case 7: return "Каналы";
            case 8: return "Описание";
            case 9: return "Легенда";
            default: break;
        }
    }
    return QVariant();
}

Qt::ItemFlags Model::flags(const QModelIndex &index) const
{
    const int col = index.column();
    if (col==2 || col==6 || col==9) return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;

    return QAbstractTableModel::flags(index);
}
