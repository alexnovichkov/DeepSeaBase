#include "data94file.h"

#include <QJsonDocument>
#include "algorithms.h"
#include "logging.h"

Data94File::Data94File(const QString &fileName) : FileDescriptor(fileName)
{

}

Data94File::Data94File(const Data94File &d) : FileDescriptor(d.fileName())
{
    this->description = d.description;
    updateDateTimeGUID();

    foreach (Data94Channel *f, d.channels) {
        this->channels << new Data94Channel(*f);
    }

    foreach (Data94Channel *f, channels) {
        f->parent = this;
    }
}

Data94File::Data94File(const FileDescriptor &other) : FileDescriptor(other.fileName())
{
    updateDateTimeGUID();
    description.insert("sourceFile", other.fileName());
    description.insert("legend", other.legend());
    setDataDescriptor(other.dataDescriptor());

    if (other.channelsCount()>0) {
        xAxisBlock.uniform = other.channel(0)->data()->xValuesFormat() == DataHolder::XValuesUniform ? 1:0;
        xAxisBlock.begin = other.channel(0)->xMin();
        xAxisBlock.values = other.channel(0)->xValues();
    }
    xAxisBlock.count = other.samplesCount();
    xAxisBlock.step = other.xStep();
    xAxisBlock.isValid = true;

    //zAxisBlock

    //channels
    for (int i=0; i<other.channelsCount(); +i) {
        Data94Channel *ch = new Data94Channel(other.channel(i));
        ch->parent = this;
        channels << ch;
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
    for (quint32 i = 0; i < channelsCount; ++i) {
        Data94Channel *c = new Data94Channel(this);
        c->_description = description.value("channels").toArray().takeAt(i).toObject();
        c->read(r);
        channels << c;
    }
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
        quint32 newDescriptionSize = json.size();
        quint32 replace = descriptionSize + paddingSize;
        //сначала записываем размер данных
        r.device()->seek(8);
        r << newDescriptionSize;
        descriptionSize = newDescriptionSize;

        //определяем, хватает ли нам паддинга
        if (newDescriptionSize < replace) {
            //можем записывать прямо поверх, обновляя паддинг
            paddingSize = replace - newDescriptionSize;

            r.writeRawData(json.data(), newDescriptionSize);
            r << paddingSize;
            r << QByteArray(paddingSize);
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
            QByteArray aboutToOverwrite(bufferLength);

            while (true) {
                // Seek to the current read position and read the data that we're about
                // to overwrite.  Appropriately increment the readPosition.

                r.device()->seek(readPosition);
                const size_t bytesRead;

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
        }

        //записываем блоки осей
        r.device()->seek(12+descriptionSize+4+paddingSize);
        xAxisBlock.write(r);
        zAxisBlock.write(r);
        r << quint32(channels.count());
    }

    if (dataChanged()) {
        r.device()->seek(12+descriptionSize+4+paddingSize
                         + xAxisBlock.size()+zAxisBlock.size()+4);
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

Descriptor::DataType Data94File::type() const
{
    if (channels.isEmpty()) return Descriptor::Unknown;

    Descriptor::DataType t = channels.first()->type();
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
        result.insert(data[i].first, data[i].second);
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
}

void Data94File::copyChannelsFrom(FileDescriptor *, const QVector<int> &)
{
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
        return channels[index];
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

double Data94File::xStep() const
{
    if (!channels.isEmpty()) return channels.first()->xStep();
    return 0.0;
}

void Data94File::setXStep(const double xStep)
{
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
    }
}

QString Data94File::xName() const
{
    if (channels.isEmpty()) return QString();

    QString xname = channels.first()->xName();

    for (int i=1; i<channels.size(); ++i) {
        if (channels[i]->xName() != xname) return QString();
    }
    return xname;
}

bool Data94File::dataTypeEquals(FileDescriptor *other) const
{
    return (this->type() == other->type());
}

QString Data94File::fileFilters() const
{
    return "Файлы Data94 (*.d94)";
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
        case DataHolder::UnitsUnknown:
        case DataHolder::UnitsLinear: units = "quadratic"; break;
        case DataHolder::UnitsQuadratic: units = "linear"; break;
        case DataHolder::UnitsDimensionless: units = "dimensionless"; break;
        default: break;
    }
    function.insert("logscale", units);
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
    dataPosition = -1;
}

void Data94Channel::read(QDataStream &r)
{
    if (r.status() != QDataStream::Ok) return;
    quint32 valueFormat;
    r >> valueFormat;

    isComplex = valueFormat == 1;

    dataPosition = r.device()->pos();

    double thr = _description.value("function").toObject().value("logref").toDouble();
    if (thr == 0.0)
        thr = threshold(yName());
    _data->setThreshold(thr);

    int units = DataHolder::UnitsLinear;
    QString unitsS = _description.value("function").toObject().value("logref").toString();
    if (unitsS == "quadratic") units = DataHolder::UnitsQuadratic;
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

    r.device()->skip(parent->zAxisBlock.count * parent->xAxisBlock.count * (isComplex?2:1) * sizeof(float));
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

void Data94Channel::populate()
{
    ///Сейчас программа не поддерживает чтение нескольких блоков данных,
    /// поэтому читаем только первый блок

    // clear previous data;
    if (populated()) return;
    _data->clear();

    setPopulated(false);


    QFile rawFile(parent->fileName());

//    QTime time;
//    time.start();

    if (rawFile.open(QFile::ReadOnly)) {
        QVector<double> YValues;
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
            _data->setYValues(YValues, DataHolder::YValuesFormat(_data->yValuesFormat()));

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
