#include "fileformats/filedescriptor.h"
#include "logging.h"
#include "dataholder.h"
#include "methods/averaging.h"
#include "algorithms.h"
#include "fileformats/formatfactory.h"

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

void FileDescriptor::fillPreliminary(const FileDescriptor *d)
{
    updateDateTimeGUID();
    dataDescription().put("dateTime", d->dataDescription().get("dateTime"));
}

void FileDescriptor::populate()
{
    const int count = channelsCount();
    for (int i=0; i<count; ++i) {
        if (!channel(i)->populated()) channel(i)->populate();
    }
}

void FileDescriptor::updateDateTimeGUID()
{
    QDateTime dt = QDateTime::currentDateTime();
    dataDescription().put("fileCreationTime", dt.toString("dd.MM.yyyy hh:mm"));
    dataDescription().put("guid", FileDescriptor::createGUID());
    dataDescription().put("createdBy", "DeepSea Base");
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
{
    return dataDescription().get("dateTime").toDateTime();
}

bool FileDescriptor::setDateTime(const QDateTime &dt)
{
    if (dt==dateTime()) return false;
    dataDescription().put("dateTime", dt);
    setChanged(true);
    return true;
}

QDateTime FileDescriptor::fileCreationTime() const
{
    QString dt = dataDescription().get("fileCreationTime").toString();
    return QDateTime::fromString(dt, "dd.MM.yyyy hh:mm");
}

bool FileDescriptor::setFileCreationTime(const QDateTime &dt)
{
    dataDescription().put("fileCreationTime", dt.toString("dd.MM.yyyy hh:mm"));
    setChanged(true);
    return true;
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

    auto units = firstChannel->data()->yValuesUnits();
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

    DataDescription descr = channels.first()->dataDescription();
    descr.put("name", "Среднее");
    QStringList l;
    for (Channel *c: channels) {
        l << c->name();
    }
    descr.put("description", QString("Среднее каналов ")+l.join(","));
    descr.put("samples",  data->samplesCount());
    descr.put("blocks", 1);
    descr.put("function.name", "AVG");
    descr.put("function.logref", data->threshold());
    descr.put("function.format", DataHolder::formatToString(data->yValuesFormat()));
    descr.put("function.logscale", DataHolder::unitsToString(data->yValuesUnits()));

    addChannelWithData(data, descr);

    setChanged(true);
    setDataChanged(true);
    write();
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

        DataDescription descr = c->dataDescription();
        descr.put("name", c->name()+" сглаж.");
        descr.put("description", "Скользящее среднее канала "+c->name());
        descr.put("samples",  data->samplesCount());
        descr.put("blocks", 1);
        descr.put("function.name", "RAVG");

        addChannelWithData(data, descr);
        if (!populated) c->clear();
    }

    setChanged(true);
    setDataChanged(true);
    write();
}

void FileDescriptor::calculateThirdOctave(FileDescriptor *source)
{
    for (int i=0; i<source->channelsCount(); ++i) {
        Channel *ch = source->channel(i);

        const bool populated = ch->populated();
        if (!populated) ch->populate();

        DataHolder *data = new DataHolder;
        auto result = thirdOctave(ch->data()->decibels(0), ch->data()->xMin(), ch->data()->xStep());

        data->setXValues(result.first);
        data->setThreshold(ch->data()->threshold());
        data->setYValuesUnits(ch->data()->yValuesUnits());
        data->setYValues(result.second, DataHolder::YValuesAmplitudesInDB);

        DataDescription descr = ch->dataDescription();
        descr.put("yname", "дБ");
        descr.put("xname", "Гц");
        descr.put("samples",  data->samplesCount());
        descr.put("blocks", 1);
        descr.put("function.octaveFormat", 3);
        descr.put("function.name", "OCTF3");
        descr.put("function.logref", data->threshold());
        descr.put("function.format", "amplitudeDb");
        descr.put("function.logscale", DataHolder::unitsToString(data->yValuesUnits()));

        addChannelWithData(data, descr);

        if (!populated) ch->clear();
    }

    setChanged(true);
    setDataChanged(true);
    write();
}

