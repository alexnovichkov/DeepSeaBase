#include "filedescriptor.h"
#include "logging.h"
#include "dataholder.h"

double threshold(const QString &name)
{
    QString n = name.toLower();
    if (n=="м/с2" || n=="м/с^2" || n=="м/с*2" || n=="m/s2" || n=="m/s^2" /*|| n=="g"*/) return 3.16e-4; //ускорение
    if (n=="па" || n=="pa" || n=="hpa" || n=="kpa" || n=="mpa"
        || n=="n/m2" || n=="n/mm2") return 2.0e-5; //давление
    if (n=="м/с" || n=="m/s") return 5.0e-8; //скорость
    if (n=="м" || n=="m") return 8.0e-14; //смещение
    if (n=="v" || n=="в" || n=="мв" || n=="mv") return 1e-6; //напряжение
    if (n=="a" || n=="а") return 1e-9; //сила тока
    if (n=="n" || n=="н") return 1.0; // сила


    return 1.0;
}



double convertFactor(const QString &from)
{
    QString n = from.toLower();
    if (n=="g") return 9.81;
    if (n=="n") return 1.0;

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

Channel::Channel(Channel *other) : _checkState(Qt::Unchecked),
    _color(QColor()),
    _data(new DataHolder(*(other->_data)))
{

}

Channel::Channel(Channel &other) : _checkState(Qt::Unchecked),
    _color(QColor()),
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

QString valuesUnit(const QString &first, const QString &second, int unitType)
{
//    if (unitType == DataHolder::UnitsLinear)
//    QString n = name.toLower();
//    if (n=="м/с2" || n=="м/с^2" || n=="м/с*2" || n=="m/s2" || n=="m/s^2" /*|| n=="g"*/) return 3.16e-4; //ускорение
//    if (n=="па" || n=="pa" || n=="hpa" || n=="kpa" || n=="mpa"
//        || n=="n/m2" || n=="n/mm2") return 2.0e-5; //давление
//    if (n=="м/с" || n=="m/s") return 5.0e-8; //скорость
//    if (n=="м" || n=="m") return 8.0e-14; //смещение
//    if (n=="v" || n=="в" || n=="мв" || n=="mv") return 1e-6; //напряжение
//    if (n=="a" || n=="а") return 1e-9; //сила тока
//    if (n=="n" || n=="н") return 1.0; // сила
    return QString();
}
