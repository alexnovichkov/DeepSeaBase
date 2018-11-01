#include "filedescriptor.h"
#include "logging.h"

double threshold(const QString &name)
{
    QString n = name.toLower();
    if (n=="м/с2" || n=="м/с^2" || n=="м/с*2" || n=="m/s2" || n=="m/s^2") return 3.16e-4;
    if (n=="па" || n=="pa" || n=="hpa" || n=="kpa" || n=="mpa"
        || n=="n/m2" || n=="n/mm2") return 2.0e-5;
    if (n=="м/с" || n=="m/s") return 5.0e-8;
    if (n=="м" || n=="m") return 8.0e-14;
    if (n=="v" || n=="в" || n=="мв" || n=="mv") return 1e-6;
    if (n=="a" || n=="а") return 1e-9;


    return 1.0;
}

FileDescriptor::FileDescriptor(const QString &fileName) :
    _fileName(fileName), _changed(false), _dataChanged(false), _hasGraphs(false)
{

}

FileDescriptor::~FileDescriptor()
{

}

bool FileDescriptor::fileExists() const
{
    return QFileInfo(_fileName).exists();
}

void FileDescriptor::setChanged(bool changed)
{//DD;
    _changed = changed;
}

void FileDescriptor::setDataChanged(bool changed)
{
    _dataChanged = changed;
}

bool FileDescriptor::hasGraphs() const
{
    for (int i=0; i<channelsCount(); ++i) {
        if (channel(i)->checkState() == Qt::Checked) return true;
    }
    return false;
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

Channel::Channel(Channel *other) : _checkState(other->_checkState),
    _color(other->_color),
    _data(new DataHolder(*(other->_data)))
{

}

Channel::Channel(Channel &other) : _checkState(other._checkState),
    _color(other._color),
    _data(new DataHolder(*(other._data)))
{

}

double Channel::xMin() const
{
    return _data->xMin();
}

double Channel::xMax() const
{
    return _data->xMax();
}
