#include "fileformats/filedescriptor.h"
#include "logging.h"
#include "dataholder.h"
#include "averaging.h"
#include "algorithms.h"

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
    if (from.compare("g", Qt::CaseInsensitive)) return 9.81;
    if (from.compare("n", Qt::CaseInsensitive)) return 1.0;

    return 1.0;
}

FileDescriptor::FileDescriptor(const QString &fileName) :
    _fileName(fileName)
{
    qDebug()<<fileName;
}

FileDescriptor::~FileDescriptor()
{
    //   qDebug()<<"deleting"<<fileName();
}

bool FileDescriptor::rename(const QString &newName, const QString &newPath)
{
    //newName приходит с суффиксом

    if (!newPath.isEmpty()) QDir().mkpath(newPath);
    QString name = newName;
    if (!newPath.isEmpty()) name.prepend(newPath + "/");
    else name.prepend(QFileInfo(fileName()).absolutePath()+"/");

    name.replace("//", "/");
    bool result = QFile::rename(fileName(), name);
    if (result) setFileName(name);

    return result;
}

void FileDescriptor::populate()
{
    const int count = channelsCount();
    for (int i=0; i<count; ++i) {
        if (!channel(i)->populated()) channel(i)->populate();
    }
}

bool FileDescriptor::copyTo(const QString &name)
{
    return QFile::copy(fileName(), name);
}

Descriptor::DataType FileDescriptor::type() const
{
    if (channelsCount() == 0) return Descriptor::Unknown;

    Descriptor::DataType t = channel(0)->type();
    for (int i=1; i<channelsCount(); ++i) {
        if (channel(i)->type() != t) return Descriptor::Unknown;
    }
    return t;
}

QString FileDescriptor::typeDisplay() const
{
    return Descriptor::functionTypeDescription(type());
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
            //if (!channel(0)->populated()) channel(0)->populate();
            QVector<double> vals = channel(0)->data()->xValues();
            if (!vals.isEmpty())
            size = vals.last();
        }
    }
    return size;
}

void FileDescriptor::calculateMean(const QList<Channel *> &channels)
{
    if (channels.isEmpty()) return;

    Channel *firstChannel = channels.constFirst();

    //ищем наименьшее число отсчетов
    int numInd = firstChannel->data()->samplesCount();
    for (int i=1; i<channels.size(); ++i) {
        if (channels.at(i)->data()->samplesCount() < numInd)
            numInd = channels.at(i)->data()->samplesCount();
    }

    // ищем формат данных для нового канала
    // если форматы разные, то формат будет линейный (амплитуды), не логарифмированный
    auto format = firstChannel->data()->yValuesFormat();
    for (int i=1; i<channels.size(); ++i) {
        if (channels.at(i)->data()->yValuesFormat() != format) {
            format = DataHolder::YValuesAmplitudes;
            break;
        }
    }

    int units = firstChannel->data()->yValuesUnits();
    for (int i=1; i<channels.size(); ++i) {
        if (channels.at(i)->data()->yValuesUnits() != units) {
            units = DataHolder::UnitsUnknown;
            break;
        }
    }

    Averaging averaging(Averaging::Linear, channels.size());

    for (Channel *ch: channels) {
        const bool populated = ch->populated();
        if (!populated) ch->populate();
        if (ch->data()->yValuesFormat() == DataHolder::YValuesComplex)
            averaging.average(ch->data()->yValuesComplex(0));
        else
            averaging.average(ch->data()->linears(0));
        if (!populated) ch->clear();
    }

    DataHolder *data = new DataHolder;

    data->setThreshold(firstChannel->data()->threshold());
    data->setYValuesUnits(units);

    if (firstChannel->data()->xValuesFormat()==DataHolder::XValuesUniform) {
        data->setXValues(firstChannel->data()->xMin(), firstChannel->data()->xStep(), numInd);
    }
    else {
        data->setXValues(firstChannel->data()->xValues().mid(0, numInd));
    }
    //усредняем только первый блок
    //TODO: добавить усреднение по всем блокам
    data->setZValues(firstChannel->data()->zMin(), firstChannel->data()->zStep(), 1);

    if (format == DataHolder::YValuesComplex) {
        auto d = averaging.getComplex().mid(0, numInd);
        d.resize(data->samplesCount());
        data->setYValues(d);
    }
    else {//не комплексные
        QVector<double> d = averaging.get().mid(0, numInd);
        d.resize(data->samplesCount());
        if (format == DataHolder::YValuesAmplitudesInDB) {
            data->setYValues(DataHolder::toLog(d, data->threshold(), units), format);
        }
        else
            data->setYValues(d, format);
    }

    QJsonObject descr;
    descr.insert("name", "Среднее");
    QStringList l;
    for (Channel *c: channels) {
        l << c->name();
    }
    descr.insert("description", "Среднее каналов "+l.join(","));
    descr.insert("ynameold", channels.first()->yNameOld());
    descr.insert("yname", channels.first()->yName());
    descr.insert("xname", channels.first()->xName());
    descr.insert("samples",  data->samplesCount());
    descr.insert("blocks", 1);
    descr.insert("zname", channels.first()->zName());

    QJsonObject function;
    function.insert("type", channels.first()->type());
    function.insert("octaveFormat", channels.first()->octaveType());
    function.insert("name", "AVG");
    function.insert("logref", data->threshold());
    QString formatS;
    switch (data->yValuesFormat()) {
        case DataHolder::YValuesComplex: formatS = "complex"; break;
        case DataHolder::YValuesAmplitudes: formatS = "amplitude"; break;
        case DataHolder::YValuesAmplitudesInDB: formatS = "amplitudeDb"; break;
        case DataHolder::YValuesImags: formatS = "imaginary"; break;
        case DataHolder::YValuesPhases: formatS = "phase"; break;
        default: formatS = "real";
    }
    function.insert("format", formatS);
    QString unitsS;
    switch (data->yValuesUnits()) {
        case DataHolder::UnitsUnknown: unitsS = "unknown"; break;
        case DataHolder::UnitsLinear: unitsS = "linear"; break;
        case DataHolder::UnitsQuadratic: unitsS = "quadratic"; break;
        case DataHolder::UnitsDimensionless: unitsS = "dimensionless"; break;
        default: break;
    }
    function.insert("units", unitsS);
    descr.insert("function", function);

    addChannelWithData(data, descr);

    setChanged(true);
    setDataChanged(true);
    write();
    writeRawFile();
}

