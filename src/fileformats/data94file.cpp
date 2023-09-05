#include "data94file.h"

#include <QJsonDocument>
#include "algorithms.h"
#include "logging.h"
#include "unitsconverter.h"
#include "settings.h"

QDebug operator<<(QDebug dbg, const AxisBlock &b)
{
    QDebugStateSaver saver(dbg);
    dbg << "(uniform=" << b.uniform << ", count=" << b.count;
    if (b.uniform==1) {
        dbg <<", begin="<<b.begin<<", step="<<b.step;
    }
    else
        dbg << b.values;
    dbg << ")";

    return dbg;
}

DataPrecision toDataPrecision(const QString &s)
{DD;
    if (s=="int8")        return DataPrecision::Int8;
    else if (s=="uint8")  return DataPrecision::UInt8;
    else if (s=="int16")  return DataPrecision::Int16;
    else if (s=="uint16") return DataPrecision::UInt16;
    else if (s=="uint32") return DataPrecision::UInt32;
    else if (s=="int32")  return DataPrecision::Int32;
    else if (s=="int64")  return DataPrecision::Int64;
    else if (s=="uint64") return DataPrecision::UInt64;
    else if (s=="float") return DataPrecision::Float;
    else if (s=="double") return DataPrecision::Double;
    return DataPrecision::Float;
}

Data94File::Data94File(const QString &fileName) : FileDescriptor(fileName)
{DD;

}

Data94File::Data94File(const FileDescriptor &other, const QString &fileName, QVector<int> indexes)
    : FileDescriptor(fileName)
{DD;
    QVector<Channel *> source;
    if (indexes.isEmpty())
        for (int i=0; i<other.channelsCount(); ++i) source << other.channel(i);
    else
        for (int i: indexes) source << other.channel(i);

    init(source);
}

Data94File::Data94File(const QVector<Channel *> &source, const QString &fileName)
 : FileDescriptor(fileName)
{DD;
    init(source);
}

void Data94File::init(const QVector<Channel *> &source)
{DD;
    if (source.isEmpty()) return;

    auto d = source.first()->descriptor();

    setDataDescription(d->dataDescription());
    updateDateTimeGUID();

    if (channelsFromSameFile(source)) {
        dataDescription().put("source.file", d->fileName());
        dataDescription().put("source.guid", d->dataDescription().get("guid"));
        dataDescription().put("source.dateTime", d->dataDescription().get("dateTime"));

        if (d->channelsCount() > source.size()) {
            //только если копируем не все каналы
            dataDescription().put("source.channels", stringify(channelIndexes(source)));
        }
    }

    QFile f(fileName());
    if (!f.open(QFile::WriteOnly)) {
        LOG(ERROR)<<QString("Не удалось открыть файл для записи:")<<fileName();
        return;
    }

    QDataStream r(&f);
    r.setByteOrder(QDataStream::LittleEndian);
    r.setFloatingPointPrecision(QDataStream::SinglePrecision);

    //переписываем описательную часть
    r.device()->write("data94  ");

    QJsonDocument doc(dataDescription().toJson());
    QByteArray json = doc.toJson();
    descriptionSize = json.size();
    r << descriptionSize;
    r.writeRawData(json.data(), descriptionSize);

    //записываем количество каналов
    r << quint32(source.size());

    for (auto sourceChannel: qAsConst(source)) {
        Data94Channel *c = new Data94Channel(sourceChannel, this);
        c->setChanged(true);
        c->setDataChanged(true);

        bool populated = sourceChannel->populated();
        if (!populated) sourceChannel->populate();
        c->write(r, 0, sourceChannel->data()); //данные берутся из sourceChannel

        if (!populated) sourceChannel->clear();
    }
}

Data94File::~Data94File()
{DD;
    if (changed() || dataChanged())
        write();

    qDeleteAll(channels);
}

