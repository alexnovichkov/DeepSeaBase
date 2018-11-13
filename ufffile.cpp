#include "ufffile.h"

//#include <QtGui>
#include <QMessageBox>
#include "logging.h"
#include "mainwindow.h"
#include "algorithms.h"
#include "dataholder.h"
#include "averaging.h"

QList<AbstractField*> fields = {
    new DelimiterField,
    new Float13_5Field,
    new Float15_7Field,
    new Float20_12Field,
    new Float25_17Field,
    new Integer4Field,
    new Integer5Field,
    new Integer6Field,
    new Integer10Field,
    new Integer12Field,
    new String80Field,
    new String10Field,
    new String10aField,
    new String20Field,
    new TimeDateField,
    new TimeDate80Field,
    new EmptyField
};

int abscissaType(const QString &xName)
{
    QString s = xName.toLower();
    if (s == "hz" || s == "гц") return 18;
    if (s == "s" || s == "с") return 17;
    if (s == "m/s" || s == "м/с") return 11;
    if (s == "m/s2" || s == "m/s^2" || s == "м/с2" || s == "м/с^2") return 12;
    if (s == "n" || s == "н") return 13;
    if (s == "pa" || s == "psi" || s == "па") return 15;
    if (s == "m" || s == "м") return 8;
    if (s == "kg" || s == "кг") return 16;

    return 0; //0 - unknown
}



QString abscissaTypeDescription(int type)
{
    switch (type) {
        case 0: return "Unknown";
        case 1: return "General";
        case 2: return "Stress";
        case 3: return "Strain";
        case 5: return "Temperature";
        case 6: return "Heat flux";
        case 8: return "Displacement";
        case 9: return "Reaction force";
        case 11: return "Velocity";
        case 12: return "Acceleration";
        case 13: return "Excitation force";
        case 15: return "Pressure";
        case 16: return "Mass";
        case 17: return "Time";
        case 18: return "Frequency";
        case 19: return "RPM";
        case 20: return "Order";
    }
    return "Unknown";
}

UffFileDescriptor::UffFileDescriptor(const QString &fileName) : FileDescriptor(fileName)
  , NumInd(0)
{DD;



}

UffFileDescriptor::UffFileDescriptor(const UffFileDescriptor &other) : FileDescriptor(other.fileName())
{DD;
    this->NumInd = other.NumInd;
    this->header = other.header;
    this->units = other.units;

    foreach (Function *f, other.channels) {
        this->channels << new Function(*f);
    }
    foreach (Function *f, channels) {
        f->parent = this;
    }
}

UffFileDescriptor::UffFileDescriptor(const FileDescriptor &other) : FileDescriptor(other.fileName())
{
    //заполнение header
//    header.type151[10].value = QDateTime::fromString(other.dateTime(), "dd.MM.yy hh:mm:ss");
//    header.type151[12].value = QDateTime::fromString(other.dateTime(), "dd.MM.yy hh:mm:ss");
    header.type151[10].value = other.dateTime();
    header.type151[12].value = other.dateTime();
    header.type151[16].value = QDateTime::currentDateTime();

    int referenceChannelNumber = -1; //номер опорного канала ("сила")
    //заполнение каналов
    for (int i=0; i<other.channelsCount(); ++i) {
        Channel *ch = other.channel(i);
        if (!ch->populated()) ch->populate();
        Function *f = new Function(*ch);

        f->type58[15].value = i+1;
        f->type58[10].value = QString("Record %1").arg(i+1);

        if (ch->xName().toLower()=="сила")
            referenceChannelNumber = i;

        f->type58[8].value = header.type151[10].value;
        channels << f;
        //clearing
        ch->setPopulated(false);
        ch->data()->clear();
    }
    //заполнение инфы об опорном канале
    if (referenceChannelNumber>=0) {
        for (int nc=0; nc<channels.size(); ++nc) {
            channels[nc]->type58[18].value = other.channel(nc)->name();
            channels[nc]->type58[19].value = referenceChannelNumber+1;
        }
    }
    fillRest();
}

UffFileDescriptor::~UffFileDescriptor()
{DD; /*qDebug()<<fileName()<<changed();*/
    if (changed())
        write();

    qDeleteAll(channels);
}

void UffFileDescriptor::fillPreliminary(Descriptor::DataType)
{DD;
    updateDateTimeGUID();
}

void UffFileDescriptor::fillRest()
{DD;

}


