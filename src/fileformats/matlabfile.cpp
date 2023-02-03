#include "matlabfile.h"

#include "logging.h"
#include "dataholder.h"
#include "algorithms.h"
#include "unitsconverter.h"

QString uffTypeToMatType(Descriptor::DataType type)
{
    switch(type) {
        case Descriptor::TimeResponse: return "Signal";
        case Descriptor::AutoSpectrum: return "AutoPowerSpectrum";
        case Descriptor::CrossSpectrum: return "CrossPowerSpectrum";
        case Descriptor::FrequencyResponseFunction: return "FRF";
        //Transmissibility = 5,
        case Descriptor::Coherence: return "Coherence";
        //AutoCorrelation = 7,
        //CrossCorrelation = 8,
        case Descriptor::PowerSpectralDensity: return "PSD";
        case Descriptor::EnergySpectralDensity: return "ESD";
        //ProbabilityDensityFunction:
        case Descriptor::Spectrum: return "FrequencySpectrum";
//        CumulativeFrequencyDistribution = 13,
//        PeaksValley,
//        StressCycles,
//        StrainCycles,
//        Orbit,
//        ModeIndicator,
//        ForcePattern,
//        PartialPower,
//        PartialCoherence,
//        Eigenvalue,
//        Eigenvector,
//        ShockResponseSpectrum,
//        FiniteImpulseResponseFilter = 25,
//        MultipleCoherence,
//        OrderFunction
        default: break;
    }
    return "Unknown";
}

MatlabFile::MatlabFile(const QString &fileName) : FileDescriptor(fileName)
{DD;

}

MatlabFile::MatlabFile(const FileDescriptor &other, const QString &fileName, const QVector<int> &indexes) : FileDescriptor(fileName)
{
    QVector<Channel *> source;
    if (indexes.isEmpty())
        for (int i=0; i<other.channelsCount(); ++i) source << other.channel(i);
    else
        for (int i: indexes) source << other.channel(i);

    init(source);
}

MatlabFile::MatlabFile(const QVector<Channel *> &source, const QString &fileName) : FileDescriptor(fileName)
{
    init(source);
}

MatlabFile::~MatlabFile()
{DD;
    if (changed() || dataChanged())
        write();

    qDeleteAll(channels);
    for (auto var: records) Mat_VarFree(var);
    Mat_Close(matfp);
}

QStringList MatlabFile::fileFilters()
{
    return {"Файлы Matlab (*.mat)"};
}

QStringList MatlabFile::suffixes()
{
    return {"*.mat"};
}

QString matVarToString(mat_t *mat, matvar_t * var)
{
    if (!mat || !var) return QString();

    Mat_VarReadDataAll(mat, var);
    if (var->data_type == MAT_T_UTF8)
        return QString::fromUtf8((char*)var->data, var->dims[0]*var->dims[1]);
    if (var->data_type == MAT_T_UTF16)
        return QString::fromUtf16((ushort*)var->data, var->dims[0]*var->dims[1]);
    if (var->data_type == MAT_T_UTF32)
        return QString::fromUcs4((uint*)var->data, var->dims[0]*var->dims[1]);
    return QString::fromUtf8((char*)var->data, var->dims[0]*var->dims[1]);
}

template<typename T>
T matVarToNumber(mat_t *mat, matvar_t * var)
{
    if (!mat || !var) return T();

    Mat_VarReadDataAll(mat, var);
    switch (var->data_type) {
        case MAT_T_INT8:   return static_cast<T>(static_cast<qint8*>(var->data)[0]);
        case MAT_T_UINT8:  return static_cast<T>(static_cast<quint8*>(var->data)[0]);
        case MAT_T_INT16:  return static_cast<T>(static_cast<qint16*>(var->data)[0]);
        case MAT_T_UINT16: return static_cast<T>(static_cast<quint16*>(var->data)[0]);
        case MAT_T_INT32:  return static_cast<T>(static_cast<qint32*>(var->data)[0]);
        case MAT_T_UINT32: return static_cast<T>(static_cast<quint32*>(var->data)[0]);
        case MAT_T_SINGLE: return static_cast<T>(static_cast<float*>(var->data)[0]);
        case MAT_T_DOUBLE: return static_cast<T>(static_cast<double*>(var->data)[0]);
        case MAT_T_INT64:  return static_cast<T>(static_cast<qint64*>(var->data)[0]);
        case MAT_T_UINT64: return static_cast<T>(static_cast<quint64*>(var->data)[0]);
        default: break;
    }
    return T();
}