void Data94File::updatePositions()
{DD;
    // шапка файла имеет размер
//    const qint64 fileHeader = 8+4+descriptionSize+4+paddingSize
//                              +xAxisBlock.size()+zAxisBlock.size()+4;

//    qint64 dataPosition = fileHeader + 4;

//    for (Data94Channel *ch: qAsConst(channels)) {
//        quint32 format = ch->isComplex ? 2 : 1;
//        const qint64 csize = xAxisBlock.count * zAxisBlock.count * format * sizeof(float);

//        ch->dataPosition = dataPosition;
//        dataPosition += csize //channel size in bytes
//                        + 4; //channel format
//    }
}

void Data94File::read()
{DD;
    QFile f(fileName());

    if (!f.open(QFile::ReadOnly)) return;

    QDataStream r(&f);
    r.setByteOrder(QDataStream::LittleEndian);
    r.setFloatingPointPrecision(QDataStream::SinglePrecision);
    QString label = QString::fromLocal8Bit(r.device()->read(8));

    if (label != "data94  ") {
        LOG(ERROR)<<QString("файл неправильного типа");
        return;
    }

    //reading file description
    r >> descriptionSize;
    QByteArray descriptionBuffer = r.device()->read(descriptionSize);
    if ((quint32)descriptionBuffer.size() != descriptionSize) {
        LOG(ERROR)<<QString("не удалось прочитать описание файла");
        return;
    }

    QJsonParseError error;

    QJsonDocument doc = QJsonDocument::fromJson(descriptionBuffer, &error);
    if (error.error != QJsonParseError::NoError) {
        LOG(ERROR)<<QString("не удалось распознать описание файла:") << fileName() << error.errorString() << error.offset;
//        LOG(DEBUG)<<descriptionBuffer;
    }
    else setDataDescription(DataDescription::fromJson(doc.object()));

    // Исправление для файлов от Николая Ксенофонтовича:
    // если dateTime пуст, то копируем его из source.dateTime
    if (!dataDescription().dateTime().isValid())
        dataDescription().put("dateTime", dataDescription().get("source.dateTime"));

    //дальше - каналы
    quint32 channelsCount;
    r >> channelsCount;
    for (quint32 i = 0; i < channelsCount; ++i) {
        Data94Channel *c = new Data94Channel(this);
        c->read(r);
    }
}

void Data94File::write()
{DD;
    if (!changed() && !dataChanged()) return;

    QFile f(fileName());
    bool newFile = !f.exists();

    if (!f.open(QFile::ReadOnly) && !newFile) {
        LOG(ERROR)<<QString("Не удалось открыть файл для чтения");
        return;
    }

    QTemporaryFile temp;
    if (!temp.open()) {
        LOG(ERROR)<<QString("Не удалось открыть временный файл для записи");
        return;
    }

    temporaryFiles->add(temp.fileName());

    QDataStream r(&temp);
    r.setByteOrder(QDataStream::LittleEndian);
    r.setFloatingPointPrecision(QDataStream::SinglePrecision);

    QDataStream in(&f);
    in.setByteOrder(QDataStream::LittleEndian);
    in.setFloatingPointPrecision(QDataStream::SinglePrecision);

    //переписываем описательную часть
    r.device()->write("data94  ");

    QJsonDocument doc(dataDescription().toJson());
    QByteArray json = doc.toJson();
    descriptionSize = json.size();
    r << descriptionSize;
    r.writeRawData(json.data(), descriptionSize);
    r << quint32(channels.count());

    for (Data94Channel *c: qAsConst(channels)) {
        c->write(r, &in, c->data()); //данные берутся из самого канала
    }
    setDataChanged(false);
    setChanged(false);

    f.close();
    temp.close();

    if (QFile::remove(fileName()) || newFile) {
        if (!QFile::copy(temp.fileName(), fileName()))
            LOG(ERROR)<<QString("Не удалось сохранить файл")<<fileName();
    }
    else {
        LOG(ERROR)<<QString("Не удалось удалить исходный файл")<<fileName();
    }
}

int Data94File::channelsCount() const
{DD;
    return channels.size();
}