void FileDescriptor::calculateMovingAvg(const QList<Channel *> &channels, int windowSize)
{
    if (channels.isEmpty()) return;

    for (Channel *c: channels) {
        DataHolder *data = new DataHolder;

        bool populated = c->populated();
        if (!populated) c->populate();

        data->setThreshold(c->data()->threshold());
        data->setYValuesUnits(c->data()->yValuesUnits());

        if (c->data()->xValuesFormat()==DataHolder::XValuesUniform)
            data->setXValues(c->data()->xMin(), c->data()->xStep(), c->data()->samplesCount());
        else
            data->setXValues(c->data()->xValues());

        auto format = c->data()->yValuesFormat();
        if (format == DataHolder::YValuesComplex) {
            //только первый блок
            data->setYValues(movingAverage(c->data()->yValuesComplex(0), windowSize));
        }
        else {
            QVector<double> values = movingAverage(c->data()->linears(0), windowSize);
            if (format == DataHolder::YValuesAmplitudesInDB)
                format = DataHolder::YValuesAmplitudes;
            //только первый блок
            data->setYValues(values, format, 0);
        }
        //TODO: добавить сглаживание по всем блокам
        data->setZValues(c->data()->zMin(), c->data()->zStep(), 1);

        QJsonObject descr;
        descr.insert("name", c->name()+" сглаж.");
        descr.insert("description", "Скользящее среднее канала "+c->name());
        descr.insert("ynameold", c->yNameOld());
        descr.insert("yname", c->yName());
        descr.insert("xname", c->xName());
        descr.insert("samples",  data->samplesCount());
        descr.insert("blocks", 1);
        descr.insert("zname", c->zName());

        QJsonObject function;
        function.insert("type", c->type());
        function.insert("octaveFormat", c->octaveType());
        function.insert("name", "RAVG");
        function.insert("logref", data->threshold());
        QString formatS;
        switch (data->yValuesFormat()) {
            case DataHolder::YValuesComplex: formatS = "complex"; break;
            case DataHolder::YValuesAmplitudes: formatS = "amplitude"; break;
            case DataHolder::YValuesAmplitudesInDB: formatS = "amplitudeDb"; break;
            case DataHolder::YValuesImags: formatS = "imaginary"; break;
            case DataHolder::YValuesPhases: formatS = "phase"; break;
            default: formatS = "real";
        }
        function.insert("format", formatS);
        QString unitsS;
        switch (data->yValuesUnits()) {
            case DataHolder::UnitsUnknown: unitsS = "unknown"; break;
            case DataHolder::UnitsLinear: unitsS = "linear"; break;
            case DataHolder::UnitsQuadratic: unitsS = "quadratic"; break;
            case DataHolder::UnitsDimensionless: unitsS = "dimensionless"; break;
            default: break;
        }
        function.insert("units", unitsS);
        descr.insert("function", function);

        addChannelWithData(data, descr);
        if (!populated) c->clear();
    }

    setChanged(true);
    setDataChanged(true);
    write();
    writeRawFile();
}