void MatlabFile::read()
{
    matfp = Mat_Open(fileName().toLocal8Bit().data(), MAT_ACC_RDONLY);
    if (NULL == matfp) return;

    while (matvar_t * matvar = Mat_VarReadNextInfo(matfp)) {
        //Mat_VarPrint(matvar, 1);
        if (matvar) {
            if (QString::fromUtf8((char*)matvar->name)=="dataDescription")
                readFileDescription(matvar);
            else
                records << matvar;
        }
    }

    ///теперь реорганизуем данные - если каналы в файле сгруппированы,
    /// то мы уже не имеем прямого соответствия record -> channel

    for (auto rec: records) {
        if (rec->class_type != MAT_C_STRUCT) continue;

        //проверяем, сгруппирован ли канал
        //запись function_record.name будет иметь размерность > 1
        if (auto function_record = Mat_VarGetStructFieldByName(rec, "function_record", 0)) {
            QVector<MatlabChannel *> toAppend;

            auto name = Mat_VarGetStructFieldByName(function_record, "name", 0);
            if (name->class_type == MAT_C_CELL) {//сгруппированные данные
                int count = name->dims[1];
                for (int i=0; i<count; ++i) {
                    MatlabChannel *channel = new MatlabChannel(this);
                    channel->grouped = true;
                    channel->indexInGroup = i;
                    channel->groupSize = count;
                    if (auto cell = Mat_VarGetCell(name, i))
                        channel->dataDescription().put("name", matVarToString(matfp, cell).section(" ", 0, 0));

                    toAppend << channel;
                }
            }
            else {//несгруппированные данные, record = channel
                MatlabChannel *channel = new MatlabChannel(this);
                channel->dataDescription().put("name", QString::fromUtf8((char*)rec->name).section("_", 0, 0));
                toAppend << channel;
            }
            //Mat_VarFree(name);

            for (MatlabChannel *channel: toAppend) {
                if (auto type = Mat_VarGetStructFieldByName(function_record, "type", 0)) {
                    channel->_type = matVarToString(matfp, type);
                }
//                XChannel c;
//                for (int j=0; j<xml.channels.size(); ++j) {
//                    if (xml.channels.at(j).catLabel == channel->_name) {
//                        c = xml.channels.at(j);
//                        break;
//                    }
//                }
//                channel->xml = c;

                //x_values
                double xbegin = 0.0;
                double xstep = 0.0;
                int samplescount = 0;
                int bands = 0;
                QString bandtype;
                QVector<double> xvalues;
                double startfrequency = 0.0;

                if (auto x_values = Mat_VarGetStructFieldByName(rec, "x_values", 0)) {
                    if (auto startValue = Mat_VarGetStructFieldByName(x_values, "start_value", 0))
                        xbegin = matVarToNumber<double>(matfp,startValue);
                    if (auto increment = Mat_VarGetStructFieldByName(x_values, "increment", 0))
                        xstep = matVarToNumber<double>(matfp, increment);
                    if (auto numberOfValues = Mat_VarGetStructFieldByName(x_values, "number_of_values", 0))
                        samplescount = matVarToNumber<int>(matfp, numberOfValues);
                    if (auto bandType = Mat_VarGetStructFieldByName(x_values, "band_type", 0))
                        bandtype = matVarToString(matfp, bandType);
                    if (auto bandsCount = Mat_VarGetStructFieldByName(x_values, "number_of_bands", 0))
                        bands = matVarToNumber<int>(matfp, bandsCount);
                    if (auto startFr = Mat_VarGetStructFieldByName(x_values, "start_frequency", 0))
                        startfrequency = matVarToNumber<double>(matfp, startFr);
                    if (auto quantity = Mat_VarGetStructFieldByName(x_values, "quantity", 0)) {
                        Mat_VarReadDataAll(matfp, quantity);
                        if (auto xname = Mat_VarGetStructFieldByName(quantity, "label", 0)) {
                            channel->dataDescription().put("xname", matVarToString(matfp, xname));
                        }
                    }
                    if (auto values = Mat_VarGetStructFieldByName(x_values, "values", 0)) {
                        Mat_VarReadDataAll(matfp, values);
                        xvalues = getDataFromMatVar(values, -1);
                    }
                }
                if (!bandtype.isEmpty() && xvalues.isEmpty()) {
                    //значения октавных полос не хранились в файле, собираем сами
                    if (bandtype == "BandOctave1_3") {
                        int startBand= qRound(10.0*log10(startfrequency));
                        for (int band=startBand; band<startBand+samplescount; ++band)
                            xvalues << std::pow(10.0, double(band)/10.0);
                    }
                    else if (bandtype == "BandOctave1_1") {
                        int startBand= qRound(10.0*log10(startfrequency));
                        for (int band=startBand; ; band+=3) {
                            xvalues << std::pow(10.0, double(band)/10.0);
                            if (xvalues.size() == samplescount) break;
                        }
                    }
                }

                int octave = 0;
//                if (c.expression.startsWith("OCTF1(")) octave = 1;
//                else if (c.expression.startsWith("OCTF3(")) octave = 3;
//                else if (c.expression.startsWith("OCTF6(")) octave = 6;
//                else if (c.expression.startsWith("OCTF2(")) octave = 2;
//                else if (c.expression.startsWith("OCTF12(")) octave = 12;
//                else if (c.expression.startsWith("OCTF24(")) octave = 24;
                if (!bandtype.isEmpty()) octave = bandtype.section('_',1,1).toInt();
                channel->dataDescription().put("function.octaveFormat", octave);

                if (xvalues.isEmpty())
                    channel->data()->setXValues(xbegin, xstep, samplescount);
                else {
                    channel->data()->setXValues(xvalues);
                }

                //y_values
                if (auto y_values = Mat_VarGetStructFieldByName(rec, "y_values", 0)) {
                    if (auto values = Mat_VarGetStructFieldByName(y_values, "values", 0)) {
                        channel->values = values;
                        channel->complex = values->isComplex!=0;
                    }
                    if (auto quantity = Mat_VarGetStructFieldByName(y_values, "quantity", 0)) {
                        //y_values.quantity.label может врать
                        QString s;
                        if (auto label = Mat_VarGetStructFieldByName(quantity, "label", 0)) {
                            s = matVarToString(matfp, label).simplified();
                            channel->dataDescription().put("yname", s);
                        }
                        if (auto unit = Mat_VarGetStructFieldByName(quantity, "unit_transformation", 0)) {
                            if (auto logref = Mat_VarGetStructFieldByName(unit, "log_reference", 0)) {
                                auto log = matVarToNumber<double>(matfp, logref);
                                channel->data()->setThreshold(log);
                                channel->dataDescription().put("function.logref", log);
                            }
                        }
                        if (s.isEmpty()) {
                            if (auto q = Mat_VarGetStructFieldByName(quantity, "quantity_terms", 0)) {
                                //Mat_VarReadDataAll(matfp, q);
                                //TODO: добавить чтение единицы
                                //s = getLabel(q);
                            }
                        }
                    }
                }

                DataHolder::YValuesFormat yformat = DataHolder::YValuesReals;
//                QString type;
//                if (auto typeS = Mat_VarGetStructFieldByName(function_record, "type", 0)) {
//                    Mat_VarReadDataAll(matfp, typeS);
//                    type = charToString(typeS);
//                }
                QString format;
                if (auto formatS = Mat_VarGetStructFieldByName(function_record, "spectrum_format", 0)) {
                    format = matVarToString(matfp, formatS);
                }
                if (channel->_type=="Signal") {
                    channel->dataDescription().put("function.name", "Time Response");
                    channel->dataDescription().put("function.type", 1);
                }
                if (channel->_type=="FrequencySpectrum") {
                    channel->dataDescription().put("function.type", 12);
                    channel->dataDescription().put("function.logscale", "linear");
                    if (channel->yName()=="deg")
                        yformat = DataHolder::YValuesPhases;
                    else
                        yformat = DataHolder::YValuesAmplitudes;
                    if (channel->dataDescription().get("function.octaveFormat").toInt()>0) {
                        channel->dataDescription().put("function.name", "OCTF");
                        channel->dataDescription().put("function.type", 9);
                    }
                    else
                        channel->dataDescription().put("function.name", "FFT");
                }
                if (channel->_type=="CrossPowerSpectrum") {
                    if (channel->yName()=="deg")
                        yformat = DataHolder::YValuesPhases;
                    else
                        yformat = DataHolder::YValuesAmplitudes;
                    if (format=="LINEAR") {
                        channel->dataDescription().put("function.name", "GXYN");
                    }
                    else {
                        channel->dataDescription().put("function.name", "GXY");
                    }
                    channel->dataDescription().put("function.type", 3);
                }
                if (channel->_type=="Coherence") {
                    channel->dataDescription().put("function.name", "COH");
                    channel->dataDescription().put("function.type", 6);
                    channel->dataDescription().put("function.logscale", "dimensionless");
                    channel->dataDescription().put("function.logref", 1);
                }
                if (channel->_type=="PSD") {
                    yformat = DataHolder::YValuesAmplitudes;
                    channel->dataDescription().put("function.name", "PSD");
                    channel->dataDescription().put("function.type", 9);
//                    channel->dataDescription().put("function.logscale", "dimensionless");
                }
                if (channel->_type=="AutoPowerSpectrum") {
                    yformat = DataHolder::YValuesAmplitudes;
                    channel->dataDescription().put("function.name", "APS");
                    channel->dataDescription().put("function.type", 2);
                }


                if (channel->complex) yformat = DataHolder::YValuesComplex;
//                else if (c.expression.startsWith("FFT(") ||
//                         c.expression.startsWith("GXY(") ||
//                         c.expression.startsWith("GXYN(")||
//                         c.expression.startsWith("FRF(")) {
//                    if (c.fftDataType == 1) yformat = DataHolder::YValuesAmplitudes;
//                    else if (c.fftDataType == 2) yformat = DataHolder::YValuesPhases;
//                }
//                else if (c.expression.startsWith("PSD(") ||
//                         c.expression.startsWith("ESD(") ||
//                         c.expression.startsWith("APS(") ||
//                         c.expression.startsWith("OCT"))
//                    yformat = DataHolder::YValuesAmplitudes;
                channel->data()->setYValuesFormat(yformat);

//                channel->data()->setThreshold(c.logRef);

                auto units = DataHolder::UnitsUnknown;
//                if (c.scale == 10) units = DataHolder::UnitsQuadratic;
//                else if (c.scale == 20) units = DataHolder::UnitsLinear;
                if (format == "LINEAR") units = DataHolder::UnitsLinear;
                else if (format == "POWER") units = DataHolder::UnitsQuadratic;
                channel->data()->setYValuesUnits(units);

                //z_values
                if (auto z_values = Mat_VarGetStructFieldByName(rec, "z_values", 0)) {
                    double zMin = 0;
                    double zStep = 0;
                    int zCount = 0;
                    QVector<double> zValues;

                    if (auto startValue = Mat_VarGetStructFieldByName(z_values, "start_value", 0))
                        zMin = matVarToNumber<double>(matfp, startValue);
                    if (auto increment = Mat_VarGetStructFieldByName(z_values, "increment", 0))
                        zStep = matVarToNumber<double>(matfp, increment);
                    if (auto numberOfValues = Mat_VarGetStructFieldByName(z_values, "number_of_values", 0))
                        zCount = matVarToNumber<int>(matfp, numberOfValues);
                    if (auto quantity = Mat_VarGetStructFieldByName(z_values, "quantity", 0)) {
                        Mat_VarReadDataAll(matfp, quantity);
                        if (auto zname = Mat_VarGetStructFieldByName(quantity, "label", 0)) {
                            channel->dataDescription().put("zname", matVarToString(matfp, zname));
                        }
                    }
                    if (auto values = Mat_VarGetStructFieldByName(z_values, "values", 0)) {
                        Mat_VarReadDataAll(matfp, values);
                        zValues = getDataFromMatVar(values, -1);
                    }
                    if (zValues.isEmpty())
                        channel->data()->setZValues(zMin, zStep, zCount);
                    else
                        channel->data()->setZValues(zValues);
                }
                else
                    channel->data()->setZValues(0.0, 0.0, 1);

                if (auto time = Mat_VarGetStructFieldByName(function_record, "creation_time", 0)) {
                    Mat_VarReadDataAll(matfp, time);
                    if (time->class_type == MAT_C_CELL) {
                        if (auto t = Mat_VarGetCell(time, channel->indexInGroup)) {
                            auto timeS = matVarToString(matfp, t);
                            channel->dataDescription().put("dateTime",
                                                           QDateTime::fromString(timeS, "yyyy/MM/dd hh:mm:ss"));
                        }
                    }
                    else {
                        auto timeS = matVarToString(matfp, time);
                        channel->dataDescription().put("dateTime",
                                                       QDateTime::fromString(timeS, "yyyy/MM/dd hh:mm:ss"));
                    }
                }

//                QString ChanAddress = QString("SCADAS\\")+channel->xml.catLabel;
//                QStringList info = channel->xml.info;
//                info.append(ChanAddress);
//                channel->dataDescription().put("description",info.join(" \\"));

                //QStringList l;
                //l<<channel->xml.generalName<<channel->xml.pointId<<channel->xml.direction;
            }
            channels << toAppend;
        }
    }

//    Mat_Close(matfp);
}