void Data94File::deleteChannels(const QVector<int> &channelsToDelete)
{DD;
    QTemporaryFile tempFile;
    QFile rawFile(fileName());

    if (!tempFile.open() || !rawFile.open(QFile::ReadOnly)) {
        LOG(ERROR)<<" Couldn't replace raw file with temp file.";
        return;
    }
    temporaryFiles->add(tempFile.fileName());

    QDataStream tempStream(&tempFile);
    tempStream.setByteOrder(QDataStream::LittleEndian);
    tempStream.setFloatingPointPrecision(QDataStream::SinglePrecision);

    QDataStream rawStream(&rawFile);
    rawStream.setByteOrder(QDataStream::LittleEndian);
    rawStream.setFloatingPointPrecision(QDataStream::SinglePrecision);

    //записываем шапку файла
    QByteArray buffer = rawStream.device()->read(8+4+descriptionSize);
    tempStream.device()->write(buffer);

    //записываем новое количество каналов
    quint32 ccount = channels.size() - channelsToDelete.size();
    tempStream << ccount;

    qint64 pos = tempStream.device()->pos();
    for (int i = 0; i < channels.size(); ++i) {
        // пропускаем канал, предназначенный для удаления
        if (channelsToDelete.contains(i)) continue;
        qint64 len = channels.at(i)->dataPosition - channels.at(i)->position;

        rawStream.device()->seek(channels.at(i)->position);
        buffer = rawStream.device()->read(channels.at(i)->size);
        tempStream.device()->write(buffer);

        //обновляем положение каналов в файле
        channels.at(i)->position = pos;
        channels.at(i)->dataPosition = pos+len;
        pos += buffer.size();
    }

    rawFile.close();
    tempFile.close();

    if (QFile::remove(fileName())) {
        if (!QFile::copy(tempFile.fileName(), fileName())) {
            LOG(ERROR)<<QString("Не удалось сохранить файл")<<fileName();
            return;
        }
    }
    else {
        LOG(ERROR)<<QString("Не удалось удалить исходный файл")<<fileName();
        return;
    }

    for (int i=channels.size()-1; i>=0; --i) {
        if (channelsToDelete.contains(i)) {
            delete channels.takeAt(i);
        }
    }
}

void Data94File::copyChannelsFrom(const QVector<Channel *> &source)
{DD;
    const int count = channelsCount();

    QFile f(fileName());
    if (!f.open(QFile::ReadWrite)) {
        LOG(ERROR)<<QString("Не удалось открыть файл для записи");
        return;
    }
    QDataStream r(&f);
    r.setByteOrder(QDataStream::LittleEndian);
    r.setFloatingPointPrecision(QDataStream::SinglePrecision);

    //записываем новое количество каналов
    r.device()->seek(8+4+descriptionSize);
    quint32 ccount = count + source.size();
    r << ccount;

    //имеющийся размер файла
    qint64 pos = 8+4+descriptionSize+4;
    for (auto c: qAsConst(channels))
        pos += c->size;

    Q_ASSERT_X(pos == f.size(), "copyChannelsFrom", "maximum pos must equal to file size");

    r.device()->seek(pos);

    for (auto c: qAsConst(source)) {
        Data94Channel *destChannel = new Data94Channel(c, this);
        destChannel->setChanged(true);
        destChannel->setDataChanged(true);

        bool populated = c->populated();
        if (!populated) c->populate();

        destChannel->write(r, 0, c->data()); //данные берутся из sourceChannel

        if (!populated)
            c->clear();
    }
}

void Data94File::addChannelWithData(DataHolder *data, const DataDescription &description)
{DD;
    Data94Channel *ch = new Data94Channel(this);
    ch->setPopulated(true);
    ch->setChanged(true);
    ch->setDataChanged(true);
    ch->setData(data);
    ch->dataDescription() = description;

    ch->initFrom(data, description);
}