void UffFileDescriptor::read()
{DD;
    if (QFile::exists(fileName()+"~")) {
        // в папке с записью есть двоичный файл с описанием записи
        QFile uff(fileName()+"~");
        if (uff.open(QFile::ReadOnly)) {
            QDataStream stream(&uff);

            stream >> header;
            stream >> units;

            while (!stream.atEnd()) {
                Function *f = new Function(this);
                f->read(stream);

                channels << f;
            }
        }
    }
    else {
        QFile uff(fileName());
        if (!uff.exists()) return;

        if (uff.open(QFile::ReadOnly | QFile::Text)) {
            QTextStream stream(&uff);

            header.read(stream);
            units.read(stream);

            while (!stream.atEnd()) {
                Function *f = new Function(this);
                f->read(stream, -1);

                channels << f;

            }
        }

        QFile buff(fileName()+"~");
        if (buff.open(QFile::WriteOnly)) {
            QDataStream stream(&buff);

            stream << header;
            stream << units;

            foreach(Function *f, channels) {
                stream << f->header;
                stream << f->type58;
                stream << f->dataPosition;
            }
        }
    }

}

void UffFileDescriptor::write()
{DD;
    if (!changed()) return;

    //be sure all channels were read. May take too much memory
    populate();

    QFile uff(fileName());
    if (uff.open(QFile::WriteOnly | QFile::Text)) {
        QTextStream stream(&uff);
        header.write(stream);
        units.write(stream);

        foreach (Function *c, channels) {
            c->write(stream);
        }
    }
    setChanged(false);
}

void UffFileDescriptor::writeRawFile()
{DD;
    // Nothing to do here
}

void UffFileDescriptor::populate()
{DD;
    for(int i = 0; i < channels.size(); ++i) {
        if (!channels[i]->populated()) channels[i]->populate();
    }
}

void UffFileDescriptor::updateDateTimeGUID()
{DD;
    QDateTime t = QDateTime::currentDateTime();
    header.type151[10].value = t;
    header.type151[12].value = t;
    header.type151[16].value = t;
    header.type151[14].value = "DeepSeaBase by Novichkov";
}

QString makeStringFromPair(const QPair<QString, QString> &pair)
{
    QString result = pair.second;
    if (!pair.first.isEmpty()) {
        result = pair.first+"="+result;
    }
    result.truncate(80);
    return result;
}

QString UffFileDescriptor::dataDescriptorAsString() const
{
    return header.info();
}

QMap<QString, QString> UffFileDescriptor::info() const
{
    return QMap<QString, QString>();
}

Descriptor::DataType UffFileDescriptor::type() const
{
    if (channels.isEmpty()) return Descriptor::Unknown;

    Descriptor::DataType t = channels.first()->type();
    for (int i=1; i<channels.size(); ++i) {
        if (channels.at(i)->type() != t) return Descriptor::Unknown;
    }
    return t;
}

QString UffFileDescriptor::typeDisplay() const
{
    return Function::functionTypeDescription(type());
}

double UffFileDescriptor::size() const
{
    double size = 0.0;
    if (!channels.isEmpty()) {
        if (channels.first()->data()->xValuesFormat() == DataHolder::XValuesUniform)
            size = samplesCount() * xStep();
        else {
            channels.first()->populate();
            size = channels.first()->data()->xValues().last();
        }
    }
    return size;
}

QDateTime UffFileDescriptor::dateTime() const
{DD;
    return header.type151[10].value.toDateTime()/*.toString("dd.MM.yy hh:mm:ss")*/;
}

double UffFileDescriptor::xStep() const
{
    if (!channels.isEmpty()) return channels.first()->xStep();
    return 0.0;
}

void UffFileDescriptor::setXStep(const double xStep)
{DD;
    if (channels.isEmpty()) return;
    bool changed = false;

    for (int i=0; i<channels.size(); ++i) {
        if (channels.at(i)->xStep()!=xStep) {
            changed = true;
            channels[i]->type58[29].value = xStep;
            channels[i]->data()->setXStep(xStep);
        }
    }
    if (changed) setChanged(true);
    write();
}

QString UffFileDescriptor::xName() const
{DD;
    if (channels.isEmpty()) return QString();

    QString xname = channels.first()->xName();

    for (int i=1; i<channels.size(); ++i) {
        if (channels[i]->xName() != xname) return QString();
    }
    return xname;
}

bool UffFileDescriptor::setLegend(const QString &legend)
{DD;
    if (legend == header.type151[8].value.toString()) return false;
    header.type151[8].value = legend;
    setChanged(true);
    return true;
}

QString UffFileDescriptor::legend() const
{DD;
    return header.type151[8].value.toString();
}

void UffFileDescriptor::setDateTime(QDateTime dt)
{DD;
    header.type151[10].value = dt;
    setChanged(true);
}

void UffFileDescriptor::deleteChannels(const QVector<int> &channelsToDelete)
{DD;
    for (int i=channels.size()-1; i>=0; --i) {
        if (channelsToDelete.contains(i)) {
            delete channels.takeAt(i);
        }
    }

    setChanged(true);
    removeTempFile();
}

void UffFileDescriptor::removeTempFile()
{
    if (QFile::exists(fileName()+"~")) QFile::remove(fileName()+"~");
}