void MatlabFile::write()
{

}

int MatlabFile::channelsCount() const
{
    return channels.size();
}

Channel *MatlabFile::channel(int index) const
{
    if (channels.size()>index)
        return channels.at(index);
    return nullptr;
}

QString MatlabFile::icon() const
{
    return ":/icons/mat.svg";
}

//bool MatlabFile::rename(const QString &newName, const QString &newPath)
//{
//}

//bool MatlabFile::copyTo(const QString &name)
//{
//}

//Descriptor::DataType MatlabFile::type() const
//{

//}

//QString MatlabFile::typeDisplay() const
//{
//}

//bool MatlabFile::fileExists() const
//{
//}

//bool MatlabFile::isSourceFile() const
//{
//}

//bool MatlabFile::operator==(const FileDescriptor &descriptor)
//{
//}

//bool MatlabFile::dataTypeEquals(FileDescriptor *other) const
//{
//}

//bool MatlabFile::canTakeChannelsFrom(FileDescriptor *other) const
//{
//}

//bool MatlabFile::canTakeAnyChannels() const
//{
//}

//qint64 MatlabFile::fileSize() const
//{
//}

//void MatlabFile::setChanged(bool changed)
//{
//}

void MatlabFile::init(const QVector<Channel *> &source)
{
    if (source.isEmpty()) return;

    auto other = source.first()->descriptor();

    setDataDescription(other->dataDescription());
    updateDateTimeGUID();

    if (channelsFromSameFile(source)) {
        dataDescription().put("source.file", other->fileName());
        dataDescription().put("source.guid", other->dataDescription().get("guid"));
        dataDescription().put("source.dateTime", other->dataDescription().dateTime("dateTime"));
        if (other->channelsCount() > source.size()) {
            //только если копируем не все каналы
            dataDescription().put("source.channels", stringify(channelIndexes(source)));
        }
    }

    //сохраняем файл, попутно подсасывая данные из other
    matfp = Mat_CreateVer(fileName().toLocal8Bit().data(), NULL, MAT_FT_MAT73);
    if (!matfp) {
        LOG(ERROR)<<"Couldn't open file"<<fileName()<<"to write";
        return;
    }

    auto header = createFileDescription();
    Mat_VarWrite(matfp, header, MAT_COMPRESSION_NONE);
    Mat_VarFree(header);

    //заполнение каналов

    //int id=1;
    for (auto ch: qAsConst(source)) {
        bool populated = ch->populated();
        if (!populated) ch->populate();

        MatlabChannel *f = new MatlabChannel(*ch, this);

        f->write(matfp);

        //clearing
        if (!populated) ch->clear();
        f->clear();
    }
}

