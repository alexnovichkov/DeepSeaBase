#include "ufffile.h"

//#include <QtGui>
#include "logging.h"

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
    if (s == "hz" || s == "Гц") return 18;
    if (s == "s" || s == "c") return 17;
    if (s == "m/s" || s == "м/с") return 11;
    if (s == "m/s2" || s == "m/s^2" || s == "м/с2" || s == "м/с^2") return 12;
    if (s == "n" || s == "н") return 13;
    if (s == "pa" || s == "psi" || s == "па") return 15;
    if (s == "m" || s == "м") return 8;
    if (s == "kg" || s == "кг") return 18;

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
{DD;
    if (changed())
        write();

    qDeleteAll(channels);
}

void UffFileDescriptor::fillPreliminary(Descriptor::DataType)
{DD;

}

void UffFileDescriptor::fillRest()
{DD;

}


void UffFileDescriptor::read()
{DD;
    QFile uff(fileName());
    if (uff.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream stream(&uff);

        header.read(stream);
        units.read(stream);

//        int index = 0;
        while (!stream.atEnd()) {
            Function *f = new Function(this);
            f->read(stream);


            if (f->type() > 1) {// skip time responses
                channels << f;
//                f->index = index++;
            }
            else {
                delete f;
                continue;
            }
        }
    }
    if (!channels.isEmpty()) {
        setXBegin(channels.first()->xBegin());
        setSamplesCount(channels.first()->samplesCount());
    }

}

void UffFileDescriptor::write()
{DD;
    if (!changed()) return;

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
    // Nothing to do here
}