void UffFileDescriptor::copyChannelsFrom(const QList<QPair<FileDescriptor *, int> > &channelsToCopy)
{DD;
    QList<FileDescriptor*> records;
    for (int i=0; i<channelsToCopy.size(); ++i)
        if (!records.contains(channelsToCopy.at(i).first)) records << channelsToCopy.at(i).first;

    foreach (FileDescriptor *record, records) {
        UffFileDescriptor *uff = static_cast<UffFileDescriptor *>(record);
        QList<int> channelsIndexes = filterIndexes(record, channelsToCopy);

        for(int i = 0; i < record->channelsCount(); ++i) {
            if (!record->channel(i)->populated() && channelsIndexes.contains(i))
                record->channel(i)->populate();
        }

        //добавляем в файл dfd копируемые каналы из dfdRecord
        foreach (int index, channelsIndexes) {
            if (uff) {
                channels.append(new Function(*uff->channels[index]));
            }
            else {
                channels.append(new Function(*record->channel(index)));
            }
        }
    }

    for (int i=0; i<channels.size(); ++i) {
        channels[i]->parent = this;
    }

    //меняем параметры файла uff
    updateDateTimeGUID();
    setChanged(true);
    write();
    removeTempFile();
}

void UffFileDescriptor::calculateMean(const QList<QPair<FileDescriptor *, int> > &channels)
{DD;

    if (channels.isEmpty()) return;

    populate();

    Function *ch = new Function(this);

    QList<Channel*> list;
    for (int i=0; i<channels.size(); ++i)
        list << channels.at(i).first->channel(channels.at(i).second);
    Channel *firstChannel = list.first();

    //ищем наименьшее число отсчетов
    int numInd = firstChannel->samplesCount();
    for (int i=1; i<list.size(); ++i) {
        if (list.at(i)->samplesCount() < numInd)
            numInd = list.at(i)->samplesCount();
    }

    // ищем формат данных для нового канала
    // если форматы разные, то формат будет линейный (амплитуды), не логарифмированный
    int format = firstChannel->yValuesFormat();
    for (int i=1; i<list.size(); ++i) {
        if (list.at(i)->yValuesFormat() != format) {
            format = DataHolder::YValuesAmplitudes;
            break;
        }
    }

    Averaging averaging(Averaging::Linear, list.size());

    foreach (Channel *ch, list) {
        if (ch->yValuesFormat() == DataHolder::YValuesComplex)
            averaging.average(ch->data()->yValuesComplex());
        else
            averaging.average(ch->data()->linears());
    }

    // обновляем сведения канала
    ch->setName("Среднее");
    ch->type58[8].value = QDateTime::currentDateTime();

    QStringList l;
    for (int i=0; i<channels.size(); ++i) {
        l << QString::number(channels.at(i).second + 1);
    }
    ch->setDescription("Среднее каналов "+l.join(","));

    ch->data()->setThreshold(firstChannel->data()->threshold());
    if (format == DataHolder::YValuesComplex)
        ch->data()->setYValues(averaging.getComplex().mid(0, numInd));
    else if (format == DataHolder::YValuesAmplitudesInDB) {
        QVector<double> data = averaging.get().mid(0, numInd);
        ch->data()->setYValues(DataHolder::toLog(data, firstChannel->data()->threshold()), DataHolder::YValuesFormat(format));
    }
    else
        ch->data()->setYValues(averaging.get().mid(0, numInd), DataHolder::YValuesFormat(format));

    if (firstChannel->data()->xValuesFormat()==DataHolder::XValuesUniform) {
        ch->type58[27].value = 1;
        ch->data()->setXValues(firstChannel->xMin(), firstChannel->xStep(), numInd);
    }
    else {
        ch->type58[27].value = 0;
        ch->data()->setXValues(firstChannel->data()->xValues().mid(0, numInd));
    }
    ch->setPopulated(true);

    ch->type58[14].value = firstChannel->type();

    ch->type58[25].value = (format == DataHolder::YValuesComplex ? 6 : 4);
    ch->type58[26].value = ch->data()->samplesCount();

    ch->type58[28].value = firstChannel->xMin();
    ch->type58[29].value = firstChannel->xStep();

    ch->type58[32].value = abscissaType(firstChannel->xName());
    ch->type58[36].value = abscissaTypeDescription(ch->type58[32].value.toInt());
    ch->type58[37].value = firstChannel->xName();

    ch->type58[44].value = firstChannel->yName();

    ch->parent = this;

    this->channels << ch;
    removeTempFile();
}