void Data94File::move(bool up, const QVector<int> &indexes, const QVector<int> &newIndexes)
{DD;
    // заполняем вектор индексов каналов, как они будут выглядеть после перемещения
    const int count = channelsCount();
    QVector<int> indexesVector(count);
    for (int i=0; i<count; ++i) indexesVector[i] = i;

    {int i=up?0:indexes.size()-1;
    while (1) {
        indexesVector.move(indexes.at(i),newIndexes.at(i));
        if ((up && i==indexes.size()-1) || (!up && i==0)) break;
        i=up?i+1:i-1;
    }}

    QTemporaryFile temp;
    if (!temp.open()) {
        LOG(ERROR)<<"Couldn't open temp file to write";
        return;
    }

    temporaryFiles->add(temp.fileName());

    QDataStream out(&temp);
    out.setByteOrder(QDataStream::LittleEndian);
    out.setFloatingPointPrecision(QDataStream::SinglePrecision);

    QFile f(fileName());
    if (!f.open(QFile::ReadOnly)) {
        LOG(ERROR)<<"Couldn't open file to write";
        return;
    }
    QDataStream in(&f);

    //переписываем описательную часть
    out.device()->write("data94  ");
    in.device()->seek(8);
    QByteArray buffer = in.device()->read(4 + descriptionSize);
    out.device()->write(buffer);

    quint32 ccount = channels.count();
    out << ccount;

    for (int i: qAsConst(indexesVector)) {
        qint64 pos = out.device()->pos();

        Data94Channel *f = channels.at(i);

        in.device()->seek(f->position);
        QByteArray buffer = in.device()->read(f->size);
        out.device()->write(buffer);

        f->position = pos;
        f->dataPosition = pos + 8 + 4 + f->descriptionSize + f->xAxisBlock.size()+
                          f->zAxisBlock.size() + 4 + 4;
    }

    temp.close();

    int i=up?0:indexes.size()-1;
    while (1) {
        channels.move(indexes.at(i),newIndexes.at(i));
        if ((up && i==indexes.size()-1) || (!up && i==0)) break;
        i=up?i+1:i-1;
    }

    QFile::remove(fileName());
    temp.copy(fileName());
}

Channel *Data94File::channel(int index) const
{DD;
    if (channels.size()>index && index>=0)
        return channels.at(index);
    return nullptr;
}

QStringList Data94File::fileFilters()
{DD;
    return QStringList()<<"Файлы Data94 (*.d94)";
}

QStringList Data94File::suffixes()
{DD;
    return QStringList()<<"*.d94";
}


/*Data94Channel implementation*/

Data94Channel::Data94Channel(Data94File *parent) : Channel(),
    parent(parent)
{DD;
    parent->channels << this;
}

Data94Channel::Data94Channel(Data94Channel *other, Data94File *parent)
    : Channel(other), parent(parent)
{DD;
    isComplex = other->isComplex;
    sampleWidth = other->sampleWidth;
    xAxisBlock = other->xAxisBlock;
    zAxisBlock = other->zAxisBlock;
    descriptionSize = other->descriptionSize;
    parent->channels << this;
}

Data94Channel::Data94Channel(Channel *other, Data94File *parent)
    : Channel(other), parent(parent)
{DD;
    parent->channels << this;
    initFrom(other->data(), other->dataDescription());
}