matvar_t* createUtf8Field(const QString &val)
{
    auto data = val.toUtf8();
    size_t dim[2] = { 1, size_t(data.size()) };
    auto field = Mat_VarCreate(NULL, MAT_C_CHAR, MAT_T_UTF8, 2, dim, data.data(), 0);
    return field;
}
matvar_t* createDoubleField(double val)
{
    size_t dim[2] = { 1, 1 };
    return Mat_VarCreate(NULL, MAT_C_DOUBLE, MAT_T_DOUBLE, 2, dim, &val, 0);
}
matvar_t* createDoubleField(const QVector<double> &val)
{
    size_t dim[2] = { size_t(val.size()), 1 };
    return Mat_VarCreate(NULL, MAT_C_DOUBLE, MAT_T_DOUBLE, 2, dim, (void*)val.data(), 0);
}
matvar_t* createSingleField(const QVector<double> &val, const size_t N, const size_t n)
{
    size_t dim[2] = { N, n };
    QVector<float> data(val.size());
    std::copy(val.cbegin(),val.cend(), data.begin());
    return Mat_VarCreate(NULL, MAT_C_SINGLE, MAT_T_SINGLE, 2, dim, (void*)data.data(), 0);
}
matvar_t* createSingleField(const QVector<cx_double> &val, const size_t N, const size_t n)
{
    size_t dim[2] = { N, n };
    mat_complex_split_t t;
    Q_ASSERT(size_t(val.size())==N*n);

    QVector<float> re(N*n);
    QVector<float> im(N*n);

    for (size_t i=0; i<N*n; ++i) {
        re[i] = val[i].real();
        im[i] = val[i].imag();
    }
    t.Re = re.data();
    t.Im = im.data();

    return Mat_VarCreate(NULL, MAT_C_SINGLE, MAT_T_SINGLE, 2, dim, (void*)&t, MAT_F_COMPLEX);
}

matvar_t *MatlabFile::createFileDescription() const
{
    /**
     * Описание файла:
     * {
     *   "dateTime" : "dd.MM.yyyy hh:mm", //создание базы данных
     *   "fileCreationTime": "dd.MM.yyyy hh:mm", //создание этого файла
     *   "createdBy": "",//программа, которая записала файл
     *   "legend" : "",
     *   "description": {
     *       "свойства конкретного файла" //аналогично DFD DataDescription
     *   },
     *   "guid": "",
     *   "source": {
     *     "file": "",
     *     "guid": "",
     *     "dateTime": "dd.MM.yyyy hh:mm",
     *     "channels": "1,2,3,4,5"
     *   },*/
    //Структура из 7 полей
    const char *fieldnames[7] = { "dateTime", "fileCreationTime", "createdBy",  "legend",
                                    "description", "guid", "source"};
    size_t structdim[2] = { 1, 1 };
    matvar_t* mat = Mat_VarCreateStruct("DataDescription", 2, structdim, fieldnames, 7);

    //1. dateTime

    auto dateTime = dataDescription().dateTime("dateTime");
    if (!dateTime.isValid()) dateTime = QDateTime::currentDateTime();
    auto field1 = createUtf8Field(dateTime.toString("yyyy/MM/dd hh:mm:ss"));
    Mat_VarSetStructFieldByName(mat, fieldnames[0], 0, field1);

    //2. fileCreationTime

    auto fileCreationTime = dataDescription().get("fileCreationTime").toDateTime();
    if (!fileCreationTime.isValid()) fileCreationTime = QDateTime::currentDateTime();
    auto field2 = createUtf8Field(fileCreationTime.toString("yyyy/MM/dd hh:mm:ss"));
    Mat_VarSetStructFieldByName(mat, fieldnames[1], 0, field2);

    //3. createdBy

    auto createdBy = dataDescription().get("createdBy").toString();
    if (createdBy.isEmpty()) createdBy = "DeepSea Base";
    auto field3 = createUtf8Field(createdBy);
    Mat_VarSetStructFieldByName(mat, fieldnames[2], 0, field3);

    //4. legend

    auto legend = dataDescription().get("legend").toString(); if (legend.isEmpty()) legend=" ";
    auto field4 = createUtf8Field(legend);
    Mat_VarSetStructFieldByName(mat, fieldnames[3], 0, field4);

    //5. description

    auto field5 = createDescription();
    Mat_VarSetStructFieldByName(mat, fieldnames[4], 0, field5);

    //6. guid

    auto guid = dataDescription().get("guid").toString(); if (guid.isEmpty()) guid=createGUID();
    auto field6 = createUtf8Field(guid);
    Mat_VarSetStructFieldByName(mat, fieldnames[5], 0, field6);

    //7. source

    auto field7 = createSourceDescription();
    Mat_VarSetStructFieldByName(mat, fieldnames[6], 0, field7);

    return mat;
}