void UffFileDescriptor::calculateMovingAvg(const QList<QPair<FileDescriptor *, int> > &channels, int windowSize)
{DD;
    populate();

    for (int i=0; i<channels.size(); ++i) {
        Function *ch = new Function(this);
        FileDescriptor *firstDescriptor = channels.at(i).first;
        Channel *firstChannel = firstDescriptor->channel(channels.at(i).second);

        int numInd = firstChannel->samplesCount();

        ch->data()->setThreshold(firstChannel->data()->threshold());
        if (firstChannel->data()->yValuesFormat() == DataHolder::YValuesComplex) {
            ch->data()->setYValues(movingAverage(firstChannel->data()->yValuesComplex(), windowSize));
        }
        else {
            QVector<double> values = movingAverage(firstChannel->data()->linears(), windowSize);
            if (firstChannel->data()->yValuesFormat() == DataHolder::YValuesAmplitudesInDB)
                values = DataHolder::toLog(values, threshold(firstChannel->yName()));
            ch->data()->setYValues(values, DataHolder::YValuesFormat(firstChannel->data()->yValuesFormat()));
        }

        if (firstChannel->data()->xValuesFormat()==DataHolder::XValuesUniform) {
            ch->type58[27].value = 1;
            ch->data()->setXValues(firstChannel->xMin(), firstChannel->xStep(), numInd);
        }
        else {
            ch->type58[27].value = 0;
            ch->data()->setXValues(firstChannel->data()->xValues());
        }

        ch->type58[29].value = firstDescriptor->xStep();

        // обновляем сведения канала
        ch->setPopulated(true);
        ch->setName(firstChannel->name()+" сглаж.");
        ch->setDescription("Скользящее среднее канала "+firstChannel->name());

        ch->type58[44].value = firstChannel->yName();
        ch->type58[37].value = firstChannel->xName();
        ch->parent = this;

        ch->type58[8].value = QDateTime::currentDateTime();

        ch->type58[26].value = ch->data()->samplesCount();
        ch->type58[25].value = (ch->data()->yValuesFormat() == DataHolder::YValuesComplex ? 6 : 4);
        ch->type58[14].value = firstChannel->type();

        ch->type58[32].value = abscissaType(firstChannel->xName());
        ch->type58[36].value = abscissaTypeDescription(ch->type58[32].value.toInt());

        ch->type58[28].value = firstChannel->xMin();

        this->channels << ch;
    }
    removeTempFile();
}

QString UffFileDescriptor::calculateThirdOctave()
{DD;
    populate();

    QString thirdOctaveFileName = this->fileName();
    thirdOctaveFileName.chop(4);

    int index = 0;
    if (QFile::exists(thirdOctaveFileName+"_3oct.uff")) {
        index++;
        while (QFile::exists(thirdOctaveFileName+"_3oct("+QString::number(index)+").uff")) {
            index++;
        }
    }
    thirdOctaveFileName = index>0?thirdOctaveFileName+"_3oct("+QString::number(index)+").uff":thirdOctaveFileName+"_3oct.uff";

    UffFileDescriptor *thirdOctUff = new UffFileDescriptor(thirdOctaveFileName);
    thirdOctUff->fillPreliminary(Descriptor::Spectrum);


    foreach (Function *ch, this->channels) {
        Function *newCh = new Function(*ch);

        auto result = thirdOctave(ch->data()->decibels(), ch->xMin(), ch->xStep());

        newCh->data()->setXValues(result.first);
        newCh->data()->setThreshold(ch->data()->threshold());
        newCh->data()->setYValues(result.second, DataHolder::YValuesAmplitudesInDB);
        newCh->setPopulated(true);

        newCh->parent = thirdOctUff;

        newCh->type58[6].value = "Третьоктава";

        newCh->type58[25].value = 4; //25 Ordinate Data Type
    //                                       2 - real, single precision
    //                                       4 - real, double precision
    //                                       5 - complex, single precision
    //                                       6 - complex, double precision
        newCh->type58[26].value = newCh->data()->samplesCount(); //26   Number of data pairs for uneven abscissa spacing,
                                           //  or number of data values for even abscissa spacing
        newCh->type58[27].value = 0; //27 Abscissa Spacing (1=even, 0=uneven,
        newCh->type58[28].value = 0.0; //28 Abscissa minimum
        newCh->type58[29].value = 0.0; //29 Abscissa increment
        newCh->type58[44].value = "дБ"; //Ordinate name

        thirdOctUff->channels.append(newCh);
    }

    thirdOctUff->fillRest();

    thirdOctUff->setChanged(true);
    thirdOctUff->setDataChanged(true);
    thirdOctUff->write();
    thirdOctUff->writeRawFile();
    delete thirdOctUff;
    return thirdOctaveFileName;
}

void UffFileDescriptor::move(bool up, const QVector<int> &indexes, const QVector<int> &newIndexes)
{DD;
    int i=up?0:indexes.size()-1;
    while (1) {
        channels.move(indexes.at(i),newIndexes.at(i));
        if ((up && i==indexes.size()-1) || (!up && i==0)) break;
        i=up?i+1:i-1;
    }
    setChanged(true);
    removeTempFile();
}

