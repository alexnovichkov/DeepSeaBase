#include "fileformats/filedescriptor.h"
#include "logging.h"
#include "dataholder.h"
#include "methods/averaging.h"
#include "algorithms.h"
#include "fileformats/formatfactory.h"

FileDescriptor::FileDescriptor(const QString &fileName) :
    _fileName(fileName)
{DDD;
    //qDebug()<<fileName;
}

FileDescriptor::~FileDescriptor()
{DDD;
    //   qDebug()<<"deleting"<<fileName();
}

bool FileDescriptor::rename(const QString &newName, const QString &newPath)
{DDD;
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

void FileDescriptor::fillPreliminary(const FileDescriptor *d)
{DDD;
    updateDateTimeGUID();
    dataDescription().put("dateTime", d->dataDescription().get("dateTime"));
}

void FileDescriptor::populate()
{DDD;
    const int count = channelsCount();
    for (int i=0; i<count; ++i) {
        if (!channel(i)->populated()) channel(i)->populate();
    }
}

void FileDescriptor::updateDateTimeGUID()
{DDD;
    QDateTime dt = QDateTime::currentDateTime();
    dataDescription().put("fileCreationTime", dt.toString("dd.MM.yyyy hh:mm"));
    dataDescription().put("guid", FileDescriptor::createGUID());
    dataDescription().put("createdBy", "DeepSea Base");
}

bool FileDescriptor::copyTo(const QString &name)
{DDD;
    return QFile::copy(fileName(), name);
}

Descriptor::DataType FileDescriptor::type() const
{DDD;
    const int count = channelsCount();
    if (count == 0) return Descriptor::Unknown;

    Descriptor::DataType t = channel(0)->type();
    for (int i=1; i<count; ++i) {
        if (channel(i)->type() != t) return Descriptor::Unknown;
    }
    return t;
}

QString FileDescriptor::typeDisplay() const
{DDD;
    return Descriptor::functionTypeDescription(type());
}

// возвращает округленную длину записи:
// 3200,0001 -> 3200
// 3199,9999 -> 3200
// 0,49999 -> 0,5
double FileDescriptor::roundedSize() const
{DDD;
    if (channelsCount()==0) return 0.0;

    if (channel(0)->data()->xValuesFormat() == DataHolder::XValuesUniform) {
        double size = samplesCount() * xStep();
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
        return size;
    }
    else {
        return channel(0)->data()->xValue(channel(0)->data()->samplesCount()-1);
    }
}

QDateTime FileDescriptor::dateTime() const
{DDD;
    auto dt = dataDescription().dateTime("dateTime");
    if (dt.isValid()) return dt;

    if (channelsCount()>0) {
        return channel(0)->dataDescription().dateTime("dateTime");
    }
    return QDateTime();
}

bool FileDescriptor::setDateTime(const QDateTime &dt)
{DDD;
    if (dt==dateTime()) return false;
    dataDescription().put("dateTime", dt);
    setChanged(true);
    return true;
}

QDateTime FileDescriptor::fileCreationTime() const
{DDD;
    return dataDescription().dateTime("fileCreationTime");
}

bool FileDescriptor::setFileCreationTime(const QDateTime &dt)
{DDD;
    dataDescription().put("fileCreationTime", dt.toString("dd.MM.yyyy hh:mm:ss"));
    setChanged(true);
    return true;
}

bool FileDescriptor::fileExists() const
{DDD;
    return QFileInfo(_fileName).exists();
}

QVariant FileDescriptor::channelHeader(int column) const
{DDD;
    if (auto c = channel(0))
        return c->channelHeader(column);
    return QVariant();
}

int FileDescriptor::columnsCount() const
{DDD;
    if (auto c = channel(0))
        return c->columnsCount();
    return 7;
}


void FileDescriptor::setChanged(bool changed)
{DDD;
    _changed = changed;
}

int FileDescriptor::plottedCount() const
{DDD;
    int plotted = 0;
    const int count = channelsCount();
    for (int i=0; i<count; ++i) {
        if (channel(i)->plotted()) plotted++;
    }
    return plotted;
}

QVector<int> FileDescriptor::plottedIndexes() const
{DDD;
    QVector<int> plotted;
    const int count = channelsCount();
    for (int i=0; i<count; ++i) {
        if (channel(i)->plotted()) plotted << i;
    }
    return plotted;
}

bool FileDescriptor::isSourceFile() const
{DDD;
    const int count = channelsCount();
    if (count == 0) return false;
    int type = channel(0)->type();
    for (int i=1; i<count; ++i) {
        if (channel(i)->type() != type) {
            return false;
        }
    }

    return (type == Descriptor::TimeResponse);
}

double FileDescriptor::xStep() const
{DDD;
    if (auto c = channel(0))
        return c->data()->xStep();
    return 0.0;
}

void FileDescriptor::setXStep(const double xStep)
{DDD;
    const int count = channelsCount();
    if (count == 0) return;
    bool changed = false;

    for (int i=0; i < count; ++i) {
        Channel *ch = channel(i);
        if (ch->data()->xValuesFormat() == DataHolder::XValuesNonUniform) continue;
        if (ch->data()->xStep()!=xStep) {
            changed = true;
            ch->setXStep(xStep);
        }
    }
    if (changed) setChanged(true);
    write();
}

double FileDescriptor::xBegin() const
{DDD;
    if (auto c = channel(0))
        return c->data()->xMin();
    return 0.0;
}

int FileDescriptor::samplesCount() const
{DDD;
    if (auto c = channel(0))
        return c->data()->samplesCount();
    return 0;
}

QString FileDescriptor::xName() const
{DDD;
    QString result;
    const int count = channelsCount();
    if (count > 0) {
        result = channel(0)->xName();
        for (int i=1; i<count; ++i) {
            if (channel(i)->xName() != result) return "разные";
        }
    }
    return result;
}

bool FileDescriptor::dataTypeEquals(FileDescriptor *other) const
{DDD;
    return (this->type() == other->type());
}

QString FileDescriptor::legend() const
{DDD;
    return dataDescription().get("legend").toString();
}

bool FileDescriptor::setLegend(const QString &s)
{DDD;
    if (s==legend()) return false;
    dataDescription().put("legend", s);
    setChanged(true);
    return true;
}

bool FileDescriptor::canTakeChannelsFrom(FileDescriptor *other) const
{DDD;
    Q_UNUSED(other);
    return true;
}

bool FileDescriptor::canTakeAnyChannels() const
{DDD;
    return true;
}

qint64 FileDescriptor::fileSize() const
{DDD;
    return QFileInfo(fileName()).size();
}

void FileDescriptor::copyChannelsFrom(FileDescriptor *sourceFile, const QVector<int> &indexes)
{DDD;
    QVector<Channel *> source;
    for (auto i: indexes) {
        if (auto c=sourceFile->channel(i)) source << c;
    }
    if (!source.isEmpty())
        copyChannelsFrom(source);
}

bool FileDescriptor::hasCurves() const
{DDD;
    const int count = channelsCount();
    for (int i=0; i<count; ++i) {
        if (channel(i)->plotted()) return true;
    }
    return false;
}

QString FileDescriptor::createGUID()
{DDD;
    QString result = QUuid::createUuid().toString().toUpper();
    if (result.at(24) == '-') result.remove(24,1);
    else result.remove(25,1);
    return result;
}

//QString descriptionEntryToString(const DescriptionEntry &entry)
//{DDD;
//    QString result = entry.second;
//    if (!entry.first.isEmpty()) return entry.first+"="+result;

//    return result;
//}

Channel::Channel(Channel *other) :
    _plotted(0),
    _populated(other->_populated),
    _data(new DataHolder(*(other->_data))),
    _dataDescription(other->_dataDescription)
{DDD;

}

Channel::Channel(Channel &other) :
    _plotted(0),
    _populated(other._populated),
    _data(new DataHolder(*(other._data))),
    _dataDescription(other._dataDescription)
{DDD;

}

QVariant Channel::info(int column, bool edit) const
{DDD;
    Q_UNUSED(edit)
    switch (column) {
        case 0: return dataDescription().get("name");
        case 1: return dataDescription().get("yname");
        case 2: return data()->yValuesFormatString();
        case 3: return dataDescription().get("description");
        case 4: return dataDescription().get("function.name");
        case 5: return data()->blocksCount();
        case 6: return dataDescription().get("correction");
        default: ;
    }
    return QVariant();
}

int Channel::columnsCount() const
{DDDD;
    return 7;
}

QVariant Channel::channelHeader(int column) const
{DDDD;
    switch (column) {
        case 0: return QString("Имя");
        case 1: return QString("Ед.изм.");
        case 2: return QString("Формат");
        case 3: return QString("Описание");
        case 4: return QString("Функция");
        case 5: return QString("Кол-во блоков");
        case 6: return QString("Коррекция");
        default: return QVariant();
    }
    return QVariant();
}

int Channel::octaveType() const
{DDD;
    return dataDescription().get("function.octaveFormat").toInt();
}

void Channel::clear()
{DDD;
    _data->clear();
    setPopulated(false);
}

void Channel::maybeClearData()
{DDD;
    if (data()->samplesCount()>1000000) clear();
}

QString Channel::name() const
{DDDD;
    return dataDescription().get("name").toString();
}

void Channel::setName(const QString &name)
{DDD;
    dataDescription().put("name", name);
}

QString Channel::description() const
{DDDD;
    return _dataDescription.data.value("description").toString();
}

void Channel::setDescription(const QString &description)
{DDD;
    _dataDescription.data.insert("description", description);
}

QString Channel::xName() const
{DDDD;
    return _dataDescription.data.value("xname").toString();
}

QString Channel::yName() const
{DDDD;
    return _dataDescription.data.value("yname").toString();
}

QString Channel::yNameOld() const
{DDDD;
    return yName();
}

QString Channel::zName() const
{DDDD;
    return _dataDescription.data.value("zname").toString();
}

void Channel::setYName(const QString &yName)
{DDD;
    _dataDescription.data.insert("yname", yName);
}

void Channel::setXName(const QString &xName)
{DDD;
    _dataDescription.data.insert("xname", xName);
}

void Channel::setZName(const QString &zName)
{DDD;
    _dataDescription.data.insert("zname", zName);
}

void Channel::setXStep(double xStep)
{DDD;
    _data->setXStep(xStep);
}

QString Channel::legendName() const
{DDDD;
    QStringList l;
    l << name();
    if (!correction().isEmpty()) l << correction();
    if (!descriptor()->legend().isEmpty()) l << descriptor()->legend();

    return l.join(" ");
}

QByteArray Channel::wavData(qint64 pos, qint64 samples, int format)
{DDD;
    QByteArray b;

    //assume that channel is time response
//    if (type() != Descriptor::TimeResponse) return b;

    //assume that channel is populated
//    populate();

    QDataStream s(&b, QIODevice::WriteOnly);
    s.setByteOrder(QDataStream::LittleEndian);
    s.setFloatingPointPrecision(QDataStream::SinglePrecision);

    QVector<double> values = data()->rawYValues(0).mid(pos,samples);

    const double max = qMax(qAbs(data()->yMax()), qAbs(data()->yMin()));
    const double coef = 32768.0 / max;
    for (qint64 i=0; i<values.size(); ++i) {
        if (format == 1) {//format == 1 - PCM
            qint16 v = qint16(coef * values[i]);
            s << v;
        }
        else if (format == 2) {//format = 2 - float
            s << static_cast<float>(values[i]);
        }
    }
    return b;
}

//QString valuesUnit(const QString &first, const QString &second, int unitType)
//{DDD;
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
{DDD;
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
        case  28: return "Transit";
        default : return "Неизв.";
    }
    return "Неизв.";
}



