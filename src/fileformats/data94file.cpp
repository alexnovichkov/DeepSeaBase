#include "data94file.h"

#include <QJsonDocument>
#include "algorithms.h"
#include "logging.h"
#include "unitsconverter.h"

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

Data94File::Data94File(const QString &fileName) : FileDescriptor(fileName)
{

}

Data94File::Data94File(const FileDescriptor &other, const QString &fileName, QVector<int> indexes)
    : FileDescriptor(fileName)
{
    QVector<Channel *> source;
    if (indexes.isEmpty())
        for (int i=0; i<other.channelsCount(); ++i) source << other.channel(i);
    else
        for (int i: indexes) source << other.channel(i);

    init(source);
}

Data94File::Data94File(const QVector<Channel *> &source, const QString &fileName)
 : FileDescriptor(fileName)
{
    init(source);
}

void Data94File::init(const QVector<Channel *> &source)
{
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
        qDebug()<<"Не удалось открыть файл для записи:"<<fileName();
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
{
    if (changed() || dataChanged())
        write();

    qDeleteAll(channels);
}

void Data94File::updatePositions()
{
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
    setDataDescription(DataDescription::fromJson(doc.object()));

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
        qDebug()<<"Не удалось открыть файл для чтения";
        return;
    }

    QTemporaryFile temp;
    if (!temp.open()) {
        qDebug()<<"Не удалось открыть временный файл для записи";
        return;
    }

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
            qDebug()<<"Не удалось сохранить файл"<<fileName();
    }
    else {
        qDebug()<<"Не удалось удалить исходный файл"<<fileName();
    }
}

int Data94File::channelsCount() const
{
    return channels.size();
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
            qDebug()<<"Не удалось сохранить файл"<<fileName();
            return;
        }
    }
    else {
        qDebug()<<"Не удалось удалить исходный файл"<<fileName();
        return;
    }

    for (int i=channels.size()-1; i>=0; --i) {
        if (channelsToDelete.contains(i)) {
            delete channels.takeAt(i);
        }
    }
}

