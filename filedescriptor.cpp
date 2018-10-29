#include "filedescriptor.h"
#include "logging.h"

double threshold(const QString &name)
{
    if (name=="м/с2" || name=="м/с^2" || name=="м/с*2" || name=="m/s2" || name=="m/s^2") return 3.16e-4;
    if (name=="Па" || name=="Pa" || name=="hPa" || name=="kPa" || name=="MPa"
        || name=="N/m2" || name=="N/mm2") return 2.0e-5;
    if (name=="м/с" || name=="m/s") return 5.0e-8;
    if (name=="м" || name=="m") return 8.0e-14;
    if (name=="V" || name=="В" || name=="мВ" || name=="mV") return 1e-6;
    if (name=="A" || name=="А") return 1e-9;


    return 1.0;
}

FileDescriptor::FileDescriptor(const QString &fileName) :
    _fileName(fileName), _changed(false), _dataChanged(false)
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
