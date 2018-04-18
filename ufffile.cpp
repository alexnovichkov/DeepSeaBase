#include "ufffile.h"

//#include <QtGui>
#include "logging.h"
#include "mainwindow.h"
#include "algorithms.h"

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
{DD;



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
    if (!channels.isEmpty()) {
        //setXBegin(channels.first()->xBegin());
        setSamplesCount(channels.first()->samplesCount());
    }
}

void UffFileDescriptor::write()
{DD;
    if (!changed()) return;

    //be sure all channels were read. May take too much memory
    for(int i = 0; i< channels.size(); ++i) {
        if (!channels[i]->populated()) channels[i]->populate();
    }

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

QList<QPair<QString, QString> > UffFileDescriptor::dataDescriptor() const
{
    QList<QPair<QString, QString> > result;
    result << QPair<QString, QString>("", header.type151[4].value.toString());
    result << QPair<QString, QString>("", header.type151[6].value.toString());

    return result;
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

void UffFileDescriptor::setDataDescriptor(const QList<QPair<QString, QString> > &data)
{
    if (data.size()>0) {
        header.type151[4].value = makeStringFromPair(data.first());
    }
    if (data.size()>1) {
        header.type151[6].value = makeStringFromPair(data.at(1));
    }
    setChanged(true);
    //write();
}

QString UffFileDescriptor::dataDescriptorAsString() const
{
    return header.info();
}

QString UffFileDescriptor::fileType() const
{
    if (channels.isEmpty()) return "Неизв.";
    int type = channels.first()->type();
   // bool equal = true;
    for (int i=1; i<channels.size(); ++i) {
        if (channels.at(i)->type() != type) {
            return "Неизв.";
        }
    }
    return channels.first()->functionTypeDescription();
}

QStringList UffFileDescriptor::info() const
{DD;
    return QStringList()
     << QFileInfo(fileName()).completeBaseName() //QString("Файл") 1
         << header.type151[10].value.toDateTime().toString("dd.MM.yy hh:mm:ss") // QString("Дата") 2
         << fileType() // QString("Тип") 3
         << (xStep()==0 ? QString::number(samplesCount()) : QString::number(samplesCount() * xStep())) // QString("Размер") 4
         << xName() // QString("Ось Х") 5
         << QString::number(xStep()) // QString("Шаг") 6
         << QString::number(channelsCount()) // QString("Каналы")); 7
         << header.info()
         << legend();
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

QString UffFileDescriptor::dateTime() const
{DD;
    return header.type151[10].value.toDateTime().toString("dd.MM.yy hh:mm:ss");
}

double UffFileDescriptor::xStep() const
{
    if (!channels.isEmpty()) return channels.first()->xStep();
    return 0.0;
}

void UffFileDescriptor::setXStep(const double xStep)
{
    if (channels.isEmpty()) return;
    bool changed = false;

    for (int i=0; i<channels.size(); ++i) {
        if (channels.at(i)->xStep()!=xStep) {
            changed = true;
            channels[i]->type58[29].value = xStep;
        }
    }
    if (changed) setChanged(true);
    write();
}

QString UffFileDescriptor::xName() const
{
    if (channels.isEmpty()) return QString();
    QString xname = channels.first()->xName();
    for (int i=1; i<channels.size(); ++i) {
        if (channels[i]->xName() != xname) return QString();
    }
    return xname;
}

void UffFileDescriptor::setLegend(const QString &legend)
{
    if (legend == header.type151[8].value.toString()) return;
    header.type151[8].value = legend;
    setChanged(true);
}

QString UffFileDescriptor::legend() const
{
    return header.type151[8].value.toString();
}

void UffFileDescriptor::setDateTime(QDateTime dt)
{
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
    //write();
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
}

void UffFileDescriptor::calculateMean(const QList<QPair<FileDescriptor *, int> > &channels)
{DD;
    Function *ch = new Function(this);

    FileDescriptor *firstDescriptor = channels.first().first;
    Channel *firstChannel = firstDescriptor->channel(channels.first().second);

    QList<Channel*> list;
    for (int i=0; i<channels.size(); ++i)
        list << channels.at(i).first->channel(channels.at(i).second);

    // считаем данные для этого канала
    // если ось = дБ, сначала переводим значение в линейное

    //ищем наименьшее число отсчетов
    quint32 numInd = list.first()->samplesCount();
    for (int i=1; i<list.size(); ++i) {
        if (list.at(i)->samplesCount() < numInd)
            numInd = list.at(i)->samplesCount();
    }

    ch->values = QVector<double>(numInd, 0.0);

    ch->yMin = 1.0e100;
    ch->yMax = -1.0e100;

    for (quint32 i=0; i<numInd; ++i) {
        double sum = 0.0;
        for (int file = 0; file < list.size(); ++file) {
            double temp = list[file]->yValues()[i];
            if (list[file]->yName() == "дБ" || list[file]->yName() == "dB")
                temp = pow(10.0, (temp/10.0));
            sum += temp;
        }
        sum /= list.size();
        if (list[0]->yName() == "дБ" || list[0]->yName() == "dB")
            sum = 10.0 * log10(sum);
        if (sum < ch->yMin) ch->yMin = sum;
        if (sum > ch->yMax) ch->yMax = sum;
        ch->values[i] = sum;
    }

    // обновляем сведения канала
    ch->setName("Среднее");
    ch->type58[8].value = QDateTime::currentDateTime();

    QStringList l;
    for (int i=0; i<channels.size(); ++i) {
        l << QString::number(channels.at(i).second);
    }
    ch->setDescription("Среднее каналов "+l.join(","));

    ch->samples = numInd;
    ch->type58[26].value = ch->samples;
    ch->type58[25].value = 4;
    ch->type58[14].value = firstChannel->type();

    ch->type58[44].value = firstChannel->yName();
    ch->type58[37].value = firstChannel->xName();
    ch->type58[32].value = abscissaType(firstChannel->xName());
    ch->type58[36].value = abscissaTypeDescription(ch->type58[32].value.toInt());

    ch->type58[28].value = firstChannel->xBegin();
    ch->type58[29].value = firstChannel->xStep();

    ch->xMax = firstChannel->xBegin() + firstChannel->xStep() * numInd;

    ch->parent = this;

    this->channels << ch;
}

void UffFileDescriptor::calculateMovingAvg(const QList<QPair<FileDescriptor *, int> > &channels, int windowSize)
{
    for(int i = 0; i< this->channels.size(); ++i) {
        if (!this->channels[i]->populated()) this->channels[i]->populate();
    }

    for (int i=0; i<channels.size(); ++i) {
        Function *ch = new Function(this);
        FileDescriptor *firstDescriptor = channels.at(i).first;
        Channel *firstChannel = firstDescriptor->channel(channels.at(i).second);

        quint32 numInd = firstChannel->samplesCount();
        ch->values = QVector<double>(numInd, 0.0);

        quint32 span = windowSize / 2;
        bool log = firstChannel->yName() == "дБ" || firstChannel->yName() == "dB";

        for (quint32 j=span; j<numInd-span; ++j) {
            double sum = 0.0;
            for (int k=0; k<windowSize;++k) {
                double temp = firstChannel->yValues()[j-span+k];
                if (log)
                    temp = pow(10.0, (temp/10.0));
                sum += temp;
            }
            sum /= windowSize;
            if (log)
                sum = 10.0 * log10(sum);

            ch->values[j] = sum;
        }

        //начало диапазона и конец диапазона
        for (quint32 j=0; j<span; ++j)
            ch->values[j] = firstChannel->yValues()[j];
        for (quint32 j=numInd-span; j<numInd; ++j)
            ch->values[j] = firstChannel->yValues()[j];

        ch->type58[29].value = firstDescriptor->xStep();

        ch->yMin = 1.0e100;
        ch->yMax = -1.0e100;
        for (quint32 i=0; i<ch->samples; ++i) {
            if (ch->values[i] < ch->yMin) ch->yMin = ch->values[i];
            if (ch->values[i] > ch->yMax) ch->yMax = ch->values[i];
        }

        if (ch->xvalues.isEmpty())
            ch->xMax = ch->xBegin() + ch->xStep() * ch->samples;
        else
            ch->xMax = ch->xvalues.last();

        if (!firstChannel->xValues().isEmpty()) {
            ch->xvalues = firstChannel->xValues();
            ch->xMax = firstChannel->xValues().last();
        }

        // обновляем сведения канала
        ch->setPopulated(true);
        ch->setName(firstChannel->name()+" сглаж.");
        ch->setDescription("Скользящее среднее канала "+firstChannel->name());

        ch->samples = numInd;

        ch->type58[44].value = firstChannel->yName();
        ch->type58[37].value = firstChannel->xName();
        ch->parent = this;

        ch->type58[8].value = QDateTime::currentDateTime();

        ch->type58[26].value = ch->samples;
        ch->type58[25].value = 4; //real, double precision
        ch->type58[14].value = firstChannel->type();

        ch->type58[32].value = abscissaType(firstChannel->xName());
        ch->type58[36].value = abscissaTypeDescription(ch->type58[32].value.toInt());

        ch->type58[28].value = firstChannel->xBegin();

        this->channels << ch;
    }
}

FileDescriptor *UffFileDescriptor::calculateThirdOctave()
{
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

        auto result = thirdOctave(ch->yValues(), ch->xBegin(), ch->xStep());

        newCh->xvalues = result.first;
        newCh->values = result.second;
        newCh->setPopulated(true);

        newCh->samples = newCh->xvalues.size();
        newCh->xMax = newCh->xvalues.last();
        newCh->yMin = *(std::min_element(newCh->values.begin(), newCh->values.end()));
        newCh->yMax = *(std::max_element(newCh->values.begin(), newCh->values.end()));

        newCh->parent = thirdOctUff;

        newCh->type58[6].value = "Третьоктава";

        newCh->type58[25].value = 2; //25 Ordinate Data Type
    //                                       2 - real, single precision
    //                                       4 - real, double precision
    //                                       5 - complex, single precision
    //                                       6 - complex, double precision
        newCh->type58[26].value = newCh->samples; //26   Number of data pairs for uneven abscissa spacing,
                                           //  or number of data values for even abscissa spacing
        newCh->type58[27].value = 0; //27 Abscissa Spacing (1=even, 0=uneven,
        newCh->type58[28].value = 0.0; //28 Abscissa minimum
        newCh->type58[29].value = 0.0; //29 Abscissa increment
        newCh->type58[44].value = "дБ"; //Ordinate name

        thirdOctUff->channels.append(newCh);
    }


    thirdOctUff->setSamplesCount(thirdOctUff->channels.first()->samples);

    thirdOctUff->fillRest();

    thirdOctUff->setChanged(true);
    thirdOctUff->setDataChanged(true);
    thirdOctUff->write();
    thirdOctUff->writeRawFile();
    return thirdOctUff;

    return 0;
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
    //write();
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

Channel *UffFileDescriptor::channel(int index)
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
    //мы пока что не умеем считать спектры файлов uff
//    for (int i=0; i<channels.size(); ++i) {
//        if (channels.at(i)->type()==Descriptor::TimeResponse) {
//            return true;
//        }
//    }
    return false;
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
{
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


Function::Function(UffFileDescriptor *parent) : Channel(),
    xMax(0.0), yMin(0.0), yMax(0.0),
    samples(0), parent(parent), _populated(false), dataPosition(-1)
{DD;
    temporalCorrection = false;
    oldCorrectionValue = 0.0;
    setType58(type58);
}



Function::Function(Channel &other)
{
    temporalCorrection = false;
    oldCorrectionValue = 0.0;
    setType58(type58);

    samples = other.samplesCount();
    values = other.yValues();
//    values = new double[samplesCount()];
//    memcpy(static_cast<void *>(values), static_cast<void *>(other.yValues()),
//           samplesCount()*sizeof(double));
    xvalues = other.xValues();

    type58[4].value = other.name(); if (other.name().isEmpty()) type58[4].value = "NONE";
    type58[6].value = other.description(); if (other.description().isEmpty()) type58[6].value = "NONE";
    type58[8].value = QDateTime::currentDateTime();
    type58[14].value = other.type();


    type58[25].value = (unsigned int)other.yFormat();
    type58[26].value = samples;

    type58[28].value = other.xBegin();
    type58[29].value = other.xStep();

    type58[32].value = abscissaType(other.xName());
    type58[36].value = abscissaTypeDescription(type58[32].value.toInt());
    type58[37].value = other.xName(); if (other.xName().isEmpty()) type58[37].value = "NONE";

    type58[44].value = other.yName(); if (other.yName().isEmpty()) type58[44].value = "NONE";

    xMax = type58[28].value.toDouble() + type58[29].value.toDouble() * samples;
    yMin = other.yMinInitial();
    yMax = other.yMaxInitial();
    _populated = !values.isEmpty();
    dataPosition=-1;
}

Function::Function(const Function &other)
{
    temporalCorrection = other.temporalCorrection;
    oldCorrectionValue = other.oldCorrectionValue;

    header = other.header;
    samples = other.samples;
    xMax = other.xMax;
    yMin = other.yMin;
    yMax = other.yMax;

    values = other.values;
    xvalues = other.xvalues;

    parent = other.parent;
    type58 = other.type58;
    _populated = other._populated;
    dataPosition = -1;
}

Function::~Function()
{DD;
    //delete [] values;
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

    samples = type58[26].value.toULongLong();

    dataPosition = stream.pos();
    if (pos == -1) {
        QString s;
        do {
            s = stream.readLine().trimmed();
        }
        while (s != "-1");
    }
    //else stream.seek(pos);
}

void Function::write(QTextStream &stream)
{DD;
    // Temporal correction
    //fixing channel name
    if (temporalCorrection && !nameBeforeCorrection.isEmpty()) {
        setName(nameBeforeCorrection);
    }
    //fixing values with correction
    if (temporalCorrection) {
        for (quint32 i=0; i<samples; ++i)
            values[i] -= oldCorrectionValue;
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
            for (quint32 i=0; i<samples; ++i) {
                if (!xvalues.isEmpty()) {
                    fields[FTFloat13_5]->print(xvalues[i], stream);
                    j++;
                }
                fields[FTFloat13_5]->print(values[i], stream);
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
            for (quint32 i=0; i<samples; ++i) {
                if (!xvalues.isEmpty()) {
                    fields[FTFloat13_5]->print(xvalues[i], stream);
                    j++;
                }
                fields[FTFloat20_12]->print(values[i], stream);
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
            for (int i=0; i<valuesComplex.size(); i++) {
                fields[FTFloat13_5]->print(valuesComplex[i].first, stream);
                j++;
                fields[FTFloat13_5]->print(valuesComplex[i].second, stream);
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
            for (int i=0; i<valuesComplex.size(); i++) {
                fields[FTFloat20_12]->print(valuesComplex[i].first, stream);
                j++;
                fields[FTFloat20_12]->print(valuesComplex[i].second, stream);
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

QString Function::functionTypeDescription() const
{DD;
    switch (type58[14].value.toInt()) {
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
                         << functionTypeDescription()
                            ;
}

Descriptor::DataType Function::type() const
{
    return (Descriptor::DataType)type58[14].value.toInt();
}

Descriptor::OrdinateFormat Function::yFormat() const
{
    return (Descriptor::OrdinateFormat)type58[25].value.toUInt();
}

void Function::populate()
{DD;
    values.clear();
    xvalues.clear();
    valuesComplex.clear();
    _populated = false;

    QFile uff(parent->fileName());
    if (!uff.open(QFile::ReadOnly | QFile::Text)) return;
    QTextStream stream(&uff);
    if (stream.seek(dataPosition)) {//function type is not time response
        double thr = threshold(this->yName());
        if (this->type()==Descriptor::FrequencyResponseFunction) thr=1.0;

        bool doLog = (this->type()==Descriptor::Spectrum || this->type()==Descriptor::FrequencyResponseFunction
                      || this->type()==Descriptor::AutoSpectrum || this->type()==Descriptor::CrossSpectrum);
        if (yName()=="dB" || yName()=="дБ" || type58[43].value.toString()=="Уровень")
            doLog=false;

        values = QVector<double>(samples, 0.0);
        if (type58[25].value.toInt() >= 5) { //complex values
            valuesComplex = QVector<QPair<double,double> >(samples, QPair<double,double>(0.0,0.0));
        }
        if (type58[27].value.toInt() == 0) {
            // uneven abscissa, read data pairs
            xvalues = QVector<double>(samples, 0.0);
        }
        if (type58[25].value.toInt() < 5) {//real values
            quint32 j=0;
            for (quint32 i=0; i<samples; ++i) {
                double value;
                stream >> value;

                if (type58[27].value.toInt() == 0) {// uneven abscissa
                    xvalues[j] = value;
                    stream >> value;
                }

                if (doLog)
                    value = 20*log10(value/thr);
                else
                    value = value;

                values[j] = value;
                j++;
            }
            if (doLog) {
                type58[43].value = "Уровень";
                type58[44].value = "дБ";
            }
        }
        else {//complex values
            double first, second;
            quint32 j=0;
            for (quint32 i=0; i<samples; ++i) {
                if (type58[27].value.toInt() == 0) {// uneven abscissa
                    stream >> first;
                    xvalues[j] = first;
                }
                stream >> first >> second; //qDebug()<<first<<second;
                valuesComplex[j].first = first;
                valuesComplex[j].second = second;

                double amplitude = sqrt(first*first + second*second);
                if (doLog)
                    values[j] = 20*log10(amplitude/thr);
                else
                    values[j] = amplitude;
                j++;
            }
        }

        yMin = *std::min_element(values.begin(), values.end());
        yMax = *std::max_element(values.begin(), values.end());

        if (xvalues.isEmpty())
            xMax = xBegin() + xStep() * samples;
        else
            xMax = xvalues.last();

        QString end = stream.readLine(); //qDebug()<<"end"<<end;
        end = stream.readLine().trimmed(); //qDebug()<<"end"<<end;
        Q_ASSERT(end == "-1");
    }
    _populated = !values.isEmpty() || !valuesComplex.isEmpty();
}

void Function::clear()
{
    values.clear();
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

double Function::xBegin() const
{DD;
    return type58[28].value.toDouble();
}

double Function::xStep() const
{DD;
    return type58[29].value.toDouble();
}

quint32 Function::samplesCount() const
{DD;
    return samples;
}

QVector<double> &Function::yValues()
{DD;
    return values;
}

QVector<double> &Function::xValues()
{
    return xvalues;
}

double Function::xMaxInitial() const
{DD;
    return xMax;
}

double Function::yMinInitial() const
{DD;
    return yMin;
}

double Function::yMaxInitial() const
{DD;
    return yMax;
}

void Function::addCorrection(double correctionValue, bool writeToFile)
{DD;
//    for (uint j = 0; j < samplesCount(); ++j)
//        values[j] += correctionValue;

//    setName(name() + QString(correctionValue>=0?"+":"")
//                +QString::number(correctionValue));
//    yMax += correctionValue;
//    yMin += correctionValue;

    for (uint j = 0; j < samplesCount(); ++j)
        values[j] += correctionValue - oldCorrectionValue;

    yMax += correctionValue - oldCorrectionValue;
    yMin += correctionValue - oldCorrectionValue;

    oldCorrectionValue = correctionValue;

    if (nameBeforeCorrection.isEmpty())
        nameBeforeCorrection = name();

    temporalCorrection = !writeToFile;

    if (correctionValue == 0.0)
        setName(nameBeforeCorrection);
    else
        setName(nameBeforeCorrection + QString(correctionValue>=0?"+":"")
                +QString::number(correctionValue));
}