void MatlabFile::readFileDescription(matvar_t *mat)
{
    if (!mat) return;
    Mat_VarReadDataAll(matfp, mat);

    DataDescription d;

    auto f = Mat_VarGetStructFieldByName(mat, "dateTime", 0);
    d.put("dateTime", QDateTime::fromString(matVarToString(matfp,f), "yyyy/MM/dd hh:mm:ss"));
    f = Mat_VarGetStructFieldByName(mat, "fileCreationTime", 0);
    d.put("fileCreationTime", QDateTime::fromString(matVarToString(matfp,f), "yyyy/MM/dd hh:mm:ss"));
    f = Mat_VarGetStructFieldByName(mat, "createdBy", 0);
    d.put("createdBy", matVarToString(matfp,f));
    f = Mat_VarGetStructFieldByName(mat, "legend", 0);
    d.put("legend", matVarToString(matfp,f));
    f = Mat_VarGetStructFieldByName(mat, "guid", 0);
    d.put("guid", matVarToString(matfp,f));
    f = Mat_VarGetStructFieldByName(mat, "description", 0);
    if (f) {
        Mat_VarReadDataAll(matfp,f);
        auto count = Mat_VarGetNumberOfFields(f);
        for (unsigned i = 0; i<count; ++i) {
            auto ff = Mat_VarGetStructFieldByIndex(f, i, 0);
            d.put(QString("description.%1").arg(ff->name), matVarToString(matfp,ff));
        }
    }
    f = Mat_VarGetStructFieldByName(mat, "source", 0);
    if (f) {
        Mat_VarReadDataAll(matfp,f);
        auto count = Mat_VarGetNumberOfFields(f);
        for (unsigned i = 0; i<count; ++i) {
            auto ff = Mat_VarGetStructFieldByIndex(f, i, 0);
            d.put(QString("source.%1").arg(ff->name), matVarToString(matfp, ff));
        }
    }

    setDataDescription(d);
}

matvar_t *MatlabFile::createDescription() const
{
    const auto description = dataDescription().filter("description");
    size_t dims[2] = {1, 1};
    matvar_t* mat = Mat_VarCreateStruct("description", 2, dims, 0, 0);

    for (const auto &[key, val]: asKeyValueRange(description)) {
        auto var = createUtf8Field(val.toString());
        Mat_VarAddStructField(mat, key.toUtf8().data());
        Mat_VarSetStructFieldByName(mat, key.toUtf8().data(), 0, var);
    }
    return mat;
}

matvar_t *MatlabFile::createSourceDescription() const
{
    const auto description = dataDescription().filter("source");
    size_t dims[2] = {1, 1};
    matvar_t* mat = Mat_VarCreateStruct("source", 2, dims, 0, 0);

    for (const auto &[key, val]: asKeyValueRange(description)) {
        auto var = createUtf8Field(val.toString());
        Mat_VarAddStructField(mat, key.toUtf8().data());
        Mat_VarSetStructFieldByName(mat, key.toUtf8().data(), 0, var);
    }
    return mat;
}

MatlabChannel::MatlabChannel(MatlabFile *parent) : Channel(), parent(parent)
{

}

MatlabChannel::MatlabChannel(Channel &other, MatlabFile *parent) : Channel(other), parent(parent)
{DD;
    parent->channels << this;
    complex = other.data()->yValuesFormat() == DataHolder::YValuesComplex;
    _type = uffTypeToMatType(other.type());

    dataDescription().put("dateTime", QDateTime::currentDateTime());
}

//QVariant MatlabChannel::info(int column, bool edit) const
//{

//}

//int MatlabChannel::columnsCount() const
//{
//}

//QVariant MatlabChannel::channelHeader(int column) const
//{
//}

Descriptor::DataType MatlabChannel::type() const
{
    if (_type == "Signal") return Descriptor::TimeResponse;
    if (_type == "FRF") return Descriptor::FrequencyResponseFunction;
    if (_type == "FrequencySpectrum") return Descriptor::Spectrum;
    if (_type == "PSD") {
        if (xml.expression.startsWith("ESD")) return Descriptor::EnergySpectralDensity;
        return Descriptor::PowerSpectralDensity;
    }
    if (_type == "ESD") return Descriptor::EnergySpectralDensity; //никак не отличить
    if (_type == "AutoPowerSpectrum") return Descriptor::AutoSpectrum;
    if (_type == "CrossPowerSpectrum") return Descriptor::CrossSpectrum;
    if (_type == "Coherence") return Descriptor::Coherence;
    //TODO: добавить типов функций

    return Descriptor::Unknown;
}