QString stringify(const QVector<int> &vec)
{DDDD;
    QStringList result;
    for (int i:vec) result<<QString::number(i);
    return result.join(",");
}

QDataStream &operator>>(QDataStream &stream, DataDescription &data)
{DDDD;
    stream >> data.data;
    return stream;
}

QDataStream &operator<<(QDataStream &stream, const DataDescription &data)
{DDDD;
    stream << data.data;
    return stream;
}

void DataDescription::put(const QString &key, const QVariant &value)
{DDDD;
    data.insert(key, value);
}

QVariant DataDescription::get(const QString &key) const
{DDDD;
    return data.value(key);
}

QJsonObject DataDescription::toJson() const
{DDD;
    QJsonObject result;
    for (auto i = data.constBegin(); i != data.constEnd(); ++i) {
        const QString key = i.key();
        QVariant val = i.value();
        if (key.contains("dateTime") || key == "fileCreationTime")
            val = val.toDateTime().toString("dd.MM.yyyy hh:mm:ss");
        if (key.contains('.')) {
            auto r = result.value(key.section('.',0,0)).toObject();
            r.insert(key.section('.',1), QJsonValue::fromVariant(val));
            result.insert(key.section('.',0,0), r);
        }
        else
            result.insert(key, QJsonValue::fromVariant(val));
    }
    return result;
}

