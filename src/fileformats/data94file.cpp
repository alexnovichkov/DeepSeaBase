#include "data94file.h"

#include <QJsonDocument>
#include "algorithms.h"
#include "logging.h"

Data94File::Data94File(const QString &fileName) : FileDescriptor(fileName)
{

}

Data94File::Data94File(const Data94File &other, const QString &fileName, QVector<int> indexes)
    : FileDescriptor(fileName)
{
    this->description = other.description;
    this->xAxisBlock = other.xAxisBlock;
    this->zAxisBlock = other.zAxisBlock;
    this->descriptionSize = other.descriptionSize;
    this->paddingSize = other.paddingSize;

    updateDateTimeGUID();

    //если индексы пустые - копируем все каналы
    if (indexes.isEmpty())
        for (int i=0; i<other.channelsCount(); ++i) indexes << i;

    for (int i: indexes) {
        Data94Channel *c = other.channels.at(i);
        this->channels << new Data94Channel(c);
    }

    foreach (Data94Channel *c, channels) {
        c->parent = this;
    }

    //сразу сохраняем файл
    setChanged(true);
    write();

    writeData(other, indexes);
}

//сразу записываем данные, взятые из файла other
Data94File::Data94File(const FileDescriptor &other, const QString &fileName, QVector<int> indexes)
    : FileDescriptor(fileName)
{
    updateDateTimeGUID();
    description.insert("sourceFile", other.fileName());
    description.insert("legend", other.legend());
    setDataDescriptor(other.dataDescriptor());

    //если индексы пустые - копируем все каналы
    if (indexes.isEmpty())
        for (int i=0; i<other.channelsCount(); ++i) indexes << i;

    //Поскольку other может содержать каналы с разным размером и шагом,
    //данные берем из первого канала, который будем сохранять
    //предполагается, что все каналы из indexes имеют одинаковые параметры

    Channel *firstChannel = other.channel(indexes.constFirst());
    Q_ASSERT_X(firstChannel, "Data94 constructor", "channel to copy is null");


    xAxisBlock.uniform = firstChannel->data()->xValuesFormat() == DataHolder::XValuesUniform ? 1:0;
    xAxisBlock.begin = firstChannel->xMin();
    if (xAxisBlock.uniform == 0) // not uniform
        xAxisBlock.values = firstChannel->xValues();

    xAxisBlock.count = firstChannel->samplesCount();
    xAxisBlock.step = firstChannel->xStep();
    xAxisBlock.isValid = true;

    //zAxisBlock
    zAxisBlock.count = 1;
    zAxisBlock.isValid = true;

    //channels - сначала только описание каналов
    //если каналы были populated, то данные игнорируются
    for (int i: indexes) {
        Channel *ch = other.channel(i);
        Data94Channel *c = new Data94Channel(ch);
        c->parent = this;
        channels << c;
    }

    //теперь записываем файл
    setChanged(true);
    write();

    writeData(other, indexes);
}