void Data94File::copyChannelsFrom(const QVector<Channel *> &source)
{
    const int count = channelsCount();

    QFile f(fileName());
    if (!f.open(QFile::ReadWrite)) {
        qDebug()<<"Не удалось открыть файл для записи";
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
{
    Data94Channel *ch = new Data94Channel(this);
    ch->setPopulated(true);
    ch->setChanged(true);
    ch->setDataChanged(true);
    ch->setData(data);
    ch->dataDescription() = description;

    //Заполнение данными
    //xAxisBlock
    ch->xAxisBlock.uniform = data->xValuesFormat() == DataHolder::XValuesUniform ? 1:0;
    ch->xAxisBlock.begin = data->xMin();
    if (ch->xAxisBlock.uniform == 0) {// not uniform
        ch->xAxisBlock.values = data->xValues();
        ch->xAxisBlock.values.resize(data->samplesCount());
    }

    ch->xAxisBlock.count = data->samplesCount();
    ch->xAxisBlock.step = data->xStep();

    //zAxisBlock
    ch->zAxisBlock.count = 1;

    ch->isComplex = data->yValuesFormat() == DataHolder::YValuesComplex;

    if (ch->xAxisBlock.uniform == 1 && !qFuzzyIsNull(ch->xAxisBlock.step))
        ch->dataDescription().put("samplerate", int(1.0 / ch->xAxisBlock.step));
}

void Data94File::move(bool up, const QVector<int> &indexes, const QVector<int> &newIndexes)
{
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
        qDebug()<<"Couldn't open temp file to write";
        return;
    }

    QDataStream out(&temp);
    out.setByteOrder(QDataStream::LittleEndian);
    out.setFloatingPointPrecision(QDataStream::SinglePrecision);

    QFile f(fileName());
    if (!f.open(QFile::ReadOnly)) {
        qDebug()<<"Couldn't open file to write";
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
{
    if (channels.size()>index)
        return channels.at(index);
    return nullptr;
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
    parent->channels << this;
}

Data94Channel::Data94Channel(Data94Channel *other, Data94File *parent)
    : Channel(other), parent(parent)
{
    isComplex = other->isComplex;
    sampleWidth = other->sampleWidth;
    xAxisBlock = other->xAxisBlock;
    zAxisBlock = other->zAxisBlock;
    descriptionSize = other->descriptionSize;
    parent->channels << this;
}

Data94Channel::Data94Channel(Channel *other, Data94File *parent)
    : Channel(other), parent(parent)
{
    parent->channels << this;
    isComplex = other->data()->yValuesFormat() == DataHolder::YValuesComplex;

    //xAxisBlock
    xAxisBlock.uniform = other->data()->xValuesFormat() == DataHolder::XValuesUniform ? 1:0;
    xAxisBlock.begin = other->data()->xMin();
    if (xAxisBlock.uniform == 0) // not uniform
        xAxisBlock.values = other->data()->xValues();
    xAxisBlock.count = other->data()->samplesCount();
    xAxisBlock.step = other->data()->xStep();

    //zAxisBlock
    zAxisBlock.uniform = other->data()->zValuesFormat() == DataHolder::XValuesUniform ? 1:0;
    zAxisBlock.count = other->data()->blocksCount();
    zAxisBlock.begin = other->data()->zMin();
    zAxisBlock.step = other->data()->zStep();
    if (zAxisBlock.uniform == 0) // not uniform
        zAxisBlock.values = other->data()->zValues();
    if (zAxisBlock.values.isEmpty() && !zAxisBlock.uniform) {
        for (uint i=0; i<zAxisBlock.count; ++i) zAxisBlock.values << i;
    }
//    qDebug()<<zAxisBlock;

    //по умолчанию точность float
    //int8 / uint8 / int16 / uint16 / int32 / uint32 / int64 / uint64 / float / double
    QString  precision = other->dataDescription().get("function.precision").toString();
    if (precision.isEmpty()) precision = "float";
    if (precision == "int64" || precision=="uint64" || precision=="double")
        sampleWidth = 8;
    else
        sampleWidth = 4;
}

void Data94Channel::read(QDataStream &r)
{
    if (r.status() != QDataStream::Ok) return;
    position = r.device()->pos();
//    qDebug()<<"Reading at"<<position;
    QString label = QString::fromLocal8Bit(r.device()->read(8));

    if (label != "d94chan ") {
        qDebug()<<"канал неправильного типа";
        return;
    }

    //reading file description
    r >> descriptionSize;

    QByteArray descriptionBuffer = r.device()->read(descriptionSize);
    if ((quint32)descriptionBuffer.size() != descriptionSize) {
        qDebug()<<"не удалось прочитать описание канала";
        return;
    }
//    qDebug() << descriptionBuffer;
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(descriptionBuffer, &error);
    if (error.error != QJsonParseError::NoError) {
        qDebug()<<error.errorString() << error.offset;
        return;
    }
    dataDescription() = DataDescription::fromJson(doc.object());

    //reading xAxisBlock
    xAxisBlock.read(r);
//    qDebug()<<"xAxisBlock"<<xAxisBlock;

    //reading zAxisBlock
    zAxisBlock.read(r);
//    qDebug()<<"zAxisBlock"<<zAxisBlock;

    quint32 valueFormat;
    r >> valueFormat;

    isComplex = valueFormat == 2;

    r >> sampleWidth; // 4 или 8
//    qDebug()<<"valueFormat"<<valueFormat<<"samplewidth"<<sampleWidth;

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
    // соответствия:             blockCount        sampleCount         factor        4 или 8
    r.device()->skip(dataToSkip);


    size = r.device()->pos() - position;
//    qDebug()<<"actual size"<<size;

    const qint64 requiredSize = 8 + 4 + descriptionSize + xAxisBlock.size() + zAxisBlock.size() +
                          4 + 4 + dataToSkip;
//    qDebug()<<"required size"<<requiredSize;
//    qDebug()<<"pos"<<hex<<r.device()->pos();

    if (size != requiredSize)
        qDebug()<<"Strange channel sizeBytes: should be"<<requiredSize<<"got"<<size;
}

void Data94Channel::write(QDataStream &r, QDataStream *in, DataHolder *data)
{
    r.setFloatingPointPrecision(QDataStream::SinglePrecision);
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

    qint64 newdataposition = r.device()->pos();;
    if (dataChanged()) {
        if (sampleWidth == 8)
            r.setFloatingPointPrecision(QDataStream::DoublePrecision);

        for (int block = 0; block < data->blocksCount(); ++block) {
            if (!isComplex) {
                const QVector<double> yValues = data->rawYValues(block);
                if (yValues.isEmpty()) {
                    qDebug()<<"Отсутствуют данные для записи в канале"<<name();
                    continue;
                }

                for (double v: qAsConst(yValues)) {
                    if (sampleWidth == 4)
                        r << (float)v;
                    else
                        r << v;
                }
            } // !c->isComplex
            else {
                const auto yValues = data->yValuesComplex(block);
                if (yValues.isEmpty()) {
                    qDebug()<<"Отсутствуют данные для записи в канале"<<name();
                    continue;
                }
                for (cx_double v: qAsConst(yValues)) {
                    if (sampleWidth == 4) {
                        r << (float)v.real();
                        r << (float)v.imag();
                    }
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
{
    Channel::setXStep(xStep);
    xAxisBlock.step = float(xStep);
}

Descriptor::DataType Data94Channel::type() const
{
    return Descriptor::DataType(dataDescription().get("function.type").toInt());
}

void Data94Channel::populate()
{
    _data->clear();
    setPopulated(false);

    QFile rawFile(parent->fileName());

    if (rawFile.open(QFile::ReadOnly)) {
        QVector<double> YValues;

        //количество отсчетов в одном блоке - удваивается, если данные комплексные
        const quint64 blockSize = xAxisBlock.count * (isComplex ? 2 : 1);
        const quint64 fullDataSize = zAxisBlock.count * blockSize;

        if (dataPosition < 0) {
            qDebug()<<"Поврежденный файл: не удалось найти положение данных в файле";
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
                                                  0xc0000000+sampleWidth);
                }
            }
            else {
                //читаем классическим способом
                QDataStream readStream(&rawFile);
                readStream.setByteOrder(QDataStream::LittleEndian);
                readStream.setFloatingPointPrecision(sampleWidth == 4 ? QDataStream::SinglePrecision :
                                                                        QDataStream::DoublePrecision);

                readStream.device()->seek(dataPosition);
                YValues = getChunkOfData<double>(readStream, fullDataSize, 0xc0000000 + sampleWidth, 0);
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
        qDebug()<<"Не удалось открыть файл"<<parent->fileName();
    }
}


FileDescriptor *Data94Channel::descriptor() const
{
    return parent;
}

int Data94Channel::index() const
{
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
        values = getChunkOfData<double>(r, count, 0xC0000004, &actuallyRead);
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
{
    if (uniform) return 16;

    return 8 + values.size() * 4;
}