void FileDescriptor::calculateThirdOctave(FileDescriptor *source)
{
    for (int i=0; i<source->channelsCount(); ++i) {
        Channel *ch = source->channel(i);

        const bool populated = ch->populated();
        if (!populated) ch->populate();

        DataHolder *data = new DataHolder;
        auto result = thirdOctave(ch->data()->decibels(), ch->data()->xMin(), ch->data()->xStep());

        data->setXValues(result.first);
        data->setThreshold(ch->data()->threshold());
        data->setYValuesUnits(ch->data()->yValuesUnits());
        data->setYValues(result.second, DataHolder::YValuesAmplitudesInDB);

        QJsonObject descr;
        descr.insert("name", ch->name());
        descr.insert("description", ch->description());
        descr.insert("ynameold", ch->yNameOld());
        descr.insert("yname", "дБ");
        descr.insert("xname", "Гц");
        descr.insert("samples",  data->samplesCount());
        descr.insert("blocks", 1);
        descr.insert("zname", ch->zName());

        QJsonObject function;
        function.insert("type", ch->type());
        function.insert("octaveFormat", 3);
        function.insert("name", "OCTF3");
        function.insert("logref", data->threshold());
        function.insert("format", "amplitudeDb");
        QString unitsS;
        switch (data->yValuesUnits()) {
            case DataHolder::UnitsUnknown: unitsS = "unknown"; break;
            case DataHolder::UnitsLinear: unitsS = "linear"; break;
            case DataHolder::UnitsQuadratic: unitsS = "quadratic"; break;
            case DataHolder::UnitsDimensionless: unitsS = "dimensionless"; break;
            default: break;
        }
        function.insert("units", unitsS);
        descr.insert("function", function);

        addChannelWithData(data, descr);

        if (!populated) ch->clear();
    }

    setChanged(true);
    setDataChanged(true);
    write();
    writeRawFile();
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

bool FileDescriptor::isSourceFile() const
{
    if (channelsCount() == 0) return false;
    int type = channel(0)->type();
    for (int i=1; i<channelsCount(); ++i) {
        if (channel(i)->type() != type) {
            return false;
        }
    }

    return (type == Descriptor::TimeResponse);
}

bool FileDescriptor::canTakeChannelsFrom(FileDescriptor *other) const
{
    return (dataTypeEquals(other)
            && qFuzzyIsNull(this->xStep() - other->xStep()));
}

bool FileDescriptor::canTakeAnyChannels() const
{
    return true;
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
    if (data()->samplesCount()>1000000) clear();
}

QString Channel::yNameOld() const
{
    return yName();
}

QByteArray Channel::wavData(qint64 pos, qint64 samples)
{
    QByteArray b;

    //assume that channel is time response
//    if (type() != Descriptor::TimeResponse) return b;

    //assume that channel is populated
//    populate();

    QDataStream s(&b, QIODevice::WriteOnly);
    s.setByteOrder(QDataStream::LittleEndian);
    QVector<double> values = data()->rawYValues(0).mid(pos,samples);

    const double max = qMax(qAbs(data()->yMax()), qAbs(data()->yMin()));
    const double coef = 32768.0 / max;
    for (qint64 i=0; i<values.size(); ++i) {
        qint16 v = qint16(coef * values[i]);
        s << v;
    }
    return b;
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
        case  24: return "SRS";
        case  25: return "FIR Filter";
        case  26: return "Multiple Coherence";
        case  27: return "Order Function";
        default: return "Неизв.";
    }
    return "Неизв.";
}