void Data94File::writeData(const FileDescriptor &d, const QVector<int> &indexes)
{
    //теперь записываем данные - это позволит не копить данные в оперативной
    //памяти, и работать с файлами с любым количеством каналов
    QFile f(fileName());
    if (!f.open(QFile::ReadWrite)) {
        qDebug()<<"Не удалось открыть файл для записи";
        return;
    }

    QDataStream r(&f);
    r.setByteOrder(QDataStream::LittleEndian);
    r.setFloatingPointPrecision(QDataStream::SinglePrecision);
    // шапка файла имеет размер
    const qint64 fileHeader = 8+4+descriptionSize+4+paddingSize
                              +xAxisBlock.size()+zAxisBlock.size()+4;

    qint64 dataPosition = fileHeader + 4;
    int destIndex = 0;

    for (int i=0; i<d.channelsCount(); ++i) {
        if (!indexes.contains(i)) continue;

        Data94Channel *destChannel = channels.at(destIndex);
        Channel *sourceChannel = d.channel(i);

        bool populated = sourceChannel->populated();
        if (!populated) sourceChannel->populate();

        //пишем данные напрямую
        quint32 format = destChannel->isComplex ? 2 : 1;
        const qint64 csize = xAxisBlock.count * zAxisBlock.count * format * sizeof(float);

        destChannel->dataPosition = dataPosition;
        dataPosition += csize //channel size in bytes
                        + 4; //channel format

        r.device()->seek(destChannel->dataPosition - 4);
        r << format;

        if (!destChannel->isComplex) {
            QVector<double> yValues = sourceChannel->data()->rawYValues();
            if (yValues.isEmpty()) {
                qDebug()<<"Отсутствуют данные для записи в канале"<<destChannel->name();
                continue;
            }

            foreach (double v, yValues) {
                r << (float)v;
            }
        }
        else {
            auto yValues = sourceChannel->data()->yValuesComplex();
            if (yValues.isEmpty()) {
                qDebug()<<"Отсутствуют данные для записи в канале"<<destChannel->name();
                continue;
            }
            foreach (cx_double v, yValues) {
                r << (float)v.real();
                r << (float)v.imag();
            }
        }

        if (!populated) {
            sourceChannel->clear();
            destChannel->clear();
        }
        destIndex++;
    }
}

void Data94File::fillPreliminary(Descriptor::DataType)
{
}

void Data94File::fillRest()
{
}

void Data94File::read()
{
    QFile f(fileName());

    if (!f.open(QFile::ReadOnly)) return;

    QDataStream r(&f);
    r.setByteOrder(QDataStream::LittleEndian);
    r.setFloatingPointPrecision(QDataStream::SinglePrecision);
    QString label = QString::fromLocal8Bit(r.device()->read(8));

    if (label != "data94  ") {
        qDebug()<<"файл неправильного типа";
        return;
    }

    //reading file description
    r >> descriptionSize;
    QByteArray descriptionBuffer = r.device()->read(descriptionSize);
    if ((quint32)descriptionBuffer.size() != descriptionSize) {
        qDebug()<<"не удалось прочитать описание файла";
        return;
    }
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(descriptionBuffer, &error);
    if (error.error != QJsonParseError::NoError) {
        qDebug()<<error.errorString() << error.offset;
        return;
    }
    description = doc.object();

    //reading padding
    r >> paddingSize;
    r.device()->skip(paddingSize);

    //reading xAxisBlock
    //данные по оси Х одинаковые для всех каналов
    xAxisBlock.read(r);

    //reading zAxisBlock
    //данные по оси Z одинаковые для всех каналов
    zAxisBlock.read(r);

    //дальше - каналы
    quint32 channelsCount;
    r >> channelsCount;
    QJsonArray channelsDescription = description.value("channels").toArray();
    for (quint32 i = 0; i < channelsCount; ++i) {
        Data94Channel *c = new Data94Channel(this);
        c->_description = channelsDescription.at(i).toObject();
        c->read(r);
        channels << c;
    }
    description.remove("channels");
}