void UffFileDescriptor::updateDateTimeGUID()
{DD;
    QDateTime t = QDateTime::currentDateTime();
    header.type151[10].value = t;
    header.type151[12].value = t;
    header.type151[16].value = t;
    header.type151[14].value = "DeepSeaBase by Novichkov & sukin sons";
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
    write();
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
         << QString::number(samplesCount() * xStep()) // QString("Размер") 4
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

QString UffFileDescriptor::xName() const
{
    if (channels.isEmpty()) return QString();
    QString xname = channels.first()->xName();
    for (int i=1; i<channels.size(); ++i) {
        if (channels[i]->xName() != xname) return QString();
    }
    return xname;
}

void UffFileDescriptor::deleteChannels(const QVector<int> &channelsToDelete)
{DD;
    for (int i=channels.size()-1; i>=0; --i) {
        if (channelsToDelete.contains(i)) {
            delete channels.takeAt(i);
        }
    }

    setChanged(true);
    write();
}

void UffFileDescriptor::copyChannelsFrom(const QMultiHash<FileDescriptor *, int> &channelsToCopy)
{DD;
    QList<FileDescriptor*> records;
    foreach (FileDescriptor* dfd, channelsToCopy.keys()) {
        if (!records.contains(dfd)) records << dfd;
    }

    foreach (FileDescriptor *record, records) {
        UffFileDescriptor *uff = static_cast<UffFileDescriptor *>(record);
        QList<int> channelsIndexes = channelsToCopy.values(record);

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

void UffFileDescriptor::calculateMean(const QMultiHash<FileDescriptor *, int> &channels)
{DD;
    Function *ch = new Function(this);

    FileDescriptor *firstDescriptor = channels.keys().first();
    Channel *firstChannel = firstDescriptor->channel(channels.value(firstDescriptor));

    QList<Channel*> list;
    QHashIterator<FileDescriptor *, int> it(channels);
    while (it.hasNext()) {
        it.next();
        list << it.key()->channel(it.value());
    }

    // считаем данные для этого канала
    // если ось = дБ, сначала переводим значение в линейное

    //ищем наименьшее число отсчетов
    quint32 numInd = list.first()->samplesCount();
    for (int i=1; i<list.size(); ++i) {
        if (list.at(i)->samplesCount() < numInd)
            numInd = list.at(i)->samplesCount();
    }

    ch->values = new double[numInd];

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

    QList<int> ll;
    QStringList l;
    it.toFront();
    while (it.hasNext()) {
        it.next();
        ll << it.value()+1;
    }
    qSort(ll);
    foreach(int n,ll) l << QString::number(n);
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

void UffFileDescriptor::move(bool up, const QVector<int> &indexes, const QVector<int> &newIndexes)
{DD;
    int i=up?0:indexes.size()-1;
    while (1) {
        channels.move(indexes.at(i),newIndexes.at(i));
        if ((up && i==indexes.size()-1) || (!up && i==0)) break;
        i=up?i+1:i-1;
    }
    setChanged(true);
    write();
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
    return channels[channel]->getInfoHeaders();
}

Channel *UffFileDescriptor::channel(int index)
{DD;
    return channels[index];
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
//    for (int i=0; i<channels.size(); ++i) {
//        if (channels.at(i)->uffType() == 1) {
//            return true;
//        }
//    }
    return false;
}

bool UffFileDescriptor::operator ==(const FileDescriptor &descriptor)
{DD;
    return this->fileName() == descriptor.fileName();
}

bool UffFileDescriptor::dataTypeEquals(FileDescriptor *other)
{DD;
    return (this->type() == other->type());
}

QString UffFileDescriptor::fileFilters() const
{DD;
    return QString("Файлы uff (*.uff)");
}


UffHeader::UffHeader()
{DD;
     type151  = { // header
        {FTDelimiter, "-1"}, {FTEmpty, ""}, //0-1
        {FTInteger6, 151}, {FTEmpty, ""}, //2-3
        {FTString80, "NONE"}, {FTEmpty, ""},//4-5 model file name
        {FTString80, "NONE"}, {FTEmpty, ""}, //6-7 model file description
        {FTString80, "NONE"}, {FTEmpty, ""}, //8-9 program which created db
        {FTTimeDate, ""}, {FTEmpty, ""}, //10-11 date time database created DD.MM.YY HH:MM:SS
        {FTTimeDate, ""}, {FTEmpty, ""}, //12-13 date time database last saved DD.MM.YY HH:MM:SS
        {FTString80, "DeepSea Base"}, {FTEmpty, ""}, //14-15 program which created uff
        {FTTimeDate, ""}, {FTEmpty, ""}, //16-17 date time uff written DD.MM.YY HH:MM:SS
        {FTDelimiter, "-1"}, {FTEmpty, ""} //18-19
                };
}

void UffHeader::read(QTextStream &stream)
{DD;
    for (int i=0; i<20; ++i) {
        fields[type151[i].type]->read(type151[i].value, stream);
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
    type164 = {
        {FTDelimiter, ""}, {FTEmpty, ""}, //0-1
        {FTInteger6, 164}, {FTEmpty, ""}, //2-3
        {FTInteger10, 1}, {FTEmpty, ""}, //4-5 1=SI,
          //   length              force               temperature
        {FTFloat25_17, 1.0}, {FTFloat25_17, 1.0}, {FTFloat25_17, 1.0}, {FTEmpty, ""}, //6-9
          //   temperature offset
        {FTFloat25_17, 2.73160000000000030E+002}, {FTEmpty, ""}, //10-11
        {FTDelimiter, ""}, {FTEmpty, ""}, //12-13
    };
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
    type1858  = {
        {FTDelimiter, ""}, {FTEmpty, ""}, //0-1
        {FTInteger6, 1858}, {FTEmpty, ""}, //2-3

        {FTInteger12, 1}, //4 set record number
        {FTInteger12, 0}, //5 octave format, 0=not in octave format(default), 1=octave, n=1/n oct
        {FTInteger12, 0}, //6 measurement run number
        {FTInteger12, 0}, {FTInteger12, 0}, {FTInteger12, 0}, {FTEmpty, ""}, //7-10, not used

        {FTInteger6, 0}, //11 weighting type, 0=no, 1=A wei, 2=B wei, 3=C wei, 4=D wei
        {FTInteger6, 0}, //12 window type, 0=no, 1=hanning narrow, 2=hanning broad, 3=flattop,
                         //4=exponential, 5=impact, 6=impact and exponential
        {FTInteger6, 0}, //13 amplitude units, 0=unknown, 1=half-peak, 2=peak, 3=RMS
        {FTInteger6, 0}, //14 normalization method, 0=unknown, 1=units squared, 2=Units squared per Hz (PSD)
                               //3=Units squared seconds per Hz (ESD)
        {FTInteger6, 0}, //15  Abscissa Data Type Qualifier,
                          //0=translation, 1=rotation, 2=translation squared, 3=rotation squared
        {FTInteger6, 0}, //16 Ordinate Numerator Data Type Qualifier, see 15
        {FTInteger6, 0}, //17 Ordinate Denominator Data Type Qualifier, see 15
        {FTInteger6, 0}, //18 Z-axis Data Type Qualifier, see 15
        {FTInteger6, 0}, //19 sampling type, 0=dynamic, 1=static, 2=RPM from tacho, 3=freq from tach
        {FTInteger6, 0}, {FTInteger6, 0}, {FTInteger6, 0}, {FTEmpty, ""}, //20-23 not used

        {FTFloat15_7, 0.0}, //24 Z RPM value
        {FTFloat15_7, 0.0}, //25 Z time value
        {FTFloat15_7, 0.0}, //26 Z order value
        {FTFloat15_7, 0.0}, //27 number of samples
        {FTFloat15_7, 0.0}, {FTEmpty, ""}, //28-29 not used

        {FTFloat15_7, 0.0}, {FTFloat15_7, 0.0}, {FTFloat15_7, 0.0}, {FTFloat15_7, 0.0}, //30-33 user values
        {FTFloat15_7, 0.0}, {FTEmpty, ""}, //34-35 Exponential window damping factor
        {FTFloat15_7, 0.0}, {FTFloat15_7, 0.0}, {FTFloat15_7, 0.0}, {FTFloat15_7, 0.0}, {FTFloat15_7, 0.0}, {FTEmpty, ""}, //36-41 not used
        {FTString10a,"NONE  NONE"}, //42 response direction, reference direction
        {FTEmpty, ""},//43
        {FTString80,"NONE"}, {FTEmpty, ""},//44-45 not used
        {FTDelimiter,""}, {FTEmpty, ""} //46-47
    };
}

void FunctionHeader::read(QTextStream &stream)
{DD;
    for (int i=0; i<48; ++i) {
        fields[type1858[i].type]->read(type1858[i].value, stream);
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
    samples(0), values(0), parent(parent)
{DD;
    type58 = {
        {FTDelimiter,""}, {FTEmpty, ""}, //0-1
        {FTInteger6, 58}, {FTEmpty, ""}, //2-3
        {FTString80, "NONE"}, {FTEmpty, ""},//4-5 Function description, Точка 21/Сила/[(m/s2)/N]
        {FTString80, "NONE"}, {FTEmpty,""}, //6-7 ID line 2
        {FTTimeDate80, ""}, {FTEmpty,""}, //8-9 Time date of function creation
        {FTString80, "Record 1" }, {FTEmpty,""}, //10-11 ID line 4,
        {FTString80,"NONE" }, {FTEmpty,""},  //12-13 ID line 5

        {FTInteger5, 0}, //14 Function Type
    //                                       0 - General or Unknown
    //                                       1 - Time Response
    //                                       2 - Auto Spectrum
    //                                       3 - Cross Spectrum
    //                                       4 - Frequency Response Function
    //                                       5 - Transmissibility
    //                                       6 - Coherence
    //                                       7 - Auto Correlation
    //                                       8 - Cross Correlation
    //                                       9 - Power Spectral Density (PSD)
    //                                       10 - Energy Spectral Density (ESD)
    //                                       11 - Probability Density Function
    //                                       12 - Spectrum
    //                                       13 - Cumulative Frequency Distribution
    //                                       14 - Peaks Valley
    //                                       15 - Stress/Cycles
    //                                       16 - Strain/Cycles
    //                                       17 - Orbit
    //                                       18 - Mode Indicator Function
    //                                       19 - Force Pattern
    //                                       20 - Partial Power
    //                                       21 - Partial Coherence
    //                                       22 - Eigenvalue
    //                                       23 - Eigenvector
    //                                       24 - Shock Response Spectrum
    //                                       25 - Finite Impulse Response Filter
    //                                       26 - Multiple Coherence
    //                                       27 - Order Function

        {FTInteger10, 1}, //15 Function Identification Number
        {FTInteger5, 1}, //16 Version Number, or sequence number
        {FTInteger10, 0},//17 Load Case Identification Number
        {FTString10, "NONE"},//18  Response Entity Name ("NONE" if unused)
        {FTInteger10, 0}, //19 Response Node
        {FTInteger4, 0},//20 Response Direction +Z
        {FTString10, "NONE"},//21 Reference Entity Name ("NONE" if unused)
        {FTInteger10, 0}, //22 Reference Node
        {FTInteger4, 0}, //23 Reference Direction +Z
        {FTEmpty, ""}, //24

        {FTInteger10, 2}, //25 Ordinate Data Type
    //                                       2 - real, single precision
    //                                       4 - real, double precision
    //                                       5 - complex, single precision
    //                                       6 - complex, double precision
        {FTInteger10, 0}, //26   Number of data pairs for uneven abscissa
                             //     spacing, or number of data values for even abscissa spacing
        {FTInteger10, 1}, //27 Abscissa Spacing (1=even, 0=uneven,
        {FTFloat13_5, 0.0},//28 Abscissa minimum
        {FTFloat13_5, 0.0}, //29 Abscissa increment
        {FTFloat13_5, 0.0}, //30 Z-axis value
        {FTEmpty,""},  //31

        {FTInteger10, 1}, //32 Abscissa Data Characteristics
    //                                       0 - unknown
    //                                       1 - general
    //                                       2 - stress
    //                                       3 - strain
    //                                       5 - temperature
    //                                       6 - heat flux
    //                                       8 - displacement
    //                                       9 - reaction force
    //                                       11 - velocity
    //                                       12 - acceleration
    //                                       13 - excitation force
    //                                       15 - pressure
    //                                       16 - mass
    //                                       17 - time
    //                                       18 - frequency
    //                                       19 - rpm
    //                                       20 - order
        {FTInteger5, 0}, //33 Length units exponent
        {FTInteger5,0}, //34 Force units exponent
        {FTInteger5, 0}, //35 Temperature units exponent
        {FTString20, "NONE"}, //36 Axis label ("NONE" if not used)
        {FTString20, "NONE"}, //37 Axis units label ("NONE" if not used)
        {FTEmpty,""},  //38

        {FTInteger10, 1}, //39 Ordinate (or ordinate numerator) Data Characteristics
        {FTInteger5,0}, //40 Length units exponent
        {FTInteger5,0}, //41 Force units exponent
        {FTInteger5, 0}, //42 Temperature units exponent
        {FTString20, "NONE"},//43  Axis label ("NONE" if not used)
        {FTString20,"NONE" }, //44 Axis units label ("NONE" if not used)
        {FTEmpty,""},  //45

        {FTInteger10, 0}, //46 Ordinate Denominator Data Characteristics
        {FTInteger5,0}, //47
        {FTInteger5,0}, //48
        {FTInteger5,0}, //49
        {FTString20, "NONE"}, //50
        {FTString20,"NONE" }, //51
        {FTEmpty,""},  //52

        {FTInteger10, 0}, //53 Z-axis Data Characteristics
        {FTInteger5, 0}, //54
        {FTInteger5, 0}, //55
        {FTInteger5, 0}, //56
        {FTString20, "NONE" }, //57
        {FTString20, "NONE"}, //58
        {FTEmpty,""} //59

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

    };
}



Function::Function(Channel &other)
{
    type58 = {
        {FTDelimiter,""}, {FTEmpty, ""}, //0-1
        {FTInteger6, 58}, {FTEmpty, ""}, //2-3
        {FTString80, "NONE"}, {FTEmpty, ""},//4-5 Function description, Точка 21/Сила/[(m/s2)/N]
        {FTString80, "NONE"}, {FTEmpty,""}, //6-7 ID line 2
        {FTTimeDate80, ""}, {FTEmpty,""}, //8-9 Time date of function creation
        {FTString80, "Record 1" }, {FTEmpty,""}, //10-11 ID line 4,
        {FTString80,"NONE" }, {FTEmpty,""},  //12-13 ID line 5

        {FTInteger5, 0}, //14 Function Type
    //                                       0 - General or Unknown
    //                                       1 - Time Response
    //                                       2 - Auto Spectrum
    //                                       3 - Cross Spectrum
    //                                       4 - Frequency Response Function
    //                                       5 - Transmissibility
    //                                       6 - Coherence
    //                                       7 - Auto Correlation
    //                                       8 - Cross Correlation
    //                                       9 - Power Spectral Density (PSD)
    //                                       10 - Energy Spectral Density (ESD)
    //                                       11 - Probability Density Function
    //                                       12 - Spectrum
    //                                       13 - Cumulative Frequency Distribution
    //                                       14 - Peaks Valley
    //                                       15 - Stress/Cycles
    //                                       16 - Strain/Cycles
    //                                       17 - Orbit
    //                                       18 - Mode Indicator Function
    //                                       19 - Force Pattern
    //                                       20 - Partial Power
    //                                       21 - Partial Coherence
    //                                       22 - Eigenvalue
    //                                       23 - Eigenvector
    //                                       24 - Shock Response Spectrum
    //                                       25 - Finite Impulse Response Filter
    //                                       26 - Multiple Coherence
    //                                       27 - Order Function

        {FTInteger10, 1}, //15 Function Identification Number
        {FTInteger5, 1}, //16 Version Number, or sequence number
        {FTInteger10, 0},//17 Load Case Identification Number
        {FTString10, "NONE"},//18  Response Entity Name ("NONE" if unused)
        {FTInteger10, 0}, //19 Response Node
        {FTInteger4, 0},//20 Response Direction +Z
        {FTString10, "NONE"},//21 Reference Entity Name ("NONE" if unused)
        {FTInteger10, 0}, //22 Reference Node
        {FTInteger4, 0}, //23 Reference Direction +Z
        {FTEmpty, ""}, //24

        {FTInteger10, 2}, //25 Ordinate Data Type
    //                                       2 - real, single precision
    //                                       4 - real, double precision
    //                                       5 - complex, single precision
    //                                       6 - complex, double precision
        {FTInteger10, 0}, //26   Number of data pairs for uneven abscissa
                             //     spacing, or number of data values for even abscissa spacing
        {FTInteger10, 1}, //27 Abscissa Spacing (1=even, 0=uneven,
        {FTFloat13_5, 0.0},//28 Abscissa minimum
        {FTFloat13_5, 0.0}, //29 Abscissa increment
        {FTFloat13_5, 0.0}, //30 Z-axis value
        {FTEmpty,""},  //31

        {FTInteger10, 1}, //32 Abscissa Data Characteristics
    //                                       0 - unknown
    //                                       1 - general
    //                                       2 - stress
    //                                       3 - strain
    //                                       5 - temperature
    //                                       6 - heat flux
    //                                       8 - displacement
    //                                       9 - reaction force
    //                                       11 - velocity
    //                                       12 - acceleration
    //                                       13 - excitation force
    //                                       15 - pressure
    //                                       16 - mass
    //                                       17 - time
    //                                       18 - frequency
    //                                       19 - rpm
    //                                       20 - order
        {FTInteger5, 0}, //33 Length units exponent
        {FTInteger5,0}, //34 Force units exponent
        {FTInteger5, 0}, //35 Temperature units exponent
        {FTString20, "NONE"}, //36 Axis label ("NONE" if not used)
        {FTString20, "NONE"}, //37 Axis units label ("NONE" if not used)
        {FTEmpty,""},  //38

        {FTInteger10, 1}, //39 Ordinate (or ordinate numerator) Data Characteristics
        {FTInteger5,0}, //40 Length units exponent
        {FTInteger5,0}, //41 Force units exponent
        {FTInteger5, 0}, //42 Temperature units exponent
        {FTString20, "NONE"},//43  Axis label ("NONE" if not used)
        {FTString20,"NONE" }, //44 Axis units label ("NONE" if not used)
        {FTEmpty,""},  //45

        {FTInteger10, 0}, //46 Ordinate Denominator Data Characteristics
        {FTInteger5,0}, //47
        {FTInteger5,0}, //48
        {FTInteger5,0}, //49
        {FTString20, "NONE"}, //50
        {FTString20,"NONE" }, //51
        {FTEmpty,""},  //52

        {FTInteger10, 0}, //53 Z-axis Data Characteristics
        {FTInteger5, 0}, //54
        {FTInteger5, 0}, //55
        {FTInteger5, 0}, //56
        {FTString20, "NONE" }, //57
        {FTString20, "NONE"}, //58
        {FTEmpty,""} //59

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

    };

    samples = other.samplesCount();
    values = new double[samplesCount()];
    memcpy(static_cast<void *>(values), static_cast<void *>(other.yValues()),
           samplesCount()*sizeof(double));

    type58[4].value = other.name();
    type58[6].value = other.description();
    type58[8].value = QDateTime::currentDateTime();
    type58[14].value = other.type();


    type58[25].value = (unsigned int)other.yFormat();
    type58[26].value = samples;

    type58[28].value = other.xBegin();
    type58[29].value = other.xStep();

    type58[32].value = abscissaType(other.xName());
    type58[36].value = abscissaTypeDescription(type58[32].value.toInt());
    type58[37].value = other.xName();

    type58[44].value = other.yName();

    xMax = type58[28].value.toDouble() + type58[29].value.toDouble() * samples;
    yMin = other.yMinInitial();
    yMax = other.yMaxInitial();
}

Function::Function(const Function &other)
{
    header = other.header;
    samples = other.samples;
    xMax = other.xMax;
    yMin = other.yMin;
    yMax = other.yMax;

    values = new double[samplesCount()];
    memcpy(static_cast<void *>(values), static_cast<void *>(other.values),
           samplesCount()*sizeof(double));
    parent = other.parent;
    type58 = other.type58;
}

Function::~Function()
{DD;
    delete [] values;
}

void Function::read(QTextStream &stream)
{DD;
    header.read(stream);

    for (int i=0; i<60; ++i) {
        fields[type58[i].type]->read(type58[i].value, stream);
//        qDebug()<<type58[i].value;
    }

  //  streamPosition = stream.pos();
    samples = type58[26].value.toULongLong();

    if (type58[14].value.toInt() > 1) {//do not read time response, as it is too big
        if (type58[25].value.toInt() < 5) {//real values
            values = new double[samples];
            for (quint32 i=0; i<samples; ++i) {
                double yValue;
                stream >> yValue;
                values[i] = yValue;
            }
        }
        else {//complex values
            values = new double[samples];
            double first, second;
            for (quint32 i=0; i<samples; ++i) {
                stream >> first >> second; //qDebug()<<first<<second;
                double amplitude = sqrt(first*first + second*second);
                values[i] = amplitude;
            }
            type58[25].value = type58[25].value.toInt()==5?2:4;
        }

        yMin = 1.0e100;
        yMax = -1.0e100;
        for (quint32 i=0; i<samples; ++i) {
            if (values[i] < yMin) yMin = values[i];
            if (values[i] > yMax) yMax = values[i];
        }

        xMax = xBegin() + xStep() * samples;
        QString end = stream.readLine(); //qDebug()<<"end"<<end;
        end = stream.readLine(); //qDebug()<<"end"<<end;
        Q_ASSERT(end == "    -1");
    }
    else {
        QString s;
        do {
            s = stream.readLine();
        }
        while (s != "    -1");
    }
}

void Function::write(QTextStream &stream)
{
    header.write(stream);

    for (int i=0; i<60; ++i) {
        fields[type58[i].type]->print(type58[i].value, stream);
    }
//    AbstractField *f = 0;
    switch (type58[25].value.toInt()) {
        case 2: {
            int j = 0;
            for (quint32 i=0; i<samples; ++i) {
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
            for (quint32 i=0; i<samples; i+=2) {
                fields[FTFloat13_5]->print(values[i], stream);
                j++;
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
        case 6: {
            int j = 0;
            for (quint32 i=0; i<samples; i+=2) {
                fields[FTFloat20_12]->print(values[i], stream);
                j++;
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

bool Function::populated() const
{DD;
    return true;
}

void Function::setPopulated(bool populated)
{DD;
    Q_UNUSED(populated);
}

void Function::populate()
{DD;
    // Nothing to do here
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

double *Function::yValues()
{DD;
    return values;
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