void Data94Channel::initFrom(DataHolder *data, const DataDescription &description)
{DD;
    isComplex = data->yValuesFormat() == DataHolder::YValuesComplex;

    //xAxisBlock
    xAxisBlock.uniform = data->xValuesFormat() == DataHolder::XValuesUniform ? 1:0;
    xAxisBlock.begin = data->xMin();
    if (xAxisBlock.uniform == 0) // not uniform
        xAxisBlock.values = data->xValues();
    xAxisBlock.count = data->samplesCount();
    xAxisBlock.step = data->xStep();

    //zAxisBlock
    zAxisBlock.uniform = data->zValuesFormat() == DataHolder::XValuesUniform ? 1:0;
    zAxisBlock.count = data->blocksCount();
    zAxisBlock.begin = data->zMin();
    zAxisBlock.step = data->zStep();
    if (zAxisBlock.uniform == 0) // not uniform
        zAxisBlock.values = data->zValues();
    if (zAxisBlock.values.isEmpty() && !zAxisBlock.uniform) {
        for (uint i=0; i<zAxisBlock.count; ++i) zAxisBlock.values << i;
    }

    if (xAxisBlock.uniform == 1 && !qFuzzyIsNull(xAxisBlock.step))
        dataDescription().put("samplerate", int(1.0 / xAxisBlock.step));

    //по умолчанию точность float
    //int8 / uint8 / int16 / uint16 / int32 / uint32 / int64 / uint64 / float / double
    QString  precision = description.get("function.precision").toString();
    if (precision.isEmpty()) precision = "float";
    if (precision.endsWith("int8")) sampleWidth = 1;
    else if (precision.endsWith("int16")) sampleWidth = 2;
    else if (precision.endsWith("int64") || precision=="double") sampleWidth = 8;
    else sampleWidth = 4;
}



void Data94Channel::read(QDataStream &r)
{DD;
    if (r.status() != QDataStream::Ok) return;
    position = r.device()->pos();
//    LOG(DEBUG)<<"Reading at"<<position;
    QString label = QString::fromLocal8Bit(r.device()->read(8));

    if (label != "d94chan ") {
        LOG(ERROR)<<QString("канал неправильного типа");
        return;
    }

    //reading file description
    r >> descriptionSize;

    QByteArray descriptionBuffer = r.device()->read(descriptionSize);
    if ((quint32)descriptionBuffer.size() != descriptionSize) {
        LOG(ERROR)<<QString("не удалось прочитать описание канала");
        return;
    }

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(descriptionBuffer, &error);
    if (error.error != QJsonParseError::NoError) {
        LOG(ERROR)<<QString("не удалось распознать описание канала:")<<error.errorString() << error.offset;
    }
    else dataDescription() = DataDescription::fromJson(doc.object());

    //reading xAxisBlock
    xAxisBlock.read(r);
    //reading zAxisBlock
    zAxisBlock.read(r);

    quint32 valueFormat;
    r >> valueFormat;

    isComplex = valueFormat == 2;

    r >> sampleWidth; // 1, 2, 4 или 8
//    LOG(DEBUG)<<"valueFormat"<<valueFormat<<"samplewidth"<<sampleWidth;

    dataPosition = r.device()->pos();

    double thr = dataDescription().get("function.logref").toDouble();
    if (qFuzzyIsNull(thr)) thr = PhysicalUnits::Units::logref(yName());
    _data->setThreshold(thr);
    _data->setYValuesUnits(DataHolder::unitsFromString(dataDescription().get("function.logscale").toString()));
    _data->setYValuesFormat(DataHolder::formatFromString(dataDescription().get("function.format").toString()));

    if (xAxisBlock.uniform == 1)
        _data->setXValues(xAxisBlock.begin, xAxisBlock.step, xAxisBlock.count);
    else
        _data->setXValues(xAxisBlock.values);

    if (zAxisBlock.uniform == 1)
        _data->setZValues(zAxisBlock.begin, zAxisBlock.step, zAxisBlock.count);
    else
        _data->setZValues(zAxisBlock.values);

    const qint64 dataToSkip = zAxisBlock.count * xAxisBlock.count * valueFormat * sampleWidth;
    // соответствия:             blockCount        sampleCount         factor     1, 2, 4 или 8
    r.device()->skip(dataToSkip);


    size = r.device()->pos() - position;
//    LOG(DEBUG)<<"actual size"<<size;

    const qint64 requiredSize = 8 + 4 + descriptionSize + xAxisBlock.size() + zAxisBlock.size() +
                          4 + 4 + dataToSkip;
//    LOG(DEBUG)<<"required size"<<requiredSize;
//    LOG(DEBUG)<<"pos"<<hex<<r.device()->pos();

    if (size != requiredSize)
        LOG(WARNING)<<"Strange channel sizeBytes: should be"<<requiredSize<<"got"<<size;
}