int UffFileDescriptor::channelsCount() const
{DD;
    return channels.size();
}

bool UffFileDescriptor::hasAttachedFile() const
{DD;
    return false;
}

QString UffFileDescriptor::attachedFileName() const
{DD;
    return QString();
}

void UffFileDescriptor::setAttachedFileName(const QString &name)
{DD;
    Q_UNUSED(name);
}

QStringList UffFileDescriptor::getHeadersForChannel(int channel)
{DD;
    if (channels.size()>channel)
        return channels[channel]->getInfoHeaders();
    return QStringList();
}

Channel *UffFileDescriptor::channel(int index) const
{DD;
    if (channels.size()>index)
        return channels[index];
    return 0;
}

bool UffFileDescriptor::allUnplotted() const
{DD;
    foreach (Function *c, channels) {
        if (c->checkState() == Qt::Checked) return false;
    }
    return true;
}

bool UffFileDescriptor::isSourceFile() const
{DD;
    if (channels.isEmpty()) return false;
    int type = channels.first()->type();
    for (int i=1; i<channels.size(); ++i) {
        if (channels.at(i)->type() != type) {
            return false;
        }
    }

    return (type == Descriptor::TimeResponse);
}

bool UffFileDescriptor::operator ==(const FileDescriptor &descriptor)
{DD;
    return this->fileName() == descriptor.fileName();
}

bool UffFileDescriptor::dataTypeEquals(FileDescriptor *other) const
{DD;
    return (this->type() == other->type());
}

QString UffFileDescriptor::fileFilters() const
{DD;
    return QString("Файлы uff (*.uff);;Файлы unv (*.unv)");
}


UffHeader::UffHeader()
{DD;
    setType151(type151);
}

void UffHeader::read(QTextStream &stream)
{DD;
    for (int i=0; i<20; ++i) {
        fields[type151[i].type]->read(type151[i].value, stream);
//        qDebug() << i << type151[i].value;
    }

}

void UffHeader::write(QTextStream &stream)
{DD;
    type151[16].value = QDateTime::currentDateTime();

    for (int i=0; i<20; ++i) {
        fields[type151[i].type]->print(type151[i].value, stream);
    }
}

QString UffHeader::info() const
{DD;
    return type151[4].value.toString()+" "+type151[6].value.toString();
}


UffUnits::UffUnits()
{DD;
    setType164(type164);
}

void UffUnits::read(QTextStream &stream)
{DD;
    for (int i=0; i<14; ++i) {
        fields[type164[i].type]->read(type164[i].value, stream);
    }
}

void UffUnits::write(QTextStream &stream)
{DD;
    for (int i=0; i<14; ++i) {
        fields[type164[i].type]->print(type164[i].value, stream);
    }
}


FunctionHeader::FunctionHeader()
{DD;
    setType1858(type1858);
    valid = true;
}

void FunctionHeader::read(QTextStream &stream)
{DD;
    for (int i=0; i<4; ++i) {
        fields[type1858[i].type]->read(type1858[i].value, stream);
    }
    if (type1858[2].value.toInt()==1858) {
        for (int i=4; i<48; ++i) {
            fields[type1858[i].type]->read(type1858[i].value, stream);
        }
    }
    else {
        valid = false;
        type1858[2].value = 1858;
    }
}

void FunctionHeader::write(QTextStream &stream)
{DD;
    for (int i=0; i<48; ++i) {
        fields[type1858[i].type]->print(type1858[i].value, stream);
    }
}

QDataStream &operator>>(QDataStream &stream, FunctionHeader &header)
{
    stream >> header.type1858;
    stream >> header.valid;
    return stream;
}


Function::Function(UffFileDescriptor *parent) : Channel(),
    parent(parent), dataPosition(-1), _populated(false)
{DD;
    temporalCorrection = false;
    setType58(type58);
}



Function::Function(Channel &other) : Channel(other)
{DD;
    temporalCorrection = false;
    setType58(type58);

    type58[4].value = other.name(); if (other.name().isEmpty()) type58[4].value = "NONE";
    type58[6].value = other.description(); if (other.description().isEmpty()) type58[6].value = "NONE";
    type58[8].value = QDateTime::currentDateTime();
    type58[14].value = other.type();


    type58[25].value = (unsigned int)other.yFormat();
    type58[26].value = data()->samplesCount();

    type58[28].value = other.xMin();
    type58[29].value = other.xStep();

    type58[32].value = abscissaType(other.xName());
    type58[36].value = abscissaTypeDescription(type58[32].value.toInt());
    type58[37].value = other.xName(); if (other.xName().isEmpty()) type58[37].value = "NONE";

    type58[44].value = other.yName(); if (other.yName().isEmpty()) type58[44].value = "NONE";

    _populated = true;
    dataPosition=-1;
}

