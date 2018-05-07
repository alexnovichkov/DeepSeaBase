#include "filedescriptor.h"
#include "logging.h"

double threshold(const QString &name)
{
    if (name=="м/с2" || name=="м/с^2" || name=="м/с*2" || name=="m/s2" || name=="m/s^2") return 3.14e-4;
    if (name=="Па" || name=="Pa" || name=="hPa" || name=="kPa" || name=="MPa"
        || name=="N/m2" || name=="N/mm2") return 2.0e-5;
    if (name=="м/с" || name=="m/s") return 5.0e-8;
    if (name=="м" || name=="m") return 8.0e-14;
    if (name=="V" || name=="В" || name=="мВ" || name=="mV") return 1e-6;
    if (name=="A" || name=="А") return 1e-9;


    return 1.0;
}

FileDescriptor::FileDescriptor(const QString &fileName) :
    _fileName(fileName), _changed(false), _dataChanged(false),
    NumInd(0)
{
    signalHandler = new SignalHandler;
    signalHandler->_changed = false;
}

FileDescriptor::~FileDescriptor()
{
    delete signalHandler;
}

bool FileDescriptor::fileExists() const
{
    return QFileInfo(_fileName).exists();
}

void FileDescriptor::setChanged(bool changed)
{//DD;
    if (_changed == changed) return;
    _changed = changed;
    signalHandler->setChanged(changed || _dataChanged);
}

void FileDescriptor::setDataChanged(bool changed)
{
    if (_dataChanged == changed) return;
    _dataChanged = changed;
    signalHandler->setChanged(changed || _changed);
}

QString descriptionEntryToString(const DescriptionEntry &entry)
{
    QString result = entry.second;
    if (!entry.first.isEmpty()) return entry.first+"="+result;

    return result;
}

QList<int> filterIndexes(FileDescriptor *dfd, const QList<QPair<FileDescriptor *, int> > &channels)
{
    QList<int> result;
    for(int i=0; i<channels.size(); ++i)
        if (channels.at(i).first == dfd) result << channels.at(i).second;
    return result;
}


void SignalHandler::setChanged(bool ch)
{
    if (ch != _changed) emit changed(ch);
    _changed = ch;
}


Model::Model(QObject *parent) : QAbstractItemModel(parent)
{

}

Model::~Model()
{
    qDeleteAll(d);
}

QModelIndex Model::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid()) {
        //channel
        return createIndex(row, column, d[parent.row()]->channel(row));
    }
    else {
        return createIndex(row,column,d[row]);
    }
}

QModelIndex Model::parent(const QModelIndex &child) const
{
    if (!child.isValid()) return QModelIndex();
//    Channel *c = static_cast<Channel*>(child.internalPointer());
//    if (c) return createIndex()
//    return ->descriptor();
}

QModelIndex Model::sibling(int row, int column, const QModelIndex &idx) const
{
}

int Model::rowCount(const QModelIndex &parent) const
{
}

int Model::columnCount(const QModelIndex &parent) const
{
}

bool Model::hasChildren(const QModelIndex &parent) const
{
}

QVariant Model::data(const QModelIndex &index, int role) const
{
}

bool Model::setData(const QModelIndex &index, const QVariant &value, int role)
{
}

QVariant Model::headerData(int section, Qt::Orientation orientation, int role) const
{
}

bool Model::setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
{
}

Qt::ItemFlags Model::flags(const QModelIndex &index) const
{
}

void Model::sort(int column, Qt::SortOrder order)
{
}