void Data94Channel::write(QDataStream &r, QDataStream *in, DataHolder *data)
{DD;
    const auto precision = dataDescription().get("function.precision").toString();
    if (precision == "float")
        r.setFloatingPointPrecision(QDataStream::SinglePrecision);
    else
        r.setFloatingPointPrecision(QDataStream::DoublePrecision);

    qint64 newposition = r.device()->pos();
    r.device()->write("d94chan ");

    //description
    if (changed()) {
        //переписываем описатель

        QJsonDocument doc(dataDescription().toJson());
        QByteArray json = doc.toJson();
        descriptionSize = json.size();
        r << descriptionSize;
        r.writeRawData(json.data(), descriptionSize);
    }
    else if (in) {
        //просто копируем описатель из исходного файла
        in->device()->seek(position+8);
        QByteArray buf = in->device()->read(4+descriptionSize);
        r.writeRawData(buf.data(), buf.size());
    }

    xAxisBlock.write(r);
    zAxisBlock.write(r);

    const quint32 format = data->yValuesFormat()==DataHolder::YValuesComplex ? 2 : 1;
    isComplex = data->yValuesFormat()==DataHolder::YValuesComplex;
    r << format;
    r << sampleWidth;

    setChanged(false);

    qint64 newdataposition = r.device()->pos();
    if (dataChanged()) {
        if (sampleWidth == 8)
            r.setFloatingPointPrecision(QDataStream::DoublePrecision);

        for (int block = 0; block < data->blocksCount(); ++block) {
            if (!isComplex) {
                const QVector<double> yValues = data->rawYValues(block);
                if (yValues.isEmpty()) {
                    LOG(ERROR)<<QString("Отсутствуют данные для записи в канале")<<name();
                    continue;
                }

                for (double v: qAsConst(yValues)) {
                    if (precision=="int8")        r << (qint8)v;
                    else if (precision=="uint8")  r << (quint8)v;
                    else if (precision=="int16")  r << (qint16)v;
                    else if (precision=="uint16") r << (quint16)v;
                    else if (precision=="uint32") r << (quint32)v;
                    else if (precision=="int32")  r << (qint32)v;
                    else if (precision=="int64")  r << (qint64)v;
                    else if (precision=="uint64") r << (quint64)v;
                    else if (precision=="float") r << (float)v;
                    else r << v;
                }
            } // !c->isComplex
            else {
                const auto yValues = data->yValuesComplex(block);
                if (yValues.isEmpty()) {
                    LOG(ERROR)<<QString("Отсутствуют данные для записи в канале")<<name();
                    continue;
                }
                for (cx_double v: qAsConst(yValues)) {
                    if (precision=="int8")        {r << (qint8)v.real(); r << (qint8)v.imag();}
                    else if (precision=="uint8")  {r << (quint8)v.real(); r << (quint8)v.imag();}
                    else if (precision=="int16")  {r << (qint16)v.real(); r << (qint16)v.imag();}
                    else if (precision=="uint16") {r << (quint16)v.real(); r << (quint16)v.imag();}
                    else if (precision=="uint32") {r << (quint32)v.real(); r << (quint32)v.imag();}
                    else if (precision=="int32")  {r << (qint32)v.real(); r << (qint32)v.imag();}
                    else if (precision=="int64")  {r << (qint64)v.real(); r << (qint64)v.imag();}
                    else if (precision=="uint64") {r << (quint64)v.real(); r << (quint64)v.imag();}
                    else if (precision=="float")  {r << (float)v.real(); r << (float)v.imag();}
                    else {
                        r << v.real();
                        r << v.imag();
                    }
                }
            } // c->isComplex
        }
    }
    else if (in) {
        //просто копируем описатель из исходного файла
        in->device()->seek(dataPosition);
        QByteArray buf = in->device()->read(zAxisBlock.count * xAxisBlock.count *
                                           format * sampleWidth);
        r.writeRawData(buf.data(), buf.size());
    }
    setDataChanged(false);
    dataPosition = newdataposition;
    position = newposition;
    size = r.device()->pos() - position;
}