Function::Function(Function &other) : Channel(other)
{DD;
    temporalCorrection = other.temporalCorrection;
//    oldCorrectionValue = other.oldCorrectionValue;

    header = other.header;

    type58 = other.type58;
    _populated = other._populated;
    dataPosition = -1;

//    _data = new DataHolder(*(other.data()));
}

Function::~Function()
{DD;

}

void Function::read(QTextStream &stream, qint64 pos)
{DD;
    if (pos != -1) stream.seek(pos);

    header.read(stream);
    int i=0;
    if (!header.valid) {
        i=4;
        type58[2].value=58;
    }

    for (; i<60; ++i) {
        fields[type58[i].type]->read(type58[i].value, stream);
    }

    dataPosition = stream.pos();
    if (pos == -1) {
        QString s;
        do {
            s = stream.readLine().trimmed();
        }
        while (s != "-1");
    }

    if (type58[27].value.toInt() == 1) {// abscissa, even spacing
        _data->setXValues(type58[28].value.toDouble(),
                type58[29].value.toDouble(),
                type58[26].value.toInt());
    }
    else {// abscissa, uneven spacing
        _data->setSamplesCount(type58[26].value.toInt());
    }
}

void Function::read(QDataStream &stream)
{
    stream >> header;
    stream >> type58;
    stream >> dataPosition;

    if (type58[27].value.toInt() == 1) {// abscissa, even spacing
        _data->setXValues(type58[28].value.toDouble(),
                type58[29].value.toDouble(),
                type58[26].value.toInt());
    }
    else {// abscissa, uneven spacing
        _data->setSamplesCount(type58[26].value.toInt());
    }
}

void Function::write(QTextStream &stream)
{DD;
    int samples = data()->samplesCount();

    // Temporal correction
    //fixing channel name
    if (temporalCorrection && !nameBeforeCorrection.isEmpty()) {
        setName(nameBeforeCorrection);
    }
    //fixing values with correction
    if (temporalCorrection) {
        data()->setCorrection(0.0);
    }


    //writing header
    header.write(stream);

    for (int i=0; i<60; ++i) {
        fields[type58[i].type]->print(type58[i].value, stream);
    }

    switch (type58[25].value.toInt()) {//25 Ordinate Data Type
                                        // 2 - real, single precision
                                        // 4 - real, double precision
                                        // 5 - complex, single precision
                                        // 6 - complex, double precision

        //                                    Data Values
        //                            Ordinate            Abscissa
        //                Case     Type     Precision     Spacing       Format
        //              -------------------------------------------------------------
        //                  1      real      single        even         6E13.5
        //                  2      real      single       uneven        6E13.5
        //                  3     complex    single        even         6E13.5
        //                  4     complex    single       uneven        6E13.5
        //                  5      real      double        even         4E20.12
        //                  6      real      double       uneven     2(E13.5,E20.12)
        //                  7     complex    double        even         4E20.12
        //                  8     complex    double       uneven      E13.5,2E20.12
        //              --------------------------------------------------------------
        case 2: {
            int j = 0;
            for (int i=0; i<samples; ++i) {
                if (data()->xValuesFormat() == DataHolder::XValuesNonUniform) {
                    fields[FTFloat13_5]->print(data()->xValue(i), stream);
                    j++;
                }
                fields[FTFloat13_5]->print(data()->yValue(i), stream);
                j++;
                if (j==6) {
                    fields[FTEmpty]->print(0, stream);
                    j=0;
                }
            }
            if (j!=0) fields[FTEmpty]->print(0, stream);
            break;
        }
        case 4: {
            int j = 0;
            for (int i=0; i<samples; ++i) {
                if (data()->xValuesFormat() == DataHolder::XValuesNonUniform) {
                    fields[FTFloat13_5]->print(data()->xValue(i), stream);
                    j++;
                }
                fields[FTFloat20_12]->print(data()->yValue(i), stream);
                j++;
                if (j==4) {
                    fields[FTEmpty]->print(0, stream);
                    j=0;
                }
            }
            if (j!=0) fields[FTEmpty]->print(0, stream);
            break;
        }
        case 5: {
            int j = 0;
            for (int i=0; i<samples; i++) {
                fields[FTFloat13_5]->print(data()->yValueComplex(i).real(), stream);
                j++;
                fields[FTFloat13_5]->print(data()->yValueComplex(i).imag(), stream);
                j++;
                if (j==6) {
                    fields[FTEmpty]->print(0, stream);
                    j=0;
                }
            }
            if (j!=0) fields[FTEmpty]->print(0, stream);
            break;
        }
        case 6: {
            int j = 0;
            for (int i=0; i<samples; i++) {
                fields[FTFloat20_12]->print(data()->yValueComplex(i).real(), stream);
                j++;
                fields[FTFloat20_12]->print(data()->yValueComplex(i).imag(), stream);
                j++;
                if (j==4) {
                    fields[FTEmpty]->print(0, stream);
                    j=0;
                }
            }
            if (j!=0) fields[FTEmpty]->print(0, stream);
            break;
        }
        default: break;
    }
//    fields[FTEmpty]->print(0, stream);
    fields[FTDelimiter]->print("", stream);
    fields[FTEmpty]->print(0, stream);
}

