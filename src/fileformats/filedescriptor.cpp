#include "fileformats/filedescriptor.h"
#include "logging.h"
#include "dataholder.h"

double threshold(const QString &name)
{
    QString n = name.toLower();
    if (n=="м/с2" || n=="м/с^2" || n=="м/с*2" || n=="m/s2" || n=="m/s^2" /*|| n=="g"*/)
        return 3.16e-4; //ускорение
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
    _fileName(fileName), _changed(false), _dataChanged(false)
{

}

FileDescriptor::~FileDescriptor()
{
//   qDebug()<<"deleting"<<fileName();
}

void FileDescriptor::populate()
{
    const int count = channelsCount();
    for (int i=0; i<count; ++i) {
        if (!channel(i)->populated()) channel(i)->populate();
    }
}

// возвращает округленную длину записи:
// 3200,0001 -> 3200
// 3199,9999 -> 3200
// 0,49999 -> 0,5
double FileDescriptor::roundedSize() const
{
    double size = 0.0;
    if (channelsCount()>0) {
        if (channel(0)->data()->xValuesFormat() == DataHolder::XValuesUniform) {
            size = samplesCount() * xStep();
            // округление до ближайшего целого
            int rounded = qRound(size);
            if (std::abs(size - double(rounded)) < 0.001) {
                size = rounded;
            }
            else {// пытаемся отловить случаи вроде 0.4999999 или 3.499999
                double size2 = size*2.0;
                int rounded2 = qRound(size2);
                if (std::abs(size2 - double(rounded2)) < 0.0001) {
                    size = double(rounded2)/2.0;
                }
            }
        }
        else {
            if (!channel(0)->populated()) channel(0)->populate();
            size = channel(0)->data()->xValues().last();
        }
    }
    return size;
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

int FileDescriptor::plottedCount() const
{
    int plotted = 0;
    const int count = channelsCount();
    for (int i=0; i<count; ++i) {
        if (channel(i)->plotted()) plotted++;
    }
    return plotted;
}

bool FileDescriptor::hasCurves() const
{
    const int count = channelsCount();
    for (int i=0; i<count; ++i) {
        if (channel(i)->plotted()) return true;
    }
    return false;
}

QString descriptionEntryToString(const DescriptionEntry &entry)
{
    QString result = entry.second;
    if (!entry.first.isEmpty()) return entry.first+"="+result;

    return result;
}

Channel::Channel(Channel *other) :
    _color(QColor()), _plotted(0),
    _populated(other->_populated),
    _data(new DataHolder(*(other->_data)))
{

}

Channel::Channel(Channel &other) :
    _color(QColor()), _plotted(0),
    _populated(other._populated),
    _data(new DataHolder(*(other._data)))
{

}

void Channel::clear()
{
    _data->clear();
    setPopulated(false);
}

void Channel::maybeClearData()
{
    if (samplesCount()>1000000) clear();
}

double Channel::xMin() const
{
    return _data->xMin();
}

double Channel::xMax() const
{
    return _data->xMax();
}

//QString valuesUnit(const QString &first, const QString &second, int unitType)
//{
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
//    return QString();
//}

QString Descriptor::functionTypeDescription(int type)
{
    switch (type) {
        case  1: return "Time Response";
        case  2: return "Auto Spectrum";
        case  3: return "Cross Spectrum";
        case  4: return "FRF";
        case  5: return "Transmissibility";
        case  6: return "Coherence";
        case  7: return "Auto Correlation";
        case  8: return "Cross Correlation";
        case  9: return "PSD";
        case  10: return "ESD";
        case  11: return "Probability Density Function";
        case  12: return "Spectrum";
        case  13: return "Cumulative Frequency Distribution";
        case  14: return "Peaks Valley";
        case  15: return "Stress/Cycles";
        case  16: return "Strain/Cycles";
        case  17: return "Orbit";
        case  18: return "Mode Indicator Function";
        case  19: return "Force Pattern";
        case  20: return "Partial Power";
        case  21: return "Partial Coherence";
        case  22: return "Eigenvalue";
        case  23: return "Eigenvector";
        case  24: return "Shock Response Spectrum";
        case  25: return "Finite Impulse Response Filter";
        case  26: return "Multiple Coherence";
        case  27: return "Order Function";
        default: return "Неизв.";
    }
    return "Неизв.";
}