QVector<cx_double> getDataFromMatVarComplex(matvar_t *v, int index)
{
    //index=-1 означает все блоки сразу
    const size_t N = v->dims[0];
    const size_t n = v->dims[1];
    const size_t M = index==-1 ? N*n : N;

    //Если данные не сгруппированы, то N=1, n=sampleCount
    //Если данные сгруппированы, то N=sampleCount, n=channelCount
    //Если данные многоблочные, то N=sampleCount, n=blockCount

    Q_ASSERT(v->isComplex!=0);

    const int offset = index==-1 ? 0 : N*index;
    auto c = (mat_complex_split_t*)v->data;
    QVector<cx_double> result(M);
    switch (v->class_type) {
        case MAT_C_DOUBLE: {
            double *d1 = (double*)c->Re;
            double *d2 = (double*)c->Im;
            for (size_t i=0; i<M; ++i) result[i] = {d1[offset+i], d2[offset+i]};
            break;
        }
        case MAT_C_SINGLE: {
            float *d1 = (float*)c->Re;
            float *d2 = (float*)c->Im;
            for (size_t i=0; i<M; ++i) result[i] = {d1[offset+i], d2[offset+i]};
            break;
        }
        case MAT_C_INT8: {
            int8_t *d1 = (int8_t*)c->Re;
            int8_t *d2 = (int8_t*)c->Im;
            for (size_t i=0; i<M; ++i) result[i] = {double(d1[offset+i]), double(d2[offset+i])};
            break;
        }
        case MAT_C_UINT8: {
            uint8_t *d1 = (uint8_t*)c->Re;
            uint8_t *d2 = (uint8_t*)c->Im;
            for (size_t i=0; i<M; ++i) result[i] = {double(d1[offset+i]), double(d2[offset+i])};
            break;
        }
        case MAT_C_INT16: {
            int16_t *d1 = (int16_t*)c->Re;
            int16_t *d2 = (int16_t*)c->Im;
            for (size_t i=0; i<M; ++i) result[i] = {double(d1[offset+i]), double(d2[offset+i])};
            break;
        }
        case MAT_C_UINT16: {
            uint16_t *d1 = (uint16_t*)c->Re;
            uint16_t *d2 = (uint16_t*)c->Im;
            for (size_t i=0; i<M; ++i) result[i] = {double(d1[offset+i]), double(d2[offset+i])};
            break;
        }
        case MAT_C_INT32: {
            int32_t *d1 = (int32_t*)c->Re;
            int32_t *d2 = (int32_t*)c->Im;
            for (size_t i=0; i<M; ++i) result[i] = {double(d1[offset+i]), double(d2[offset+i])};
            break;
        }
        case MAT_C_UINT32: {
            uint32_t *d1 = (uint32_t*)c->Re;
            uint32_t *d2 = (uint32_t*)c->Im;
            for (size_t i=0; i<M; ++i) result[i] = {double(d1[offset+i]), double(d2[offset+i])};
            break;
        }
        case MAT_C_INT64: {
            int64_t *d1 = (int64_t*)c->Re;
            int64_t *d2 = (int64_t*)c->Im;
            for (size_t i=0; i<M; ++i) result[i] = {double(d1[offset+i]), double(d2[offset+i])};
            break;
        }
        case MAT_C_UINT64: {
            uint64_t *d1 = (uint64_t*)c->Re;
            uint64_t *d2 = (uint64_t*)c->Im;
            for (size_t i=0; i<M; ++i) result[i] = {double(d1[offset+i]), double(d2[offset+i])};
            break;
        }
        default: break;
    }
    return result;
}

QVector<double> getDataFromMatVar(matvar_t *v, int index)
{
    //index=-1 означает все блоки сразу
    const size_t N = v->dims[0];
    const size_t n = v->dims[1];

    //Если данные не сгруппированы, то N=1, n=sampleCount
    //Если данные сгруппированы, то N=sampleCount, n=channelCount
    //Если данные многоблочные, то N=sampleCount, n=blockCount

    const size_t M = index==-1 ? N*n : N;
    QVector<double> result(M);
    const int offset = index==-1 ? 0 : N*index;
    switch (v->class_type) {
        case MAT_C_DOUBLE: {
            double *d = (double*)v->data;
            for (size_t i=0; i<M; ++i) result[i] = d[offset+i];
            break;
        }
        case MAT_C_SINGLE: {
            float *d = (float*)v->data;
            for (size_t i=0; i<M; ++i) result[i] = d[offset+i];
            break;
        }
        case MAT_C_INT8: {
            int8_t *d = (int8_t*)v->data;
            for (size_t i=0; i<M; ++i) result[i] = d[offset+i];
            break;
        }
        case MAT_C_UINT8: {
            uint8_t *d = (uint8_t*)v->data;
            for (size_t i=0; i<M; ++i) result[i] = d[offset+i];
            break;
        }
        case MAT_C_INT16: {
            int16_t *d = (int16_t*)v->data;
            for (size_t i=0; i<M; ++i) result[i] = d[offset+i];
            break;
        }
        case MAT_C_UINT16: {
            uint16_t *d = (uint16_t*)v->data;
            for (size_t i=0; i<M; ++i) result[i] = d[offset+i];
            break;
        }
        case MAT_C_INT32: {
            int32_t *d = (int32_t*)v->data;
            for (size_t i=0; i<M; ++i) result[i] = d[offset+i];
            break;
        }
        case MAT_C_UINT32: {
            uint32_t *d = (uint32_t*)v->data;
            for (size_t i=0; i<M; ++i) result[i] = d[offset+i];
            break;
        }
        case MAT_C_INT64: {
            int64_t *d = (int64_t*)v->data;
            for (size_t i=0; i<M; ++i) result[i] = d[offset+i];
            break;
        }
        case MAT_C_UINT64: {
            uint64_t *d = (uint64_t*)v->data;
            for (size_t i=0; i<M; ++i) result[i] = d[offset+i];
            break;
        }
        default: break;
    }
    return result;
}

void MatlabChannel::populate()
{
    _data->clear();
    setPopulated(false);

    if (!values) return;

    Mat_VarReadDataAll(parent->matfp, values);
    int index = grouped?indexInGroup:-1;
    if (values->isComplex) {
        auto data = getDataFromMatVarComplex(values, index);
        _data->setYValues(data);
    }
    else {
        auto data = getDataFromMatVar(values, index);
        _data->setYValues(data, _data->yValuesFormat());
    }

    setPopulated(true);
}

//void MatlabChannel::setXStep(double xStep)
//{
//}

FileDescriptor *MatlabChannel::descriptor() const
{
    return parent;
}