QString Function::functionTypeDescription(int type)
{
    switch (type) {
        case 1: return "Time Response";
        case  2: return "Auto Spectrum";
        case  3: return "Cross Spectrum";
        case  4: return "Frequency Response Function";
        case  5: return "Transmissibility";
        case  6: return "Coherence";
        case  7: return "Auto Correlation";
        case  8: return "Cross Correlation";
        case  9: return "Power Spectral Density (PSD)";
        case  10: return "Energy Spectral Density (ESD)";
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
        case  24: return "Shock Response Spectrum";
        case  25: return "Finite Impulse Response Filter";
        case  26: return "Multiple Coherence";
        case  27: return "Order Function";
        default: return "Неизв.";
    }
    return "Неизв.";
}

FileDescriptor *Function::descriptor()
{
     return parent;
}

QStringList Function::getInfoHeaders()
{DD;
    return QStringList() << QString("       Имя")
                         << QString("Ед.изм.")
                         << QString("Описание")
                         << QString("Функция")
                            ;
}


QStringList Function::getInfoData()
{DD;
    return QStringList() << name()
                         << yName()
                         << description()
                         << functionTypeDescription(type())
                            ;
}

Descriptor::DataType Function::type() const
{DD;
    return (Descriptor::DataType)type58[14].value.toInt();
}

Descriptor::OrdinateFormat Function::yFormat() const
{DD;
    return (Descriptor::OrdinateFormat)type58[25].value.toUInt();
}

void Function::populate()
{DD;
    if (_populated) return;
    _data->clear();

    _populated = false;

    QFile uff(parent->fileName());
    if (!uff.open(QFile::ReadOnly | QFile::Text)) return;

    int sc = samplesCount();

    QTextStream stream(&uff);

    Q_ASSERT_X(dataPosition != -1, "Function::populate", "Data positions have been invalidated");
    if (stream.seek(dataPosition)) {

        double thr = threshold(this->yName());
        if (this->type()==Descriptor::FrequencyResponseFunction) thr=1.0;
        _data->setThreshold(thr);

        int yValueFormat = type58[25].value.toInt() >= 5 ? DataHolder::YValuesComplex : DataHolder::YValuesAmplitudes;

        if (yName()=="dB" || yName()=="дБ" || type58[43].value.toString()=="Уровень")
            yValueFormat = DataHolder::YValuesAmplitudesInDB;

        if (type58[43].value.toString()=="Phase")
            yValueFormat = DataHolder::YValuesPhases;

        if (type58[32].value.toInt()==17 || type58[32].value.toInt()==0) // time или неизв.
            yValueFormat = DataHolder::YValuesReals;

        QVector<double> values, xvalues;
        QVector<cx_double> valuesComplex;

        if (yValueFormat == DataHolder::YValuesComplex) { //complex values
            valuesComplex = QVector<cx_double>(sc, cx_double());
        }
        else
            values = QVector<double>(sc, 0.0);

        if (type58[27].value.toInt() == 0) {
            // uneven abscissa, read data pairs
            xvalues = QVector<double>(sc, 0.0);
        }
        if (type58[25].value.toInt() < 5) {//real values
            int j=0;
            for (int i=0; i<sc; ++i) {
                double value;
                stream >> value;

                if (type58[27].value.toInt() == 0) {// uneven abscissa
                    xvalues[j] = value;
                    stream >> value;
                }

                values[j] = value;
                j++;
            }
        }
        else {//complex values
            double first, second;
            int j=0;
            for (int i=0; i<sc; ++i) {
                if (type58[27].value.toInt() == 0) {// uneven abscissa
                    stream >> first;
                    xvalues[j] = first;
                }
                stream >> first >> second; //qDebug()<<first<<second;
                valuesComplex[j] = {first, second};
                j++;
            }
        }
        if (type58[27].value.toInt() == 0) {// uneven abscissa
            _data->setXValues(xvalues);
        }
        if (type58[25].value.toInt() < 5) {//real values
            _data->setYValues(values, DataHolder::YValuesFormat(yValueFormat));
        }
        else
            _data->setYValues(valuesComplex);

        QString end = stream.readLine(); qDebug()<<"end"<<end;
        end = stream.readLine().trimmed(); qDebug()<<"end"<<end;
        Q_ASSERT(end == "-1");
    }
    _populated = true;
}

void Function::clear()
{DD;
    _data->clear();
    setPopulated(false);
}