void Data94File::write()
{
    if (!changed() && !dataChanged()) return;

    QFile f(fileName());
    if (!f.open(QFile::ReadWrite)) {
        qDebug()<<"Не удалось открыть файл для записи";
        return;
    }

    QDataStream r(&f);
    r.setByteOrder(QDataStream::LittleEndian);
    r.setFloatingPointPrecision(QDataStream::SinglePrecision);

    //переписываем описательную часть
    if (changed()) {
        QJsonArray array;
        foreach (Data94Channel *c, channels) {
            array.append(c->_description);
        }
        QJsonObject d = description;
        d.insert("channels", array);

        QJsonDocument doc(d);
        QByteArray json = doc.toJson();


        //общая длина записанного с учетом паддинга равна descriptionSize + paddingSize + 4
        const quint32 newDescriptionSize = json.size();
        const quint32 replace = descriptionSize + paddingSize;

        r.device()->write("data94  ");
        //сначала записываем размер данных
        r << newDescriptionSize;
        descriptionSize = newDescriptionSize;

        //определяем, хватает ли нам паддинга
        if (newDescriptionSize < replace) {
            //можем записывать прямо поверх, обновляя паддинг
            paddingSize = replace - newDescriptionSize;

            r.writeRawData(json.data(), newDescriptionSize);
            r << paddingSize;
            QByteArray padding; padding.resize(paddingSize);
            r.writeRawData(padding.data(), paddingSize);
        }
        else {
            //мы должны перезаписать весь файл, и только затем продолжать
            //даже если новый паддинг получается нулевым, все равно перезаписываем
            //паддинг не обновляем, так как он остается прежним

            ulong bufferLength = PADDING_SIZE;

            while (newDescriptionSize - replace > bufferLength)
                bufferLength += PADDING_SIZE;

            long readPosition = 12 + replace;
            long writePosition = 12;

            QByteArray buffer = json;
            QByteArray aboutToOverwrite; aboutToOverwrite.resize(bufferLength);

            while (true) {
                // Seek to the current read position and read the data that we're about
                // to overwrite.  Appropriately increment the readPosition.

                r.device()->seek(readPosition);
                size_t bytesRead;

                aboutToOverwrite = r.device()->read(bufferLength);
                bytesRead = aboutToOverwrite.length();

                readPosition += bufferLength;

                // Seek to the write position and write our buffer.  Increment the
                // writePosition.

                r.device()->seek(writePosition);
                r.writeRawData(buffer.data(), buffer.length());

                // We hit the end of the file.
                if (bytesRead == 0) break;

                writePosition += buffer.size();

                // Make the current buffer the data that we read in the beginning.
                buffer = aboutToOverwrite;
            }
            r.device()->seek(8+4+descriptionSize);
            r << paddingSize;
            QByteArray padding(paddingSize, 0x0); //padding.resize(paddingSize);
            r.writeRawData(padding.data(), paddingSize);
        }


        //записываем блоки осей
        r.device()->seek(8+4+descriptionSize+4+paddingSize);
        xAxisBlock.write(r);
        zAxisBlock.write(r);
        r << quint32(channels.count());

        setChanged(false);
        foreach (auto *c, channels) {
            c->setChanged(false);
        }
    }

    if (dataChanged()) {
        // шапка файла имеер размер
        const qint64 fileHeader = 8+4+descriptionSize+4+paddingSize
                                  +xAxisBlock.size()+zAxisBlock.size()+4;

        for (int i=0; i<channels.size(); ++i) {
            Data94Channel *c = channels.at(i);
            if (!c->dataChanged()) continue;
            const quint32 format = c->isComplex ? 2 : 1;

            if (c->dataPosition < 0) {
                //каждый канал начинается с позиции
                c->dataPosition = fileHeader + 4 +
                                  i*(4+xAxisBlock.count*zAxisBlock.count*format*sizeof(float));
            }

            r.device()->seek(c->dataPosition - 4);
            r << format;

            if (!c->isComplex) {
                const QVector<double> yValues = c->data()->rawYValues();
                if (yValues.isEmpty()) {
                    qDebug()<<"Отсутствуют данные для записи в канале"<<c->name();
                    continue;
                }

                for (double v: yValues) {
                    r << (float)v;
                }
            }
            else {
                const auto yValues = c->data()->yValuesComplex();
                if (yValues.isEmpty()) {
                    qDebug()<<"Отсутствуют данные для записи в канале"<<c->name();
                    continue;
                }
                for (cx_double v: yValues) {
                    r << (float)v.real();
                    r << (float)v.imag();
                }
            }
            c->setDataChanged(false);
        }
    }

}

void Data94File::writeRawFile()
{
    //no-op
}