QString FileDescriptor::saveTimeSegment(double from, double to)
{
    // 0 проверяем, чтобы этот файл имел тип временных данных
    if (type() != Descriptor::TimeResponse) return "";
    // и имел данные
    if (channelsCount() == 0) return "";

    // 1 создаем уникальное имя файла по параметрам from и to
    QString fromString, toString;
    getUniqueFromToValues(fromString, toString, from, to);
    QString suffix = QString("_%1s_%2s").arg(fromString).arg(toString);

    QString ext = QFileInfo(fileName()).suffix();
    QString newFileName = createUniqueFileName("", fileName(), suffix, ext, false);

    // 2 создаем новый файл
    auto newFile = FormatFactory::createDescriptor(newFileName);

    newFile->setDataDescription(this->dataDescription());
    newFile->dataDescription().put("source.file", fileName());
    newFile->dataDescription().put("source.guid", dataDescription().get("guid"));
    newFile->dataDescription().put("source.dateTime", dataDescription().get("dateTime"));
    newFile->fillPreliminary(this);

    // 3 ищем границы данных по параметрам from и to
    Channel *ch = channel(0);
    int sampleStart = qRound((from - ch->data()->xMin())/ch->data()->xStep());
    if (sampleStart<0) sampleStart = 0;
    int sampleEnd = qRound((to - ch->data()->xMin())/ch->data()->xStep());
    if (sampleEnd >= ch->data()->samplesCount()) sampleEnd = ch->data()->samplesCount() - 1;

    // 4 сохраняем файл

    for (int i=0; i<channelsCount(); ++i) {
        Channel *c = channel(i);
        bool wasPopulated = c->populated();
        if (!wasPopulated) c->populate();

        DataHolder *data = new DataHolder();
        data->setSegment(*(c->data()), sampleStart, sampleEnd);

        DataDescription descr = c->dataDescription();
        descr.put("name", c->name()+" вырезка");
        descr.put("description", QString("Вырезка %1s-%2s").arg(fromString).arg(toString));
        descr.put("samples",  data->samplesCount());
        descr.put("blocks", data->blocksCount());
        descr.put("function.name", "SECTION");
        descr.put("function.logref", data->threshold());
        newFile->addChannelWithData(data, descr);

        if (!wasPopulated)
            c->clear();
    }

    newFile->setChanged(true);
    newFile->setDataChanged(true);
    newFile->write();
    delete newFile;

    // 5 возвращаем имя нового файла
    return newFileName;
}

bool FileDescriptor::fileExists() const
{
    return QFileInfo(_fileName).exists();
}

QVariant FileDescriptor::channelHeader(int column) const
{
    if (channelsCount()==0) return QVariant();
    return channel(0)->channelHeader(column);
}

int FileDescriptor::columnsCount() const
{
    if (channelsCount()==0) return 7;
    return channel(0)->columnsCount();
}


