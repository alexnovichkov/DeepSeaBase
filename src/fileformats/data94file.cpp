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
    foreach (Data94Channel *f, d.channels) {
        this->channels << new Data94Channel(*f);
    }
//    foreach (Data94Channel *f, channels) {
//        f->parent = this;
    //    }
}

Data94File::Data94File(const FileDescriptor &other) : FileDescriptor(other.fileName())
{
    //TODO: Data94File доделать создание файла
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
    quint32 descriptionSize;
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
    quint32 paddingSize;
    r >> paddingSize;
    r.device()->skip(paddingSize);

    //reading xAxisBlock
    quint32 xAxisBlockPresent;
    r >> xAxisBlockPresent;
    if (xAxisBlockPresent > 0) {
        //данные по оси Х одинаковые для всех каналов
        xAxisBlock.read(r);
    }

    //дальше - каналы
    quint32 channelsCount;
    r >> channelsCount;
    for (quint32 i = 0; i < channelsCount; ++i) {
        Data94Channel *c = new Data94Channel(this);
        c->read(r);
        channels << c;
    }
}

void Data94File::write()
{
}

void Data94File::writeRawFile()
{
}

void Data94File::updateDateTimeGUID()
{
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
    if (channels.isEmpty()) return 7;
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
    if (channels.isEmpty()) return;
    bool changed = false;

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
    if (channels.isEmpty()) return 0;
    return channels.first()->samplesCount();
}

void Data94File::setSamplesCount(int count)
{
    Q_UNUSED(count);
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

void Data94Channel::read(QDataStream &r)
{

}

void Data94Channel::setXStep(double xStep)
{

}

QVariant Data94Channel::info(int column, bool edit) const
{
}

int Data94Channel::columnsCount() const
{
}

QVariant Data94Channel::channelHeader(int column) const
{
}

Descriptor::DataType Data94Channel::type() const
{
}

bool Data94Channel::populated() const
{
}

void Data94Channel::setPopulated(bool populated)
{
}

void Data94Channel::populate()
{
}

QString Data94Channel::name() const
{
}

void Data94Channel::setName(const QString &name)
{
}

QString Data94Channel::description() const
{
}

void Data94Channel::setDescription(const QString &description)
{
}

QString Data94Channel::xName() const
{
}

QString Data94Channel::yName() const
{
}

void Data94Channel::setYName(const QString &yName)
{
}

QString Data94Channel::legendName() const
{
}

FileDescriptor *Data94Channel::descriptor()
{
}

int Data94Channel::index() const
{
}

QString Data94Channel::correction() const
{
}

void Data94Channel::setCorrection(const QString &s)
{
}

void Data94Block::read(QDataStream &r)
{
    quint64 sizeInBytes;
    r >> sizeInBytes;

    quint32 dataFormat;
    r >> dataFormat;

    r >> sampleCount; //общее количество отсчетов для всего блока

    quint32 valueFormat;
    r >> valueFormat;
    complex = valueFormat>0;

    r >> blockCount;

}

void Data94Block::write(QDataStream &r)
{

}

void XAxisBlock::read(QDataStream &r)
{DD;
    if (r.status() != QDataStream::Ok) return;

    r.setFloatingPointPrecision(QDataStream::DoublePrecision);
    quint32 labelSize;
    r >> labelSize;
    QByteArray labelData = r.device()->read(labelSize);
    label = QString::fromUtf8(labelData);

    r >> uniform;//     0 - шкала неравномерная, 1 - шкала равномерная
    r >> samplesCount;

    if (uniform > 0) {
        r >> xBegin;
        r >> xStep;
    }
    else {
        values = getChunkOfData<double>(r, samplesCount, 0xC0000008, 0);
    }
    isValid = true;
}

void XAxisBlock::write(QDataStream &r)
{DD;
    if (r.status() != QDataStream::Ok) return;

    r.setFloatingPointPrecision(QDataStream::DoublePrecision);
    QByteArray labelData = label.toUtf8();
    r << labelData.size();
    r.device()->write(labelData);

    r << uniform;//     0 - шкала неравномерная, 1 - шкала равномерная
    r << samplesCount;

    if (uniform > 0) {
        r << xBegin;
        r << xStep;
    }
    else {
        foreach (double x, values) r << x;
    }
}