void Data94File::updateDateTimeGUID()
{
    description.insert("dateTime", QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm"));
}

int Data94File::channelsCount() const
{
    return channels.size();
}

Descriptor::DataType Data94File::type() const
{
    if (channels.isEmpty()) return Descriptor::Unknown;

    Descriptor::DataType t = channels.constFirst()->type();
    for (int i=1; i<channels.size(); ++i) {
        if (channels.at(i)->type() != t) return Descriptor::Unknown;
    }
    return t;
}

QString Data94File::typeDisplay() const
{
    return Descriptor::functionTypeDescription(type());
}

DescriptionList Data94File::dataDescriptor() const
{
    DescriptionList result;
    QJsonObject d = description.value("dataDescription").toObject();
    for (auto i = d.constBegin(); i != d.constEnd(); ++i) {
        result << qMakePair<QString, QString>(i.key(),i.value().toString());
    }

    return result;
}

void Data94File::setDataDescriptor(const DescriptionList &data)
{
    QJsonObject result;
    for (int i=0; i<data.size(); ++i) {
        result.insert(data.at(i).first, data.at(i).second);
    }
    if (description.value("dataDescription").toObject() != result) {
        description.insert("dataDescription", result);
        setChanged(true);
    }
}

QString Data94File::dataDescriptorAsString() const
{
    QStringList result;

    QJsonObject d = description.value("dataDescription").toObject();
    for (auto i = d.constBegin(); i != d.constEnd(); ++i) {
        result << i.value().toString();
    }

    return result.join("; ");
}

QDateTime Data94File::dateTime() const
{
    QString dt = description.value("dateTime").toString();
    if (!dt.isEmpty())
        return QDateTime::fromString(dt, "dd.MM.yyyy hh:mm");
    return QDateTime();
}

bool Data94File::setDateTime(QDateTime dt)
{
    if (dateTime() == dt) return false;
    description.insert("dateTime", dt.toString("dd.MM.yyyy hh:mm"));
    setChanged(true);
    return true;
}

void Data94File::deleteChannels(const QVector<int> &channelsToDelete)
{
    QTemporaryFile tempFile;
    QFile rawFile(fileName());

    if (!tempFile.open() || !rawFile.open(QFile::ReadOnly)) {
        qDebug()<<" Couldn't replace raw file with temp file.";
        return;
    }

    QDataStream tempStream(&tempFile);
    tempStream.setByteOrder(QDataStream::LittleEndian);
    tempStream.setFloatingPointPrecision(QDataStream::SinglePrecision);

    QDataStream rawStream(&rawFile);
    rawStream.setByteOrder(QDataStream::LittleEndian);
    rawStream.setFloatingPointPrecision(QDataStream::SinglePrecision);

    //записываем шапку файла
    QByteArray buffer = rawStream.device()->read(8+4+descriptionSize+4+paddingSize
                                                 + xAxisBlock.size()+zAxisBlock.size());
    tempStream.device()->write(buffer);
    //записываем новое количество каналов
    tempStream << channels.size() - channelsToDelete.size();

    for (int i = 0; i < channels.size(); ++i) {
        // пропускаем канал, предназначенный для удаления
        if (channelsToDelete.contains(i)) continue; // 0 = keep, 1 = delete
        rawStream.device()->seek(channels[i]->dataPosition-4);

        //читаем формат
        quint32 format;
        rawStream >> format;

        //читаем данные
        buffer = rawStream.device()->read(xAxisBlock.count * zAxisBlock.count * format * 4);
        //                                  samples            blocks           complex

        //пишем данные
        tempStream << format;
        channels[i]->dataPosition = tempStream.device()->pos();
        tempStream.device()->write(buffer);
        tempFile.flush();
    }

    rawFile.close();
    tempFile.close();

    // удаляем файл данных, если мы перезаписывали его
    if (rawFile.remove()) {
        tempFile.copy(fileName());
    }

    for (int i=channels.size()-1; i>=0; --i) {
        if (channelsToDelete.contains(i)) {
            delete channels.takeAt(i);
        }
    }
    // перезаписываем описатель файла
    setChanged(true);
    write();
}

void Data94File::copyChannelsFrom(FileDescriptor *sourceFile, const QVector<int> &indexes)
{
    const int count = channelsCount();

    updateDateTimeGUID();

    for (int i: indexes) {
        Channel *c = sourceFile->channel(i);
        this->channels << new Data94Channel(c);
    }

    foreach (Data94Channel *c, channels) {
        c->parent = this;
    }

    //сразу сохраняем файл
    setChanged(true);
    write();

    //теперь записываем данные - это позволит не копить данные в оперативной
    //памяти, и работать с файлами с любым количеством каналов
    QFile f(fileName());
    if (!f.open(QFile::ReadWrite)) {
        qDebug()<<"Не удалось открыть файл для записи";
        return;
    }

    QDataStream r(&f);
    r.setByteOrder(QDataStream::LittleEndian);
    r.setFloatingPointPrecision(QDataStream::SinglePrecision);
    // шапка файла имеет размер
    const qint64 fileHeader = 8+4+descriptionSize+4+paddingSize
                              +xAxisBlock.size()+zAxisBlock.size()+4;

    int destIndex = count;
    for (int i=0; i<sourceFile->channelsCount(); ++i) {
        if (!indexes.contains(i)) continue;

        Data94Channel *destChannel = channels.at(destIndex);
        Channel *sourceChannel = sourceFile->channel(i);

        bool populated = sourceChannel->populated();
        if (!populated) sourceChannel->populate();

        //пишем данные напрямую
        quint32 format = destChannel->isComplex ? 2 : 1;

        if (destChannel->dataPosition < 0) {
            //каждый канал начинается с позиции
            destChannel->dataPosition = fileHeader + 4
                              + destIndex*(4+xAxisBlock.count*zAxisBlock.count*format*sizeof(float));
        }

        r.device()->seek(destChannel->dataPosition - 4);
        r << format;

        if (!destChannel->isComplex) {
            QVector<double> yValues = sourceChannel->data()->rawYValues();
            if (yValues.isEmpty()) {
                qDebug()<<"Отсутствуют данные для записи в канале"<<destChannel->name();
                continue;
            }

            foreach (double v, yValues) {
                r << (float)v;
            }
        }
        else {
            auto yValues = sourceChannel->data()->yValuesComplex();
            if (yValues.isEmpty()) {
                qDebug()<<"Отсутствуют данные для записи в канале"<<destChannel->name();
                continue;
            }
            foreach (cx_double v, yValues) {
                r << (float)v.real();
                r << (float)v.imag();
            }
        }

        if (!populated) {
            sourceChannel->clear();
            destChannel->clear();
        }
        destIndex++;
    }
}

void Data94File::calculateMean(const QList<QPair<FileDescriptor *, int> > &channels)
{
}

QString Data94File::calculateThirdOctave()
{
}

void Data94File::calculateMovingAvg(const QList<QPair<FileDescriptor *, int> > &channels, int windowSize)
{
}

QString Data94File::saveTimeSegment(double from, double to)
{
}

void Data94File::move(bool up, const QVector<int> &indexes, const QVector<int> &newIndexes)
{
}

QVariant Data94File::channelHeader(int column) const
{
    if (channels.isEmpty()) return QVariant();
    return channels[0]->channelHeader(column);
}

int Data94File::columnsCount() const
{
    if (channels.isEmpty()) return 6;
    return channels[0]->columnsCount();
}

Channel *Data94File::channel(int index) const
{
    if (channels.size()>index)
        return channels.at(index);
    return 0;
}

QString Data94File::legend() const
{
    return description.value("legend").toString();
}

bool Data94File::setLegend(const QString &legend)
{
    if (this->legend() != legend) {
        description.insert("legend", legend);
        setChanged(true);
        return true;
    }
    return false;
}

double Data94File::xBegin() const
{
    return xAxisBlock.uniform ? xAxisBlock.begin : xAxisBlock.values.value(0);
}

double Data94File::xStep() const
{
    return xAxisBlock.uniform ? xAxisBlock.step : 0.0;
}

void Data94File::setXStep(const double xStep)
{
    if (!xAxisBlock.uniform) return;

    bool changed = false;
    changed = xAxisBlock.step != xStep;
    xAxisBlock.step = xStep;

    if (channels.isEmpty()) return;

    for (int i=0; i<channels.size(); ++i) {
        if (channels.at(i)->xStep()!=xStep) {
            changed = true;
            channels[i]->setXStep(xStep);
        }
    }
    if (changed) setChanged(true);
//    write();
}

int Data94File::samplesCount() const
{
    return xAxisBlock.count;
}

void Data94File::setSamplesCount(int count)
{
    xAxisBlock.count = count;
    //дублируем
    foreach (Data94Channel *c, channels) {
        c->_description.insert("samples", count);
        c->data()->setSamplesCount(count);
    }
}

QString Data94File::xName() const
{
    if (channels.isEmpty()) return QString();

    QString xname = channels.constFirst()->xName();

    for (int i=1; i<channels.size(); ++i) {
        if (channels[i]->xName() != xname) return QString();
    }
    return xname;
}

bool Data94File::dataTypeEquals(FileDescriptor *other) const
{
    return (this->type() == other->type());
}

QStringList Data94File::fileFilters()
{
    return QStringList()<<"Файлы Data94 (*.d94)";
}

QStringList Data94File::suffixes()
{
    return QStringList()<<"*.d94";
}


/*Data94Channel implementation*/

Data94Channel::Data94Channel(Data94File *parent) : Channel(),
    parent(parent)
{

}

Data94Channel::Data94Channel(Data94Channel *other) : Channel(other)
{
    _description = other->_description;
    isComplex = other->isComplex;
}

Data94Channel::Data94Channel(Channel *other) : Channel(other)
{
    isComplex = other->data()->yValuesFormat() == DataHolder::YValuesComplex;
    _description.insert("id", 1);
    _description.insert("name", other->name());
    _description.insert("description", other->description());
    _description.insert("correction", other->correction());
    _description.insert("yname", other->yName());
    _description.insert("xname", other->xName());
    _description.insert("zname", other->zName());
    _description.insert("samples", other->data()->samplesCount());
    _description.insert("blocks", other->data()->blocksCount());
    if (other->data()->xValuesFormat() == DataHolder::XValuesUniform)
        _description.insert("samplerate", int(1.0 / other->data()->xStep()));

    QJsonObject function;
    function.insert("name", Descriptor::functionTypeDescription(other->type()));
    function.insert("type", other->type());
    QString format;
    switch (other->data()->yValuesFormat()) {
        case DataHolder::YValuesComplex: format = "complex"; break;
        case DataHolder::YValuesReals: format = "real"; break;
        case DataHolder::YValuesAmplitudes: format = "amplitude"; break;
        case DataHolder::YValuesAmplitudesInDB: format = "amplitudeDb"; break;
        case DataHolder::YValuesImags: format = "imaginary"; break;
        case DataHolder::YValuesPhases: format = "phase"; break;
        default: format = "real";
    }
    function.insert("format", format);
    function.insert("logref", other->data()->threshold());
    QString units;
    switch (other->data()->yValuesUnits()) {
        case DataHolder::UnitsUnknown: units = "unknown"; break;
        case DataHolder::UnitsLinear: units = "linear"; break;
        case DataHolder::UnitsQuadratic: units = "quadratic"; break;
        case DataHolder::UnitsDimensionless: units = "dimensionless"; break;
        default: break;
    }
    function.insert("units", units);
    function.insert("octaveFormat", other->octaveType());
    _description.insert("function", function);


//    *           "responseName": "lop1:1",
//    *           "responseDirection": "+z",
//    *           "referenceName": "lop1:1",
//    *           "referenceDirection": "",
//    *           "sensorID" : "", //ChanAddress
//    *           "sensorName": "", //ChanName

//    *           "bandwidth": 3200, //обычно samplerate/2.56, но может и отличаться при полосной фильтрации
//    *           "function": {
//    *               //далее идут все параметры обработки
//    *           }
}

void Data94Channel::read(QDataStream &r)
{
    if (r.status() != QDataStream::Ok) return;
    quint32 valueFormat;
    r >> valueFormat;

    isComplex = valueFormat == 2;

    dataPosition = r.device()->pos();

    double thr = _description.value("function").toObject().value("logref").toDouble();
    if (thr == 0.0) thr = threshold(yName());
    _data->setThreshold(thr);

    int units = DataHolder::UnitsUnknown;
    QString unitsS = _description.value("function").toObject().value("units").toString();
    if (unitsS == "quadratic") units = DataHolder::UnitsQuadratic;
    else if (unitsS == "linear") units = DataHolder::UnitsLinear;
    else if (unitsS == "dimensionless") units = DataHolder::UnitsDimensionless;
    _data->setYValuesUnits(units);

    DataHolder::YValuesFormat yValueFormat = DataHolder::YValuesUnknown;
    QString format = _description.value("function").toObject().value("format").toString();
    if (format == "complex") yValueFormat = DataHolder::YValuesComplex;
    else if (format == "real") yValueFormat = DataHolder::YValuesReals;
    else if (format == "imaginary") yValueFormat = DataHolder::YValuesImags;
    else if (format == "amplitude") yValueFormat = DataHolder::YValuesAmplitudes;
    else if (format == "amplitudeDb") yValueFormat = DataHolder::YValuesAmplitudesInDB;
    else if (format == "phase") yValueFormat = DataHolder::YValuesPhases;
    _data->setYValuesFormat(yValueFormat);

    if (parent->xAxisBlock.uniform == 1)
        _data->setXValues(parent->xAxisBlock.begin, parent->xAxisBlock.step, parent->xAxisBlock.count);

    _data->setBlocksCount(parent->zAxisBlock.count);

    r.device()->skip(parent->zAxisBlock.count * parent->xAxisBlock.count * valueFormat * sizeof(float));
    // соответствия:         blockCount                 sampleCount         factor
}

void Data94Channel::setXStep(double xStep)
{
    _data->setXStep(xStep);
}

QVariant Data94Channel::info(int column, bool edit) const
{
    Q_UNUSED(edit)
    switch (column) {
        case 0: return _description.value("name"); //avoiding conversion variant->string->variant
        case 1: return _description.value("yname");
        case 2: return data()->yValuesFormatString();
        case 3: return _description.value("description");
        case 4: return _description.value("function").toObject().value("name");
        case 5: return _description.value("correction");
        default: ;
    }
    return QVariant();
}

int Data94Channel::columnsCount() const
{
    int minimumCount = 6;
    ///TODO: предусмотреть возможность показывать расширенный список свойств

    return minimumCount;
}

QVariant Data94Channel::channelHeader(int column) const
{
    switch (column) {
        case 0: return QString("Имя");
        case 1: return QString("Ед.изм.");
        case 2: return QString("Формат");
        case 3: return QString("Описание");
        case 4: return QString("Функция");
        case 5: return QString("Коррекция");
        default: return QVariant();
    }
    return QVariant();
}

Descriptor::DataType Data94Channel::type() const
{
    return Descriptor::DataType(_description.value("function").toObject().value("type").toInt());
}

int Data94Channel::octaveType() const
{
    return _description.value("function").toObject().value("octaveFormat").toInt(0);
}

void Data94Channel::populate()
{
    ///Сейчас программа не поддерживает чтение нескольких блоков данных,
    /// поэтому читаем только первый блок

    // clear previous data;
    _data->clear();

    setPopulated(false);


    QFile rawFile(parent->fileName());

//    QTime time;
//    time.start();

    if (rawFile.open(QFile::ReadOnly)) {
        QVector<double> YValues;

        //количество отсчетов в одном блоке - удваивается, если данные комплексные
        const quint64 blockSize = parent->xAxisBlock.count * (isComplex ? 2 : 1);

        if (dataPosition < 0) {
            qDebug()<<"Поврежденный файл: не удалось найти положение данных в файле";
        }
        else {
            // map file into memory
            unsigned char *ptr = rawFile.map(0, rawFile.size());
            if (ptr) {//достаточно памяти отобразить весь файл
                unsigned char *maxPtr = ptr + rawFile.size();
                unsigned char *ptrCurrent = ptr;
                if (dataPosition >= 0) {
                    ptrCurrent = ptr + dataPosition;

                    YValues = convertFrom<double>(ptrCurrent,
                                                  qMin(maxPtr-ptrCurrent, int(blockSize * sizeof(float))),
                                                  0xc0000004);
                }
            }
            else {
                //читаем классическим способом
                QDataStream readStream(&rawFile);
                readStream.setByteOrder(QDataStream::LittleEndian);
                readStream.setFloatingPointPrecision(QDataStream::SinglePrecision);

                readStream.device()->seek(dataPosition);
                YValues = getChunkOfData<double>(readStream, blockSize, 0xc0000004, 0);
            }
        }

        //меняем размер, если не удалось прочитать весь блок данных
        YValues.resize(blockSize);
        if (isComplex) {
            QVector<cx_double> co(parent->xAxisBlock.count);
            for (uint i=0; i<parent->xAxisBlock.count; ++i) {
                co[i] = {YValues[i*2], YValues[i*2+1]};
            }
            _data->setYValues(co);
        }
        else
            _data->setYValues(YValues, _data->yValuesFormat());

        setPopulated(true);
        rawFile.close();

        if (!parent->xAxisBlock.values.isEmpty()) {//данные по оси Х
            _data->setXValues(parent->xAxisBlock.values);
        }
    }
    else {
        qDebug()<<"Не удалось открыть файл"<<parent->fileName();
    }
}

QString Data94Channel::name() const
{
    return _description.value("name").toString();
}

void Data94Channel::setName(const QString &name)
{
    _description.insert("name", name);
}

QString Data94Channel::description() const
{
    return _description.value("description").toString();
}

void Data94Channel::setDescription(const QString &description)
{
    _description.insert("description", description);
}

QString Data94Channel::xName() const
{
    return _description.value("xname").toString();
}

QString Data94Channel::yName() const
{
    return _description.value("yname").toString();
}

QString Data94Channel::zName() const
{
    return _description.value("zname").toString();
}

void Data94Channel::setYName(const QString &yName)
{
    _description.insert("yname", yName);
}

QString Data94Channel::legendName() const
{
    QStringList l;
    l << name();
    if (!correction().isEmpty()) l << correction();
    if (!parent->legend().isEmpty()) l << parent->legend();

    return l.join(" ");
}

FileDescriptor *Data94Channel::descriptor()
{
    return parent;
}

int Data94Channel::index() const
{
    if (parent) return parent->channels.indexOf(const_cast<Data94Channel*>(this), 0);
    return 0;
}

QString Data94Channel::correction() const
{
    return _description.value("correction").toString();
}

void Data94Channel::setCorrection(const QString &s)
{
    _description.insert("correction", s);
}

void AxisBlock::read(QDataStream &r)
{DD;
    if (r.status() != QDataStream::Ok) return;

    r.setFloatingPointPrecision(QDataStream::SinglePrecision);

    r >> uniform;//     0 - шкала неравномерная, 1 - шкала равномерная
    r >> count;

    if (uniform > 0) {
        r >> begin;
        r >> step;
        isValid = true;
    }
    else {
        int actuallyRead = 0;
        values = getChunkOfData<double>(r, count, 0xC0000004, &actuallyRead);
        if (uint(actuallyRead) == count) isValid = true;
    }
}

void AxisBlock::write(QDataStream &r)
{DD;
    if (r.status() != QDataStream::Ok) return;

    r.setFloatingPointPrecision(QDataStream::SinglePrecision);

    r << uniform;//     0 - шкала неравномерная, 1 - шкала равномерная
    r << count;

    if (uniform > 0) {
        r << begin;
        r << step;
    }
    else {
        foreach (double x, values) r << float(x);
    }
}

quint32 AxisBlock::size() const
{
    if (!isValid) return 0;

    if (uniform) return 16;

    return 8 + values.size() * 4;
}
