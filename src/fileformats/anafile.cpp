#include "anafile.h"

#include "logging.h"
#include "algorithms.h"
#include "unitsconverter.h"

AnaFile::AnaFile(const QString &fileName) : FileDescriptor(fileName)
{DD;
    rawFileName = fileName.left(fileName.length()-3)+"ana";
}

AnaFile::~AnaFile()
{
    if (changed() || dataChanged())
        write();
    delete _channel;
}

QStringList AnaFile::fileFilters()
{
    return QStringList()<< "Файлы ana/anp (*.anp)";
}

QStringList AnaFile::suffixes()
{
    return QStringList()<<"*.anp";
}

void AnaFile::read()
{
    QFile anp(fileName());
    if (!anp.open(QFile::ReadOnly | QFile::Text)) {
        LOG(ERROR) << QString("Не могу открыть файл") << fileName();
        return;
    }

    DataDescription channel;

    int format = getFormat();
    double samplerate = 1;
    int samples = 0;

    QTextStream s(&anp);
    QString date;
    QString time;
    while (!s.atEnd()) {
        QString line = s.readLine();
        if (line.startsWith(";")) {//channel name
            channel.put("name", line.mid(1));
        }
        else if (line.startsWith("GAIN ")) channel.put("description.gain", line.mid(5).toInt());
        else if (line.startsWith("ABSVOLT ")) channel.put("description.absvolt", line.mid(8).toDouble());
       // else if (line == "FORMAT i") channel.put("function.precision", format == 2 ? "int16" : "int32");
        else if (line == "FORMAT d") format = 4;
        else if (line.startsWith("DATE ")) date = line.mid(5);
        else if (line.startsWith("TIME_TSS ")) time = line.mid(9);
        else if (line.startsWith("BASE ")) channel.put("description.base", line.mid(5).toInt());
        else if (line.startsWith("DBOFF ")) channel.put("description.dboff", line.mid(6).toInt());
        else if (line.startsWith("FRC_CORR ")) channel.put("description.frccorr", line.mid(9));
        else if (line.startsWith("VEL ")) channel.put("description.velocity", line.mid(4).toInt());
        else if (line.startsWith("DTR ")) channel.put("description.distance", line.mid(4).toDouble());
        else if (line.startsWith("TTR ")) channel.put("description.time", line.mid(4).toDouble());
        else if (line.startsWith("VOLTAGE ")) channel.put("description.voltage", line.mid(8).toInt());
        else if (line.startsWith("CHANNEL ")) channel.put("description", line.toLower());
        else if (line.startsWith("FRQ ")) samplerate = line.mid(4).toDouble();
    }
    dataDescription().put("dateTime", dateTimeFromString(date,time));
    channel.put("function.name", "time");
    channel.put("function.type", 1);
    channel.put("function.format", "real");
    channel.put("function.logscale", "linear");
    channel.put("function.octaveFormat", 0);
    channel.put("xname", "с");
    channel.put("yname", "В");
    channel.put("zname", "");
    channel.put("blocks", 1);
    samples = QFile(rawFileName).size() / format;
    channel.put("samples", samples);
    channel.put("samplerate", samplerate);
    channel.put("function.precision", "float"); //всегда отдаем этот формат, потому что
    //целочисленные значения, записанные в ana, преобразуются в double

    _channel  = new AnaChannel(this);
    _channel->setDataDescription(channel);
    _channel->format = format;
    _channel->data()->setXValues(0, 1.0 / samplerate, samples);
    _channel->data()->setZValues(0,0,1);
    _channel->data()->setYValuesFormat(DataHolder::YValuesReals);
    _channel->data()->setYValuesUnits(DataHolder::UnitsLinear);
    auto thr = PhysicalUnits::Units::logref("В");
    _channel->data()->setThreshold(thr);
    dataDescription().put("function.logref", thr);
}

int AnaFile::channelsCount() const
{
    return 1;
}

Channel *AnaFile::channel(int index) const
{
    if (index != 0) return nullptr;
    return _channel;
}

QString AnaFile::fileType() const
{
    return "anp";
}

QString AnaFile::icon() const
{
    return ":/icons/anp.svg";
}

bool AnaFile::rename(const QString &newName, const QString &newPath)
{
    bool result = FileDescriptor::rename(newName, newPath);
    if (!result) return false;

    QString newRawName = changeFileExt(fileName(), "ana");

    result &= QFile::rename(rawFileName, newRawName);
    if (result) rawFileName = newRawName;
    return result;
}

bool AnaFile::rename(const QString &newName)
{
    bool result = FileDescriptor::rename(newName);
    if (!result) return false;

    QString newRawName = changeFileExt(fileName(), "ana");

    result &= QFile::rename(rawFileName, newRawName);
    if (result) rawFileName = newRawName;
    return result;
}

bool AnaFile::remove()
{
    bool result = FileDescriptor::remove();
    if (!result) return false;

    QString newRawName = changeFileExt(fileName(), "ana");

    result &= QFile::remove(newRawName);
    return result;
}