void Data94Channel::setXStep(double xStep)
{DD;
    Channel::setXStep(xStep);
    xAxisBlock.step = float(xStep);
}

Descriptor::DataType Data94Channel::type() const
{DD;
    return Descriptor::DataType(dataDescription().get("function.type").toInt());
}

void Data94Channel::populate()
{DD;
    _data->clear();
    setPopulated(false);

    auto precision = toDataPrecision(dataDescription().get("function.precision").toString());

    QFile rawFile(parent->fileName());

    if (rawFile.open(QFile::ReadOnly)) {
        QVector<double> YValues;

        //количество отсчетов в одном блоке - удваивается, если данные комплексные
        const quint64 blockSize = xAxisBlock.count * (isComplex ? 2 : 1);
        const quint64 fullDataSize = zAxisBlock.count * blockSize;

        if (dataPosition < 0) {
            LOG(ERROR)<<QString("Поврежденный файл: не удалось найти положение данных в файле");
        }
        else {
            // map file into memory
            unsigned char *ptr = rawFile.map(dataPosition, fullDataSize * sampleWidth);
            if (ptr) {//достаточно памяти отобразить
                unsigned char *maxPtr = ptr + rawFile.size();
                unsigned char *ptrCurrent = ptr;
                if (dataPosition >= 0) {
                    YValues = convertFrom<double>(ptrCurrent,
                                                  qMin(quint64(maxPtr-ptrCurrent), quint64(fullDataSize * sampleWidth)),
                                                  precision);
                }
            }
            else {
                //читаем классическим способом
                QDataStream readStream(&rawFile);
                readStream.setByteOrder(QDataStream::LittleEndian);
                readStream.setFloatingPointPrecision(sampleWidth == 8 ? QDataStream::DoublePrecision :
                                                                        QDataStream::SinglePrecision);

                readStream.device()->seek(dataPosition);
                YValues = getChunkOfData<double>(readStream, fullDataSize, precision, 0);
            }
        }

        //меняем размер, если не удалось прочитать весь блок данных
        YValues.resize(fullDataSize);

        if (isComplex) {
            QVector<cx_double> co(xAxisBlock.count * zAxisBlock.count);
            for (uint i=0; i < xAxisBlock.count * zAxisBlock.count; ++i)
                co[i] = {YValues.at(i*2), YValues.at(i*2+1)};

            _data->setYValues(co, -1);
        }
        else
            _data->setYValues(YValues, _data->yValuesFormat(), -1);

        setPopulated(true);
        rawFile.close();

        if (!xAxisBlock.values.isEmpty()) {//данные по оси Х
            _data->setXValues(xAxisBlock.values);
        }
    }
    else {
        LOG(ERROR)<<QString("Не удалось открыть файл")<<parent->fileName();
    }
}


FileDescriptor *Data94Channel::descriptor() const
{DD;
    return parent;
}

int Data94Channel::index() const
{DD;
    if (parent) return parent->channels.indexOf(const_cast<Data94Channel*>(this));
    return -1;
}

void AxisBlock::read(QDataStream &r)
{DD;
    if (r.status() != QDataStream::Ok) return;

    r.setFloatingPointPrecision(QDataStream::SinglePrecision);

    r >> uniform; //0 - шкала неравномерная, 1 - шкала равномерная
    r >> count;

    if (uniform > 0) {
        r >> begin;
        r >> step;
    }
    else {
        qint64 actuallyRead = 0;
        values = getChunkOfData<double>(r, count, DataPrecision::Float, &actuallyRead);
        if (uint(actuallyRead) != count) values.resize(count);
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
        for (double x: qAsConst(values)) r << float(x);
    }
}

quint32 AxisBlock::size() const
{DD;
    if (uniform) return 16;

    return 8 + values.size() * 4;
}
