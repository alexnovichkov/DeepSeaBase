#include "data94file.h"

#include <QJsonDocument>
#include "algorithms.h"
#include "logging.h"
#include "averaging.h"

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

Data94File::Data94File(const Data94File &other, const QString &fileName, QVector<int> indexes)
    : FileDescriptor(fileName)
{
    this->description = other.description;
    this->descriptionSize = other.descriptionSize;
    description.insert("sourceFile", other.fileName());

    updateDateTimeGUID();

    //если индексы пустые - копируем все каналы
    if (indexes.isEmpty())
        for (int i=0; i<other.channelsCount(); ++i) indexes << i;

    QFile f(fileName);
    if (!f.open(QFile::WriteOnly)) {
        qDebug()<<"Не удалось открыть файл для записи:"<<fileName;
        return;
    }

    QDataStream r(&f);
    r.setByteOrder(QDataStream::LittleEndian);
    r.setFloatingPointPrecision(QDataStream::SinglePrecision);

    //переписываем описательную часть
    r.device()->write("data94  ");

    QJsonDocument doc(description);
    QByteArray json = doc.toJson();
    descriptionSize = json.size();
    r << descriptionSize;
    r.writeRawData(json.data(), descriptionSize);

    quint32 ccount = indexes.size();
    r << ccount;

    for (int i: indexes) {
        Data94Channel *sourceChannel = other.channels.at(i);
        Data94Channel *c = new Data94Channel(sourceChannel, this);

        bool populated = sourceChannel->populated();
        if (!populated) sourceChannel->populate();

        r.setFloatingPointPrecision(QDataStream::SinglePrecision);
        qint64 pos = r.device()->pos();
        r.device()->write("d94chan ");

        //description
        QJsonDocument doc(c->_description);
        QByteArray json = doc.toJson();
        c->descriptionSize = json.size();
        r << c->descriptionSize;
        r.writeRawData(json.data(), c->descriptionSize);

        c->position = pos;

        c->xAxisBlock.write(r);
        c->zAxisBlock.write(r);

        const quint32 format = c->isComplex ? 2 : 1;
        r << format;
        r << c->sampleWidth;

        if (c->sampleWidth == 8)
            r.setFloatingPointPrecision(QDataStream::DoublePrecision);

        c->dataPosition = r.device()->pos();
        for (int block = 0; block < c->data()->blocksCount(); ++block) {
            if (!c->isComplex) {
                const QVector<double> yValues = sourceChannel->data()->rawYValues(block);
                if (yValues.isEmpty()) {
                    qDebug()<<"Отсутствуют данные для записи в канале"<<c->name();
                    continue;
                }

                for (double v: yValues) {
                    if (c->sampleWidth == 4)
                        r << (float)v;
                    else
                        r << v;
                }
            } // !c->isComplex
            else {
                const auto yValues = sourceChannel->data()->yValuesComplex(block);
                if (yValues.isEmpty()) {
                    qDebug()<<"Отсутствуют данные для записи в канале"<<c->name();
                    continue;
                }
                for (cx_double v: yValues) {
                    if (c->sampleWidth == 4) {
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
        c->size = r.device()->pos() - c->position;

        if (!populated) {
            sourceChannel->clear();
        }
    }
}

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

    QFile f(fileName);
    if (!f.open(QFile::WriteOnly)) {
        qDebug()<<"Не удалось открыть файл для записи:"<<fileName;
        return;
    }

    QDataStream r(&f);
    r.setByteOrder(QDataStream::LittleEndian);
    r.setFloatingPointPrecision(QDataStream::SinglePrecision);

    //переписываем описательную часть
    r.device()->write("data94  ");

    QJsonDocument doc(description);
    QByteArray json = doc.toJson();
    descriptionSize = json.size();
    r << descriptionSize;
    r.writeRawData(json.data(), descriptionSize);

    quint32 ccount = indexes.size();
    r << ccount;

    for (int i: indexes) {
        Channel *sourceChannel = other.channel(i);
        Data94Channel *c = new Data94Channel(sourceChannel, this);

        bool populated = sourceChannel->populated();
        if (!populated) sourceChannel->populate();

        r.setFloatingPointPrecision(QDataStream::SinglePrecision);
        qint64 pos = r.device()->pos();
        r.device()->write("d94chan ");

        //description
        QJsonDocument doc(c->_description);
        QByteArray json = doc.toJson();
        c->descriptionSize = json.size();
        r << c->descriptionSize;
        r.writeRawData(json.data(), c->descriptionSize);

        c->position = pos;

        c->xAxisBlock.write(r);
        c->zAxisBlock.write(r);

        const quint32 format = c->isComplex ? 2 : 1;
        r << format;
        r << c->sampleWidth;

        if (c->sampleWidth == 8)
            r.setFloatingPointPrecision(QDataStream::DoublePrecision);

        c->dataPosition = r.device()->pos();
        for (int block = 0; block < c->data()->blocksCount(); ++block) {
            if (!c->isComplex) {
                const QVector<double> yValues = sourceChannel->data()->rawYValues(block);
                if (yValues.isEmpty()) {
                    qDebug()<<"Отсутствуют данные для записи в канале"<<c->name();
                    continue;
                }

                for (double v: yValues) {
                    if (c->sampleWidth == 4)
                        r << (float)v;
                    else
                        r << v;
                }
            } // !c->isComplex
            else {
                const auto yValues = sourceChannel->data()->yValuesComplex(block);
                if (yValues.isEmpty()) {
                    qDebug()<<"Отсутствуют данные для записи в канале"<<c->name();
                    continue;
                }
                for (cx_double v: yValues) {
                    if (c->sampleWidth == 4) {
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
        c->size = r.device()->pos() - c->position;

        if (!populated) {
            sourceChannel->clear();
        }
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

void Data94File::fillPreliminary(FileDescriptor *file)
{
    updateDateTimeGUID();
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

    //дальше - каналы
    quint32 channelsCount;
    r >> channelsCount;
    for (quint32 i = 0; i < channelsCount; ++i) {
        Data94Channel *c = new Data94Channel(this);
        c->read(r);
    }
}

void Data94File::write()
{
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

    QJsonDocument doc(description);
    QByteArray json = doc.toJson();
    descriptionSize = json.size();
    r << descriptionSize;
    r.writeRawData(json.data(), descriptionSize);

    quint32 ccount = channels.count();
    r << ccount;

    for (int i=0; i<channels.size(); ++i) {
        r.setFloatingPointPrecision(QDataStream::SinglePrecision);
        Data94Channel *c = channels.at(i);
        qint64 pos = r.device()->pos();

        r.device()->write("d94chan ");

        //description
        if (c->changed()) {
            //переписываем описатель
            QJsonDocument doc(c->_description);
            QByteArray json = doc.toJson();
            c->descriptionSize = json.size();
            r << c->descriptionSize;
            r.writeRawData(json.data(), c->descriptionSize);
        }
        else {
            //просто копируем описатель из исходного файла
            in.device()->seek(c->position+8);
            QByteArray buf = in.device()->read(4+c->descriptionSize);
            r.writeRawData(buf.data(), buf.size());
        }
        c->position = pos;

        c->xAxisBlock.write(r);
        c->zAxisBlock.write(r);

        const quint32 format = c->isComplex ? 2 : 1;
        r << format;
        r << c->sampleWidth;

        c->setChanged(false);

        //data
        if (c->dataChanged()) {
            Q_ASSERT_X(c->populated(), "Data94 write", "Assuming that dataChanged && populated fails");

            if (c->sampleWidth == 8)
                r.setFloatingPointPrecision(QDataStream::DoublePrecision);

            c->dataPosition = r.device()->pos();
            if (!c->isComplex) {
                const QVector<double> yValues = c->data()->rawYValues(-1);
                if (yValues.isEmpty()) {
                    qDebug()<<"Отсутствуют данные для записи в канале"<<c->name();
                    continue;
                }

                for (double v: yValues) {
                    if (c->sampleWidth == 4)
                        r << (float)v;
                    else
                        r << v;
                }
            } // !c->isComplex
            else {
                const auto yValues = c->data()->yValuesComplex(-1);
                Q_ASSERT_X(uint(yValues.size()) == c->xAxisBlock.count*c->zAxisBlock.count,
                           "Data94File.write","The data size is wrong!");
                if (yValues.isEmpty()) {
                    qDebug()<<"Отсутствуют данные для записи в канале"<<c->name();
                    continue;
                }
                for (cx_double v: yValues) {
                    if (c->sampleWidth == 4) {
                        r << (float)v.real();
                        r << (float)v.imag();
                    }
                    else {
                        r << v.real();
                        r << v.imag();
                    }
                }
            } // c->isComplex

        } // c->dataChanged()
        else {
            //просто копируем описатель из исходного файла
            in.device()->seek(c->dataPosition);
            QByteArray buf = in.device()->read(c->zAxisBlock.count * c->xAxisBlock.count *
                                               format * c->sampleWidth);
            r.writeRawData(buf.data(), buf.size());
        } // !c->dataChanged()


        c->setDataChanged(false);
        c->size = r.device()->pos() - c->position;
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

void Data94File::copyChannelsFrom(FileDescriptor *sourceFile, const QVector<int> &indexes)
{
    const int count = channelsCount();

    for (int i: indexes) {
        new Data94Channel(sourceFile->channel(i), this);
    }

    //теперь записываем новые каналы - это позволит не копить данные в оперативной
    //памяти, и работать с файлами с любым количеством каналов
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
    quint32 ccount = channels.size();
    r <<ccount;

    qint64 pos = 8+4+descriptionSize+4;
    for (int i=0; i<count; ++i) {
        pos += channels.at(i)->size;
    }

    int destIndex = count;
    r.device()->seek(pos);

    for (int i=0; i<sourceFile->channelsCount(); ++i) {
        if (!indexes.contains(i)) continue;
        r.setFloatingPointPrecision(QDataStream::SinglePrecision);

        Data94Channel *destChannel = channels.at(destIndex);
        Channel *sourceChannel = sourceFile->channel(i);

        bool populated = sourceChannel->populated();
        if (!populated) sourceChannel->populate();

        destChannel->position = r.device()->pos();
        r.device()->write("d94chan ");

        //переписываем описатель
        QJsonDocument doc(destChannel->_description);
        QByteArray json = doc.toJson();
        destChannel->descriptionSize = json.size();
        r << destChannel->descriptionSize;
        r.writeRawData(json.data(), destChannel->descriptionSize);

        destChannel->xAxisBlock.write(r);
        destChannel->zAxisBlock.write(r);

        quint32 format = destChannel->isComplex ? 2 : 1;
        r << format;

        r << destChannel->sampleWidth;

        if (destChannel->sampleWidth == 8)
            r.setFloatingPointPrecision(QDataStream::DoublePrecision);

        destChannel->dataPosition = r.device()->pos();

        for (quint32 block = 0; block < destChannel->zAxisBlock.count; ++block) {
            if (!destChannel->isComplex) {
                QVector<double> yValues = sourceChannel->data()->rawYValues(block);
                if (yValues.isEmpty()) {
                    qDebug()<<"Отсутствуют данные для записи в канале"<<destChannel->name();
                    yValues.resize(destChannel->xAxisBlock.count);
                }

                for (double v: yValues) {
                    if (destChannel->sampleWidth == 4)
                        r << (float)v;
                    else
                        r << v;
                }
            }
            else {
                auto yValues = sourceChannel->data()->yValuesComplex(block);
                if (yValues.isEmpty()) {
                    qDebug()<<"Отсутствуют данные для записи в канале"<<destChannel->name();
                    yValues.resize(destChannel->xAxisBlock.count);
                }
                for (cx_double v: yValues) {
                    if (destChannel->sampleWidth == 4) {
                        r << (float)v.real();
                        r << (float)v.imag();
                    }
                    else {
                        r << v.real();
                        r << v.imag();
                    }
                }
            }
        }
        destChannel->size = r.device()->pos() - destChannel->position;

        if (!populated) {
            sourceChannel->clear();
            destChannel->clear();
        }
        destIndex++;
    }
}

void Data94File::addChannelWithData(DataHolder *data, const QList<Channel *> &source)
{
    Data94Channel *ch = new Data94Channel(this);
    ch->setPopulated(true);
    ch->setChanged(true);
    ch->setDataChanged(true);
    ch->setData(data);

    ch->setName("Среднее");
    QStringList l;
    for (int i=0; i<source.size(); ++i) {
        l << QString::number(source.at(i)->index()+1);
    }
    ch->setDescription("Среднее каналов "+l.join(","));

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
    ch->_description.insert("yname", source.first()->yName());
    ch->_description.insert("xname", source.first()->xName());
    ch->_description.insert("zname", source.first()->zName());
    ch->_description.insert("samples", qint64(ch->xAxisBlock.count));
    ch->_description.insert("blocks", qint64(ch->zAxisBlock.count));

    if (ch->xAxisBlock.uniform == 1 && !qFuzzyIsNull(ch->xAxisBlock.step))
        ch->_description.insert("samplerate", int(1.0 / ch->xAxisBlock.step));

    QJsonObject function;
    function.insert("name", Descriptor::functionTypeDescription(source.first()->type()));
    function.insert("type", source.first()->type());
    QString formatS;
    switch (data->yValuesFormat()) {
        case DataHolder::YValuesComplex: formatS = "complex"; break;
        case DataHolder::YValuesReals: formatS = "real"; break;
        case DataHolder::YValuesAmplitudes: formatS = "amplitude"; break;
        case DataHolder::YValuesAmplitudesInDB: formatS = "amplitudeDb"; break;
        case DataHolder::YValuesImags: formatS = "imaginary"; break;
        case DataHolder::YValuesPhases: formatS = "phase"; break;
        default: formatS = "real";
    }
    function.insert("format", formatS);
    function.insert("logref", data->threshold());
    QString unitsS;
    switch (data->yValuesUnits()) {
        case DataHolder::UnitsUnknown: unitsS = "unknown"; break;
        case DataHolder::UnitsLinear: unitsS = "linear"; break;
        case DataHolder::UnitsQuadratic: unitsS = "quadratic"; break;
        case DataHolder::UnitsDimensionless: unitsS = "dimensionless"; break;
        default: break;
    }
    function.insert("units", unitsS);
    int octave = source.first()->octaveType();
    if (octave > 0)
        function.insert("octaveFormat", octave);
    ch->_description.insert("function", function);
}

QString Data94File::calculateThirdOctave()
{
    QString thirdOctaveFileName = createUniqueFileName("", fileName(), "3oct", "d94", false);

    Data94File *thirdOctFile = new Data94File(thirdOctaveFileName);

    thirdOctFile->description = this->description;
    thirdOctFile->descriptionSize = this->descriptionSize;
    thirdOctFile->description.insert("sourceFile", fileName());
    thirdOctFile->updateDateTimeGUID();

    for (Data94Channel *ch: qAsConst(channels)) {
        const bool populated = ch->populated();
        if (!populated) ch->populate();

        Data94Channel *newCh = new Data94Channel(ch, thirdOctFile);

        //третьоктава рассчитывается только для первого блока
        newCh->zAxisBlock.count = 1;

        auto result = thirdOctave(ch->data()->decibels(0), ch->data()->xMin(), ch->data()->xStep());

        newCh->data()->setThreshold(ch->data()->threshold());
        newCh->data()->setYValuesUnits(ch->data()->yValuesUnits());
        newCh->data()->setXValues(result.first);
        newCh->data()->setYValues(result.second, DataHolder::YValuesAmplitudesInDB);
        newCh->_description.insert("samples", newCh->samplesCount());

        newCh->xAxisBlock.count = result.first.size();
        newCh->xAxisBlock.begin = result.first.constFirst();
        newCh->xAxisBlock.uniform = 0;
        newCh->xAxisBlock.values = result.first;

        QJsonObject function;
        function.insert("name", "OCTF3");
        function.insert("format", "amplitudeDb");
        function.insert("logref", ch->data()->threshold());
        QString units;
        switch (ch->data()->yValuesUnits()) {
            case DataHolder::UnitsUnknown: units = "unknown"; break;
            case DataHolder::UnitsLinear: units = "linear"; break;
            case DataHolder::UnitsQuadratic: units = "quadratic"; break;
            case DataHolder::UnitsDimensionless: units = "dimensionless"; break;
            default: break;
        }
        function.insert("units", units);
        function.insert("octaveFormat", 3);
        newCh->_description.insert("function", function);

        newCh->setYName(ch->yName());
        newCh->setPopulated(true);
        newCh->setChanged(true);
        newCh->setDataChanged(true);

        if (!populated) ch->clear();
    }

    thirdOctFile->setChanged(true);
    thirdOctFile->setDataChanged(true);
    thirdOctFile->write();

    delete thirdOctFile;
    return thirdOctaveFileName;
}

void Data94File::calculateMovingAvg(const QList<Channel*> &list, int windowSize)
{
    if (list.isEmpty()) return;

    populate();
    const Channel *firstChannel = list.first();
    //определяем количество отсчетов
    int numInd = firstChannel->samplesCount();

    for (Channel *ch: list) {
        const bool populated = ch->populated();
        if (!populated) ch->populate();

        Data94Channel *newCh = new Data94Channel(ch, this);
        newCh->setPopulated(true);
        newCh->setChanged(true);
        newCh->setDataChanged(true);

        //xAxisBlock
        newCh->xAxisBlock.uniform = firstChannel->data()->xValuesFormat() == DataHolder::XValuesUniform ? 1:0;
        newCh->xAxisBlock.begin = firstChannel->data()->xMin();
        if (newCh->xAxisBlock.uniform == 0) {// not uniform
            newCh->xAxisBlock.values = firstChannel->xValues();
            newCh->xAxisBlock.values.resize(numInd);
        }

        newCh->xAxisBlock.count = numInd;
        newCh->xAxisBlock.step = firstChannel->data()->xStep();

        //zAxisBlock
        newCh->zAxisBlock.count = 1;

        if (ch->data()->xValuesFormat()==DataHolder::XValuesUniform) {
            newCh->data()->setXValues(ch->data()->xMin(), ch->data()->xStep(), numInd);
        }
        else {
            QVector<double> values = ch->data()->xValues();
            values.resize(numInd);
            newCh->data()->setXValues(values);
        }

        auto format = ch->data()->yValuesFormat();

        if (format == DataHolder::YValuesComplex) {
            auto values = movingAverage(ch->data()->yValuesComplex(0), windowSize);
            values.resize(numInd);
            newCh->data()->setYValues(values);
        }
        else {
            QVector<double> values = movingAverage(ch->data()->linears(0), windowSize);
            values.resize(numInd);
            if (format == DataHolder::YValuesAmplitudesInDB)
                format = DataHolder::YValuesAmplitudes;
            newCh->data()->setYValues(values, format);
        }



        newCh->setName(ch->name()+" сглаж.");
        newCh->setDescription("Скользящее среднее канала "+ch->name());

        newCh->_description.insert("samples", qint64(newCh->xAxisBlock.count));
        if (newCh->xAxisBlock.uniform == 1 && !qFuzzyIsNull(newCh->xAxisBlock.step))
            newCh->_description.insert("samplerate", int(1.0 / newCh->xAxisBlock.step));

        if (!populated) ch->clear();
    }

    setChanged(true);
    setDataChanged(true);
    write();
}

QString Data94File::saveTimeSegment(double from, double to)
{
    // 0 проверяем, чтобы этот файл имел тип временных данных
    if (type() != Descriptor::TimeResponse) return QString();

    const quint32 count = channelsCount();

    // 1 создаем уникальное имя файла по параметрам from и to
    QString fromString, toString;
    getUniqueFromToValues(fromString, toString, from, to);
    QString suffix = QString("_%1s_%2s").arg(fromString).arg(toString);

    QString newFileName = createUniqueFileName("", fileName(), suffix, "d94", false);

    // 2 создаем новый файл
    Data94File *newFile = new Data94File(newFileName);

    newFile->description = this->description;
    newFile->descriptionSize = this->descriptionSize;
    newFile->description.insert("sourceFile", fileName());
    newFile->updateDateTimeGUID();

    QFile f(newFileName);
    if (!f.open(QFile::WriteOnly)) {
        qDebug()<<"Не удалось открыть файл для записи:"<<newFileName;
        return "";
    }

    QDataStream r(&f);
    r.setByteOrder(QDataStream::LittleEndian);
    r.setFloatingPointPrecision(QDataStream::SinglePrecision);

    //переписываем описательную часть
    r.device()->write("data94  ");
    QJsonDocument doc(description);
    QByteArray json = doc.toJson();
    descriptionSize = json.size();
    r << descriptionSize;
    r.writeRawData(json.data(), descriptionSize);
    r << count;

    // 4 сохраняем файл
    for (quint32 i=0; i<count; ++i) {
        bool wasPopulated = channels.at(i)->populated();
        if (!wasPopulated) channels.at(i)->populate();

        Data94Channel *ch = new Data94Channel(channels.at(i), newFile);

        // ищем границы данных по параметрам from и to
        int sampleStart = qRound((from/*-XBegin*/)/ch->xAxisBlock.step);
        if (sampleStart<0) sampleStart = 0;
        int sampleEnd = qRound((to/*-XBegin*/)/ch->xAxisBlock.step);
        if (sampleEnd>=samplesCount()) sampleEnd = samplesCount()-1;
        ch->xAxisBlock.count = sampleEnd - sampleStart + 1; //число отсчетов в новом файле

        ch->data()->setSegment(*(channels.at(i)->data()), sampleStart, sampleEnd);

        r.setFloatingPointPrecision(QDataStream::SinglePrecision);
        r.device()->write("d94chan ");

        //description
        QJsonDocument doc(ch->_description);
        QByteArray json = doc.toJson();
        ch->descriptionSize = json.size();
        r << ch->descriptionSize;
        r.writeRawData(json.data(), ch->descriptionSize);

        ch->xAxisBlock.write(r);
        ch->zAxisBlock.write(r);

        const quint32 format = ch->isComplex ? 2 : 1;
        r << format;

        r << ch->sampleWidth;
        if (ch->sampleWidth == 8)
            r.setFloatingPointPrecision(QDataStream::DoublePrecision);

        for (int block = 0; block < ch->data()->blocksCount(); ++block) {
            if (!ch->isComplex) {
                const QVector<double> yValues = ch->data()->rawYValues(block);
                if (yValues.isEmpty()) {
                    qDebug()<<"Отсутствуют данные для записи в канале"<<ch->name();
                    continue;
                }

                for (double v: yValues) {
                    if (ch->sampleWidth == 4)
                        r << (float)v;
                    else
                        r << v;
                }
            } // !c->isComplex
            else {
                const auto yValues = ch->data()->yValuesComplex(block);
                if (yValues.isEmpty()) {
                    qDebug()<<"Отсутствуют данные для записи в канале"<<ch->name();
                    continue;
                }
                for (cx_double v: yValues) {
                    if (ch->sampleWidth == 4) {
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

        if (!wasPopulated) channels.at(i)->data()->clear();
    }
    f.close();

    delete newFile;

    // 5 возвращаем имя нового файла
    return newFileName;
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
    if (channels.isEmpty()) return 0.0;
    return channels.constFirst()->data()->xMin();
}

double Data94File::xStep() const
{
    if (channels.isEmpty()) return 0.0;
    return channels.constFirst()->data()->xStep();
}

void Data94File::setXStep(const double xStep)
{
    if (channels.isEmpty()) return;
    bool changed = false;

    for (Data94Channel *ch: qAsConst(channels)) {
        if (ch->data()->xValuesFormat() == DataHolder::XValuesNonUniform) continue;
        if (ch->data()->xStep()!=xStep) {
            changed = true;
            ch->setXStep(xStep);
        }
    }
    if (changed) setChanged(true);
//    write();
}

int Data94File::samplesCount() const
{
    if (channels.isEmpty()) return 0;
    return channels.constFirst()->samplesCount();
}

void Data94File::setSamplesCount(int count)
{
    for (Data94Channel *c: qAsConst(channels)) {
        c->xAxisBlock.count = count;
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
    parent->channels << this;
}

Data94Channel::Data94Channel(Data94Channel *other, Data94File *parent)
    : Channel(other), parent(parent)
{
    _description = other->_description;
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

    //xAxisBlock
    xAxisBlock.uniform = other->data()->xValuesFormat() == DataHolder::XValuesUniform ? 1:0;
    xAxisBlock.begin = other->data()->xMin();
    if (xAxisBlock.uniform == 0) // not uniform
        xAxisBlock.values = other->xValues();
    xAxisBlock.count = other->samplesCount();
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
    sampleWidth = 4;

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
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(descriptionBuffer, &error);
    if (error.error != QJsonParseError::NoError) {
        qDebug()<<error.errorString() << error.offset;
        return;
    }
    _description = doc.object();

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

    double thr = _description.value("function").toObject().value("logref").toDouble();
    if (qFuzzyIsNull(thr)) thr = threshold(yName());
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

void Data94Channel::setXStep(double xStep)
{
    xAxisBlock.step = float(xStep);
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
        case 5: return data()->blocksCount();
        case 6: return _description.value("correction");
        default: ;
    }
    return QVariant();
}

int Data94Channel::columnsCount() const
{
    int minimumCount = 7;
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
        case 5: return QString("Кол-во блоков");
        case 6: return QString("Коррекция");
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

void Data94Channel::setXName(const QString &xName)
{
    _description.insert("xname", xName);
}

void Data94Channel::setZName(const QString &zName)
{
    _description.insert("zname", zName);
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
    if (parent) return parent->channels.indexOf(const_cast<Data94Channel*>(this));
    return -1;
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
        for (double x: values) r << float(x);
    }
}

quint32 AxisBlock::size() const
{
    if (uniform) return 16;

    return 8 + values.size() * 4;
}