bool AnaFile::copyTo(const QString &name)
{
    QString rawFile = name;
    QString suffix = QFileInfo(name).suffix();
    rawFile.replace(rawFile.length() - suffix.length(), suffix.length(), "ana");

    return FileDescriptor::copyTo(name) && QFile::copy(rawFileName, rawFile);
}

Descriptor::DataType AnaFile::type() const
{
    return Descriptor::TimeResponse;
}

QString AnaFile::typeDisplay() const
{
    return QString("Данные");
}

bool AnaFile::fileExists() const
{
    return (FileDescriptor::fileExists() && QFileInfo(rawFileName).exists());
}

bool AnaFile::isSourceFile() const
{
    return true;
}

bool AnaFile::dataTypeEquals(FileDescriptor *other) const
{
    return other->type() == Descriptor::TimeResponse;
}

bool AnaFile::canTakeChannelsFrom(FileDescriptor *other) const
{
    if (!_channel) return other->type() == Descriptor::TimeResponse;
    return false;
}

bool AnaFile::canTakeAnyChannels() const
{
    return false;
}

qint64 AnaFile::fileSize() const
{
    return QFile(rawFileName).size();
}

int AnaFile::getFormat() const
{
    //Если в папке с файлом нет файла zrec.ini, то возвращаем 2 (16-бит формат)
    //Если файл zrec.ini есть, то читаем из него параметр Format. Если он равен 0,
    //то возвращаем 2, иначе - 4

    QFileInfo fi(fileName());
    if (QFile f(fi.canonicalPath()+"/"+"zrec.ini"); f.exists()) {
        QSettings s(f.fileName(), QSettings::IniFormat);
        return s.value("Common/Format", 0).toInt() == 0 ? 2 : 4;
    }
    return 2;
}


AnaChannel::AnaChannel(AnaFile *parent) : Channel(), parent(parent)
{
    parent->_channel = this;
    //return ((v*ADCStep+ADC0)/AmplLevel - AmplShift - Sens0Shift)/SensSensitivity;
}

QVariant AnaChannel::info(int column, bool edit) const
{
    Q_UNUSED(edit)
    switch (column) {
        case 0: return dataDescription().get("name");
        case 1: return dataDescription().get("yname");
        case 2: return dataDescription().get("description");
        case 3: return dataDescription().get("description.base");
        case 4: return dataDescription().get("description.velocity");
        case 5: return dataDescription().get("description.distance");
        case 6: return dataDescription().get("description.time");
        default: ;
    }
    return QVariant();
}

int AnaChannel::columnsCount() const
{
    return 7;
}

QVariant AnaChannel::channelHeader(int column) const
{
    switch (column) {
        case 0: return QString("Имя");
        case 1: return QString("Ед.изм.");
        case 2: return QString("Описание");
        case 3: return QString("База");
        case 4: return QString("Скорость");
        case 5: return QString("Расст. до СИД");
        case 6: return QString("Время");
        default: return QVariant();
    }
    return QVariant();
}

Descriptor::DataType AnaChannel::type() const
{
    return Descriptor::TimeResponse;
}

DataPrecision fromAnaPrecision(int f)
{
    if (f == 2) return DataPrecision::Int16;
    if (f == 4) return DataPrecision::Int32;
    return DataPrecision::Int16;
}

void AnaChannel::populate()
{
    _data->clear();

    QFile rawFile(parent->rawFileName);

    if (rawFile.open(QFile::ReadOnly)) {
        const auto prec = fromAnaPrecision(format);
        QVector<double> YValues;
        const quint64 blockSizeBytes = rawFile.size();

        // map file into memory
        unsigned char *ptr = rawFile.map(0, rawFile.size());
        if (ptr) {//достаточно памяти отобразить весь файл
            YValues = convertFrom<double>(ptr, blockSizeBytes, prec);
        }
        else {
            //читаем классическим способом через getChunk
            QDataStream readStream(&rawFile);
            readStream.setByteOrder(QDataStream::LittleEndian);
            if (prec == DataPrecision::Float)
                readStream.setFloatingPointPrecision(QDataStream::SinglePrecision);

            qint64 actuallyRead = 0;
            const quint64 chunkSize = dataDescription().get("samples").toLongLong();
            YValues = getChunkOfData<double>(readStream, chunkSize, prec, &actuallyRead);
        }

        YValues.resize(data()->samplesCount());
        postprocess(YValues);
        _data->setYValues(YValues, _data->yValuesFormat(), -1);
        setPopulated(true);
        rawFile.close();
    }
    else {
        LOG(ERROR)<<"Cannot read raw file"<<parent->rawFileName;
    }
}

FileDescriptor *AnaChannel::descriptor() const
{
    return parent;
}

int AnaChannel::index() const
{
    return 0;
}

void AnaChannel::postprocess(QVector<double> &v)
{
    const double ABSVolt = dataDescription().get("description.absvolt").toDouble();

    for (int i=0; i<v.size(); ++i) v[i] = v[i]*ABSVolt;
}