int MatlabChannel::index() const
{
    return parent->channels.indexOf(const_cast<MatlabChannel*>(this), 0);
}

void MatlabChannel::write(mat_t *matfp)
{
    auto mat_v = createMatVar();
    Mat_VarWrite(matfp, mat_v, MAT_COMPRESSION_NONE);
    Mat_VarFree(mat_v);
}

matvar_t *MatlabChannel::createMatVar()
{
    //always ungrouped
    //Структура из 3 полей
    const char *fieldnames[4] = { "x_values", "y_values", "z_values", "function_record"};
    size_t structdim[2] = { 1, 1 };
    matvar_t* mat = Mat_VarCreateStruct(name().toUtf8().data(), 2, structdim, fieldnames, 4);

    //1. x_values
    auto xValues = createXValuesVar();
    if (xValues) Mat_VarSetStructFieldByName(mat, fieldnames[0], 0, xValues);

    //2. y_values
    auto yValues = createYValuesVar();
    if (yValues) Mat_VarSetStructFieldByName(mat, fieldnames[1], 0, yValues);

    //3. z_values
    auto zValues = createZValuesVar();
    if (zValues) Mat_VarSetStructFieldByName(mat, fieldnames[0], 0, zValues);

    //4. function_record
    auto fr = createFunctionRecord();
    if (fr) Mat_VarSetStructFieldByName(mat, fieldnames[2], 0, fr);
    return mat;
}

matvar_t *MatlabChannel::createXValuesVar()
{
    matvar_t* mat = nullptr;
    size_t structdim[2] = { 1, 1 };
    if (data()->xValuesFormat()==DataHolder::XValuesUniform) {
        //Структура из 4 полей
        const char *fieldnames[4] = { "start_value", "increment", "number_of_values", "quantity"};
        mat = Mat_VarCreateStruct("x_values", 2, structdim, fieldnames, 4);

        QVector<double> vals = {data()->xMin(), data()->xStep(), double(data()->samplesCount())};
        for (int i=0; i<3; ++i) {
            auto var = Mat_VarCreate(fieldnames[i], MAT_C_DOUBLE, MAT_T_DOUBLE, 2, structdim, &vals[i], 0);
            Mat_VarSetStructFieldByName(mat, fieldnames[i], 0, var);
        }
    }
    else {
        //Структура из 5 полей
        QString bandType;
        int t = dataDescription().get("octave_type").toInt();
        switch (t) {
            case 1: bandType="BandOctave1_1"; break;
            case 2: bandType="BandOctave1_2"; break;
            case 3: bandType="BandOctave1_3"; break;
            case 6: bandType="BandOctave1_6"; break;
            case 12: bandType="BandOctave1_12"; break;
            case 24: bandType="BandOctave1_24"; break;
        }

        if (t>0) {
            const char *fieldnames[5] = { "start_frequency", "band_type", "number_of_bands", "number_of_values", "quantity"};
            mat = Mat_VarCreateStruct("x_values", 2, structdim, fieldnames, 5);

            auto v = createDoubleField(data()->xMin());
            Mat_VarSetStructFieldByName(mat, fieldnames[0], 0, v);
            v = createUtf8Field(bandType);
            Mat_VarSetStructFieldByName(mat, fieldnames[1], 0, v);
            v = createDoubleField(data()->samplesCount());
            Mat_VarSetStructFieldByName(mat, fieldnames[2], 0, v);
            v = createDoubleField(data()->samplesCount());
            Mat_VarSetStructFieldByName(mat, fieldnames[3], 0, v);
        }
        else {
            const char *fieldnames[2] = {"values", "quantity"};
            mat = Mat_VarCreateStruct("x_values", 2, structdim, fieldnames, 2);
            auto v = createSingleField(data()->xValues(), data()->samplesCount(), 1);
            Mat_VarSetStructFieldByName(mat, fieldnames[0], 0, v);
        }
    }
    if (mat) {
        auto q = createQuantity("x");
        if (q) Mat_VarSetStructFieldByName(mat, "quantity", 0, q);
    }

    return mat;
}

matvar_t *MatlabChannel::createYValuesVar()
{
    matvar_t* mat = nullptr;
    size_t structdim[2] = { 1, 1 };
    const char *fieldnames[2] = { "values", "quantity"};
    mat = Mat_VarCreateStruct("y_values", 2, structdim, fieldnames, 2);

    matvar_t *vals;
    if (data()->yValuesFormat()==DataHolder::YValuesComplex)
        vals = createSingleField(data()->yValuesComplex(-1), data()->samplesCount(), data()->blocksCount());
    else
        vals = createSingleField(data()->rawYValues(-1), data()->samplesCount(), data()->blocksCount());
    if (vals)
        Mat_VarSetStructFieldByName(mat, fieldnames[0], 0, vals);

    auto q = createQuantity("y");
    if (q) Mat_VarSetStructFieldByName(mat, fieldnames[1], 0, q);

    return mat;
}

matvar_t *MatlabChannel::createZValuesVar()
{
    matvar_t* mat = nullptr;
    size_t structdim[2] = { 1, 1 };
    if (data()->zValuesFormat()==DataHolder::XValuesUniform) {
        //Структура из 4 полей
        const char *fieldnames[4] = { "start_value", "increment", "number_of_values", "quantity"};
        mat = Mat_VarCreateStruct("z_values", 2, structdim, fieldnames, 4);

        QVector<double> vals = {data()->zMin(), data()->zStep(), double(data()->blocksCount())};
        for (int i=0; i<3; ++i) {
            auto var = Mat_VarCreate(fieldnames[i], MAT_C_DOUBLE, MAT_T_DOUBLE, 2, structdim, &vals[i], 0);
            Mat_VarSetStructFieldByName(mat, fieldnames[i], 0, var);
        }
    }
    else {
        const char *fieldnames[2] = {"values", "quantity"};
        mat = Mat_VarCreateStruct("z_values", 2, structdim, fieldnames, 2);
        auto v = createSingleField(data()->zValues(), data()->blocksCount(), 1);
        Mat_VarSetStructFieldByName(mat, fieldnames[0], 0, v);
    }
    if (mat) {
        auto q = createQuantity("z");
        if (q) Mat_VarSetStructFieldByName(mat, "quantity", 0, q);
    }

    return mat;
}