QString Function::name() const
{DD;
    return type58[4].value.toString();
}

void Function::setName(const QString &name)
{DD;
    type58[4].value = name;
}

QString Function::description() const
{DD;
    return type58[6].value.toString();
}

void Function::setDescription(const QString &description)
{DD;
    type58[6].value = description;
}

QString Function::xName() const
{DD;
    return type58[37].value.toString();
}

QString Function::yName() const
{DD;
    return type58[44].value.toString();
}

QString Function::legendName() const
{DD;
    QString result = parent->legend();
    if (!result.isEmpty()) result.prepend(" ");

    return name() + result;
}

void Function::addCorrection(double correctionValue, bool writeToFile)
{DD;
    _data->setCorrection(correctionValue);

    if (nameBeforeCorrection.isEmpty())
        nameBeforeCorrection = name();

    temporalCorrection = !writeToFile;

    if (correctionValue == 0.0)
        setName(nameBeforeCorrection);
    else
        setName(nameBeforeCorrection + QString(correctionValue>=0?"+":"")
                +QString::number(correctionValue));
}

int Function::samplesCount() const
{DD;
    return type58[26].value.toULongLong();
}



DescriptionList UffFileDescriptor::dataDescriptor() const
{DD;
    DescriptionList result;
    result << DescriptionEntry("", header.type151[4].value.toString());
    result << DescriptionEntry("", header.type151[6].value.toString());

    return result;
}

void UffFileDescriptor::setDataDescriptor(const DescriptionList &data)
{DD;
    if (data.size()>0) {
        header.type151[4].value = makeStringFromPair(data.first());
    }
    if (data.size()>1) {
        header.type151[6].value = makeStringFromPair(data.at(1));
    }
    setChanged(true);
}

QString UffFileDescriptor::saveTimeSegment(double from, double to)
{DD;
    // 0 проверяем, чтобы этот файл имел тип временных данных
    if (type() != Descriptor::TimeResponse) return "";
    // и имел данные
    if (channelsCount() == 0) return "";

    // 1 создаем уникальное имя файла по параметрам from и to
    QString fromString, toString;
    getUniqueFromToValues(fromString, toString, from, to);
    QString suffix = QString("_%1s_%2s").arg(fromString).arg(toString);

    QString newFileName = createUniqueFileName("", fileName(), suffix, "uff", false);

    // 2 создаем новый файл
    UffFileDescriptor *newUff = new UffFileDescriptor(*this);

    // 3 ищем границы данных по параметрам from и to
    Channel *ch = channels.first();

    int sampleStart = qRound((from - ch->xMin())/ch->xStep());
    if (sampleStart<0) sampleStart = 0;
    int sampleEnd = qRound((to-ch->xMin())/ch->xStep());
    if (sampleEnd >= ch->samplesCount()) sampleEnd = ch->samplesCount() - 1;
//    newUff->setSamplesCount(sampleEnd - sampleStart + 1); //число отсчетов в новом файле

    // 4 сохраняем файл
    for (int i=0; i<this->channelsCount(); ++i) {
        bool wasPopulated = channels[i]->populated();
        if (!wasPopulated) channels[i]->populate();

        Function *ch = new Function(*(this->channels[i]));
        ch->parent = newUff;


        ch->data()->setSegment(*(channels[i]->data()), sampleStart, sampleEnd);

        ch->setPopulated(true);

        newUff->channels << ch;
        if (!wasPopulated) {
            //clearing data
            channels[i]->data()->clear();
        }
    }

    newUff->setChanged(true);
    newUff->setDataChanged(true);
    newUff->write();
    newUff->writeRawFile();
    delete newUff;

    // 5 возвращаем имя нового файла
    return newFileName;
}


int UffFileDescriptor::samplesCount() const
{
    if (channels.isEmpty()) return 0;
    return channels.first()->samplesCount();
}

void UffFileDescriptor::setSamplesCount(int count)
{
    Q_UNUSED(count);
}

void UffFileDescriptor::setChanged(bool changed)
{DD;
    FileDescriptor::setChanged(changed);
    if (changed && QFile::exists(fileName()+"~"))
        QFile::remove(fileName()+"~");
}


QDataStream &operator>>(QDataStream &stream, UffHeader &header)
{
    stream >> header.type151;
    return stream;
}

QDataStream &operator<<(QDataStream &stream, const UffHeader &header)
{
    stream << header.type151;
    return stream;
}

QDataStream &operator>>(QDataStream &stream, UffUnits &header)
{
    stream >> header.type164;
    return stream;
}

QDataStream &operator<<(QDataStream &stream, const UffUnits &header)
{
    stream << header.type164;
    return stream;
}

QDataStream &operator<<(QDataStream &stream, const FunctionHeader &header)
{
    stream << header.type1858;
    stream << header.valid;
    return stream;
}