void FileDescriptor::setChanged(bool changed)
{//DD;
    _changed = changed;
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

double FileDescriptor::xStep() const
{
    if (channelsCount()==0) return 0.0;
    return channel(0)->data()->xStep();
}

void FileDescriptor::setXStep(const double xStep)
{
    if (channelsCount() == 0) return;
    bool changed = false;

    for (int i=0; i < channelsCount(); ++i) {
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
{
    if (channelsCount()==0) return 0.0;
    return channel(0)->data()->xMin();
}

int FileDescriptor::samplesCount() const
{
    if (channelsCount()==0) return 0;
    return channel(0)->data()->samplesCount();
}

QString FileDescriptor::xName() const
{
    QString result;
    if (channelsCount() > 0) {
        result = channel(0)->xName();
        for (int i=1; i<channelsCount(); ++i) {
            if (channel(i)->xName() != result) return "разные";
        }
    }
    return result;
}

bool FileDescriptor::dataTypeEquals(FileDescriptor *other) const
{
    return (this->type() == other->type());
}

QString FileDescriptor::legend() const
{
    return dataDescription().get("legend").toString();
}

bool FileDescriptor::setLegend(const QString &s)
{
    if (s==legend()) return false;
    dataDescription().put("legend", s);
    setChanged(true);
    return true;
}

bool FileDescriptor::canTakeChannelsFrom(FileDescriptor *other) const
{
    Q_UNUSED(other);
    return true;
}

bool FileDescriptor::canTakeAnyChannels() const
{
    return true;
}

qint64 FileDescriptor::fileSize() const
{
    return QFileInfo(fileName()).size();
}

bool FileDescriptor::hasCurves() const
{
    const int count = channelsCount();
    for (int i=0; i<count; ++i) {
        if (channel(i)->plotted()) return true;
    }
    return false;
}

QString FileDescriptor::createGUID()
{
    QString result = QUuid::createUuid().toString().toUpper();
    if (result.at(24) == '-') result.remove(24,1);
    else result.remove(25,1);
    return result;
}

//QString descriptionEntryToString(const DescriptionEntry &entry)
//{
//    QString result = entry.second;
//    if (!entry.first.isEmpty()) return entry.first+"="+result;

//    return result;
//}

Channel::Channel(Channel *other) :
    _color(QColor()), _plotted(0),
    _populated(other->_populated),
    _data(new DataHolder(*(other->_data))),
    _dataDescription(other->_dataDescription)
{

}

Channel::Channel(Channel &other) :
    _color(QColor()), _plotted(0),
    _populated(other._populated),
    _data(new DataHolder(*(other._data))),
    _dataDescription(other._dataDescription)
{

}

QVariant Channel::info(int column, bool edit) const
{
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
{
    return 7;
}

QVariant Channel::channelHeader(int column) const
{
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
{
    return dataDescription().get("function.octaveFormat").toInt();
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

QString Channel::name() const
{
    return dataDescription().get("name").toString();
}

void Channel::setName(const QString &name)
{
    dataDescription().put("name", name);
}

QString Channel::description() const
{
    return _dataDescription.data.value("description").toString();
}

void Channel::setDescription(const QString &description)
{
    _dataDescription.data.insert("description", description);
}

QString Channel::xName() const
{
    return _dataDescription.data.value("xname").toString();
}

QString Channel::yName() const
{
    return _dataDescription.data.value("yname").toString();
}

QString Channel::yNameOld() const
{
    return yName();
}

QString Channel::zName() const
{
    return _dataDescription.data.value("zname").toString();
}

void Channel::setYName(const QString &yName)
{
    _dataDescription.data.insert("yname", yName);
}

void Channel::setXName(const QString &xName)
{
    _dataDescription.data.insert("xname", xName);
}

void Channel::setZName(const QString &zName)
{
    _dataDescription.data.insert("zname", zName);
}

void Channel::setXStep(double xStep)
{
    _data->setXStep(xStep);
}

QString Channel::legendName() const
{
    QStringList l;
    l << name();
    if (!correction().isEmpty()) l << correction();
    if (!descriptor()->legend().isEmpty()) l << descriptor()->legend();

    return l.join(" ");
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



QString stringify(const QVector<int> &vec)
{
    QStringList result;
    for (int i:vec) result<<QString::number(i);
    return result.join(",");
}

QDataStream &operator>>(QDataStream &stream, DataDescription &data)
{
    stream >> data.data;
    return stream;
}

QDataStream &operator<<(QDataStream &stream, const DataDescription &data)
{
    stream << data.data;
    return stream;
}

void DataDescription::put(const QString &key, const QVariant &value) {
    data.insert(key, value);
}

QVariant DataDescription::get(const QString &key) const
{
    return data.value(key);
}

QJsonObject DataDescription::toJson() const {
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

DataDescription DataDescription::fromJson(const QJsonObject &o) {
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

QStringList DataDescription::twoStringDescription() const
{
    QStringList result = toStringList("description", true);
    result = result.mid(0,2);
    return result;
}

QStringList DataDescription::toStringList(const QString &filter, bool includeKeys) const
{
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
{
    if (filter.isEmpty()) return data;

    QVariantMap result;
    for (const auto [key, val] : asKeyValueRange(data)) {
        if (key.startsWith(filter+".")) result.insert(key.mid(filter.length()+1), val);
    }

    return result;
}