matvar_t *MatlabChannel::createQuantity(QString label)
{
    matvar_t* mat = nullptr;
    size_t structdim[2] = { 1, 1 };
    const char *fieldnames[4] = { "label", "unit_transformation", "quantity_terms", "info"};
    mat = Mat_VarCreateStruct("quantity", 2, structdim, fieldnames, 4);

    auto v = createUtf8Field(label=="y"?yName():(label=="x"?xName():zName()));
    Mat_VarSetStructFieldByName(mat, fieldnames[0], 0, v);

    v = createUnitTransformation(label);
    Mat_VarSetStructFieldByName(mat, fieldnames[1], 0, v);

    v = createQuantityTerms(label);
    Mat_VarSetStructFieldByName(mat, fieldnames[2], 0, v);

    v = createUtf8Field("Data saved by DeepSeaBase");
    Mat_VarSetStructFieldByName(mat, fieldnames[3], 0, v);

    return mat;
}

QVector<double> getQuantities(QString s)
{
    QVector<double> v(8, 0); //{ "LENGTH", "ANGLE", "MASS", "TIME", "CURRENT",
    //"LIGHT", "TEMPERATURE", "MOLCULAR_AMOUNT"};
    s=s.toLower();
    auto type = PhysicalUnits::Units::unitType(s);
    switch (type) {
        case PhysicalUnits::Units::Type::Stress: v[0]=-1; v[2]=1; v[3]=-2; break;
        case PhysicalUnits::Units::Type::Strain: break; //dimensionless
        case PhysicalUnits::Units::Type::Temperature: v[6]=1; break;
        case PhysicalUnits::Units::Type::HeatFlux: v[2]=1; v[3]=-3; break;
        case PhysicalUnits::Units::Type::Displacement: v[2]=1; break;
        case PhysicalUnits::Units::Type::ReactionForce: v[0]=1; v[2]=1; v[3]=-2; break;
        case PhysicalUnits::Units::Type::Velocity: v[0]=1; v[3]=-1; break;
        case PhysicalUnits::Units::Type::Acceleration: v[0]=1; v[3]=-2; break;
        case PhysicalUnits::Units::Type::ExcitationForce: v[0]=1; v[2]=1; v[3]=-2; break;
        case PhysicalUnits::Units::Type::Pressure: v[0]=-1; v[2]=1; v[3]=-2; break;
        case PhysicalUnits::Units::Type::Mass: v[2]=1; break;
        case PhysicalUnits::Units::Type::Time: v[3]=1; break;
        case PhysicalUnits::Units::Type::Frequency: v[3]=-1; break;
        case PhysicalUnits::Units::Type::RPM: v[1]=1; v[3]=-1; break;
        case PhysicalUnits::Units::Type::Order: v[1]=-1; break;
        case PhysicalUnits::Units::Type::Voltage: v[0]=2; v[2]=1; v[3]=-3; v[4]=-1; break;
        case PhysicalUnits::Units::Type::Current: v[4]=1; break;
        default: {

        }
    }

    return v;
}

matvar_t *MatlabChannel::createQuantityTerms(QString label)
{
    size_t structdim[2] = { 1, 8 };
    const char *fieldnames[3] = { "num", "den", "quantity"};
    const char *quantities[8] = { "LENGTH", "ANGLE", "MASS", "TIME", "CURRENT",
                                  "LIGHT", "TEMPERATURE", "MOLCULAR_AMOUNT"};

    matvar_t* mat = Mat_VarCreateStruct("quantity_terms", 2, structdim, fieldnames, 3);
    auto nums = getQuantities(label=="y"?yName():(label=="x"?xName():zName()));
    for (int i=0; i<8; ++i) {
        auto v = createDoubleField(nums[i]);
        Mat_VarSetStructFieldByName(mat, fieldnames[0], i, v);
        v = createDoubleField(1);
        Mat_VarSetStructFieldByName(mat, fieldnames[1], i, v);
        v = createUtf8Field(quantities[i]);
        Mat_VarSetStructFieldByName(mat, fieldnames[2], i, v);
    }
    return mat;
}

matvar_t *MatlabChannel::createUnitTransformation(QString label)
{
    size_t structdim[2] = { 1, 1 };
    const char *fieldnames[3] = { "offset", "factor", "log_reference"};
    matvar_t* mat = Mat_VarCreateStruct("unit_transformation", 2, structdim, fieldnames, 3);

    auto v = createDoubleField(0);
    Mat_VarSetStructFieldByName(mat, fieldnames[0], 0, v);
    v = createDoubleField(1);
    Mat_VarSetStructFieldByName(mat, fieldnames[1], 0, v);
    v = createDoubleField(label=="y"?data()->threshold():1);
    Mat_VarSetStructFieldByName(mat, fieldnames[2], 0, v);

    return mat;
}

matvar_t *MatlabChannel::createFunctionRecord()
{
    size_t structdim[2] = { 1, 1 };
    const char *fieldnames[8] = { "name", "type", "creation_time", "last_modification_time",
                               "weighting", "primary_channel", "energy_amplitude_transform",
                               "average"};
    matvar_t* mat = Mat_VarCreateStruct("function_record", 2, structdim, fieldnames, 8);

    auto v = createUtf8Field(name());
    Mat_VarSetStructFieldByName(mat, fieldnames[0], 0, v);
    v = createUtf8Field(_type);
    Mat_VarSetStructFieldByName(mat, fieldnames[1], 0, v);
    auto creationTime = dataDescription().dateTime("dateTime");
    if (!creationTime.isValid()) creationTime = parent->dataDescription().dateTime("dateTime");
    if (!creationTime.isValid()) creationTime = QDateTime::currentDateTime();
    v = createUtf8Field(creationTime.toString("yyyy/MM/dd hh:mm:ss"));
    Mat_VarSetStructFieldByName(mat, fieldnames[2], 0, v);
    v = createUtf8Field(QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss"));
    Mat_VarSetStructFieldByName(mat, fieldnames[3], 0, v);


    return mat;
}