DataDescription DataDescription::fromJson(const QJsonObject &o)
{DDD;
    DataDescription result;
    for (auto i = o.constBegin(); i!=o.constEnd(); ++i) {
        QString key = i.key();
        QJsonValue val = i.value();
        //qDebug()<<key<<val;
        if (val.isArray()) {
            //qDebug()<<"Array found at"<<key;
            continue;
        }
        else if (val.isObject()) {
            QJsonObject v = val.toObject();
            for (auto j = v.constBegin(); j!=v.constEnd(); ++j) {
                if (j->isArray()) {
                    //qDebug()<<"Array found at"<<j.key();
                    continue;
                }
                else if (j->isObject()) {
                    //qDebug()<<"Object found at"<<j.key();
                    continue;
                }
                QString key1 = key+"."+j.key();
                QVariant v = j->toVariant();
                //дата-время требуют специальной обработки
                if (j.key().contains("dateTime") || j.key() == "fileCreationTime")
                    result.data.insert(key1, dateTimeFromString(v.toString()));
                else
                    result.put(key1, v);
            }
        }
        else {
            QVariant v = val.toVariant();
            //дата-время требуют специальной обработки
            if (key.contains("dateTime") || key=="fileCreationTime")
                result.data.insert(key, dateTimeFromString(v.toString()));
            else
                result.put(key, v);
        }
    }
    return result;
}

QDateTime DataDescription::dateTime(const QString &key) const
{
    //return dateTimeFromString(get(key).toString());
    return get(key).toDateTime();
}

QStringList DataDescription::twoStringDescription() const
{DDDD;
    QStringList result = toStringList("description", true);
    result = result.mid(0,2);
    return result;
}

QStringList DataDescription::toStringList(const QString &filter, bool includeKeys) const
{DDDD;
    QStringList result;
    for (const auto [key, val] : asKeyValueRange(data)) {
        QString s;
        if (filter.isEmpty()) {
            if (includeKeys) s = key+"=";
            result << s+val.toString();
        }
        else if (key.startsWith(filter+".")) {
            if (includeKeys) s = key.mid(filter.length()+1)+"=";
            result << s+val.toString();
        }
    }
    return result;
}

QVariantMap DataDescription::filter(const QString &filter) const
{DDD;
    if (filter.isEmpty()) return data;

    QVariantMap result;
    for (const auto [key, val] : asKeyValueRange(data)) {
        if (key.startsWith(filter+".")) result.insert(key.mid(filter.length()+1), val);
    }

    return result;
}
