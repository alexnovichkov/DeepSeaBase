#include "matlabfile.h"

#include "logging.h"
#include "dataholder.h"
#include "algorithms.h"

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

QString charToString(matvar_t * var)
{
    if (var->data_type == MAT_T_UTF8)
        return QString::fromUtf8((char*)var->data, var->dims[0]*var->dims[1]);
    if (var->data_type == MAT_T_UTF16)
        return QString::fromUtf16((ushort*)var->data, var->dims[0]*var->dims[1]);
    if (var->data_type == MAT_T_UTF32)
        return QString::fromUcs4((uint*)var->data, var->dims[0]*var->dims[1]);
    return QString::fromUtf8((char*)var->data, var->dims[0]*var->dims[1]);
}


void MatlabFile::read()
{
    matfp = Mat_Open(fileName().toLocal8Bit().data(), MAT_ACC_RDONLY);
    if (NULL == matfp) return;

    while (matvar_t * matvar = Mat_VarReadNextInfo(matfp)) {
        //Mat_VarPrint(matvar, 1);
        if (matvar) records << matvar;
    }

    setDataDescription(xml.toDataDescription());

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
                    if (auto cell = Mat_VarGetCell(name, i)) {
                        Mat_VarReadDataAll(matfp, cell);
                        channel->_name = charToString(cell).section(" ", 0, 0);
                    }

                    toAppend << channel;
                }
            }
            else {//несгруппированные данные, record = channel
                MatlabChannel *channel = new MatlabChannel(this);
                channel->_name = QString::fromUtf8((char*)rec->name).section("_", 0, 0);
                toAppend << channel;
            }
            //Mat_VarFree(name);

            for (MatlabChannel *channel: toAppend) {
                if (auto type = Mat_VarGetStructFieldByName(function_record, "type", 0)) {
                    Mat_VarReadDataAll(matfp, type);
                    channel->_type = charToString(type);
                }
                XChannel c;
                for (int j=0; j<xml.channels.size(); ++j) {
                    if (xml.channels.at(j).catLabel == channel->_name) {
                        c = xml.channels.at(j);
                        break;
                    }
                }
                channel->xml = c;

                //x_values
                double xbegin = 0.0;
                double xstep = 0.0;
                int samplescount = 0;
                int bands = 0;
                QString bandtype;
                QVector<double> xvalues;
                double startfrequency = 0.0;

                if (auto x_values = Mat_VarGetStructFieldByName(rec, "x_values", 0)) {
                    if (auto startValue = Mat_VarGetStructFieldByName(x_values, "start_value", 0)) {
                        Mat_VarReadDataAll(matfp, startValue);
                        xbegin = static_cast<double*>(startValue->data)[0];
                    }
                    if (auto increment = Mat_VarGetStructFieldByName(x_values, "increment", 0)) {
                        Mat_VarReadDataAll(matfp, increment);
                        xstep = static_cast<double*>(increment->data)[0];
                    }
                    if (auto numberOfValues = Mat_VarGetStructFieldByName(x_values, "number_of_values", 0)) {
                        Mat_VarReadDataAll(matfp, numberOfValues);
                        samplescount = int(static_cast<double*>(numberOfValues->data)[0]);
                    }
                    if (auto bandType = Mat_VarGetStructFieldByName(x_values, "band_type", 0)) {
                        Mat_VarReadDataAll(matfp, bandType);
                        bandtype = charToString(bandType);
                    }
                    if (auto bandsCount = Mat_VarGetStructFieldByName(x_values, "number_of_bands", 0)) {
                        Mat_VarReadDataAll(matfp, bandsCount);
                        bands = int(static_cast<double*>(bandsCount->data)[0]);
                        Q_ASSERT(bands == samplescount);
                    }

                    if (auto startFr = Mat_VarGetStructFieldByName(x_values, "start_frequency", 0)) {
                        Mat_VarReadDataAll(matfp, startFr);
                        startfrequency = static_cast<double*>(startFr->data)[0];
                    }
                    if (auto quantity = Mat_VarGetStructFieldByName(x_values, "quantity", 0)) {
                        Mat_VarReadDataAll(matfp, quantity);
                        if (auto xname = Mat_VarGetStructFieldByName(quantity, "label", 0)) {
                            Mat_VarReadDataAll(matfp, xname);
                            channel->dataDescription().put("xname", charToString(xname));
                        }
                    }
                    if (auto values = Mat_VarGetStructFieldByName(x_values, "values", 0)) {
                        Mat_VarReadDataAll(matfp, values);
                        xvalues.resize(values->dims[0]*values->dims[1]);
                        for (int i=0; i<xvalues.size(); ++i) xvalues[i]=static_cast<double*>(values->data)[i];
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
                            Mat_VarReadDataAll(matfp, label);
                            s = charToString(label).simplified();
                            channel->dataDescription().put("yname", s);
                        }
                        if (auto unit = Mat_VarGetStructFieldByName(quantity, "unit_transformation", 0)) {
                            if (auto logref = Mat_VarGetStructFieldByName(unit, "log_reference", 0)) {
                                Mat_VarReadDataAll(matfp, logref);
                                channel->data()->setThreshold(static_cast<double*>(logref->data)[0]);
                                channel->dataDescription().put("function.logref", static_cast<double*>(logref->data)[0]);
                            }
                        }
                        if (s.isEmpty()) {
                            if (auto q = Mat_VarGetStructFieldByName(quantity, "quantity_terms", 0)) {
                                Mat_VarReadDataAll(matfp, q);
                                //TODO: добавить чтение единицы
                                //s = getLabel(q);
                            }
                        }
                    }
                }

                DataHolder::YValuesFormat yformat = DataHolder::YValuesReals;
                QString type;
                if (auto typeS = Mat_VarGetStructFieldByName(function_record, "type", 0)) {
                    Mat_VarReadDataAll(matfp, typeS);
                    type = charToString(typeS);
                }
                QString format;
                if (auto formatS = Mat_VarGetStructFieldByName(function_record, "spectrum_format", 0)) {
                    Mat_VarReadDataAll(matfp, formatS);
                    format = charToString(formatS);
                }
                if (type=="Signal") {
                    channel->dataDescription().put("function.name", "Time Response");
                    channel->dataDescription().put("function.type", 1);
                }
                if (type=="FrequencySpectrum") {
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
                if (type=="CrossPowerSpectrum") {
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
                if (type=="Coherence") {
                    channel->dataDescription().put("function.name", "COH");
                    channel->dataDescription().put("function.type", 6);
                    channel->dataDescription().put("function.logscale", "dimensionless");
                    channel->dataDescription().put("function.logref", 1);
                }
                if (type=="PSD") {
                    yformat = DataHolder::YValuesAmplitudes;
                    channel->dataDescription().put("function.name", "PSD");
                    channel->dataDescription().put("function.type", 9);
//                    channel->dataDescription().put("function.logscale", "dimensionless");
                }
                if (type=="AutoPowerSpectrum") {
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

                //ЗАГЛУШКА
                channel->data()->setZValues(0.0, 0.0, 1);

                if (auto time = Mat_VarGetStructFieldByName(function_record, "creation_time", 0)) {
                    Mat_VarReadDataAll(matfp, time);
                    if (time->class_type == MAT_C_CELL) {
                        if (auto t = Mat_VarGetCell(time, channel->indexInGroup)) {
                            Mat_VarReadDataAll(matfp, t);
                            auto timeS = charToString(t);
                            channel->dataDescription().put("dateTime",
                                                           QDateTime::fromString(timeS, "yyyy/MM/dd hh:mm:ss"));
                        }
                    }
                    else {
                        auto timeS = charToString(time);
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
                channel->dataDescription().put("name", channel->_name);
            }
            channels << toAppend;
        }
    }

//    Mat_Close(matfp);
}

void MatlabFile::write()
{
}

void MatlabFile::deleteChannels(const QVector<int> &channelsToDelete)
{

}

void MatlabFile::copyChannelsFrom(const QVector<Channel *> &)
{
}

int MatlabFile::channelsCount() const
{
    return channels.size();
}

void MatlabFile::move(bool up, const QVector<int> &indexes, const QVector<int> &newIndexes)
{
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

void MatlabFile::fillPreliminary(const FileDescriptor *)
{
}

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

void MatlabFile::addChannelWithData(DataHolder *data, const DataDescription &description)
{
}

//qint64 MatlabFile::fileSize() const
//{
//}

//void MatlabFile::setChanged(bool changed)
//{
//}

void MatlabFile::init(const QVector<Channel *> &)
{

}

MatlabChannel::MatlabChannel(MatlabFile *parent) : Channel(), parent(parent)
{

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

QVector<cx_double> getDataFromMatVarUngroupedComplex(matvar_t *v)
{
    Q_ASSERT(v->dims[0]==1);
    Q_ASSERT(v->isComplex!=0);

    auto c = (mat_complex_split_t*)v->data;
    QVector<cx_double> result(v->dims[1]);
    switch (v->class_type) {
        case MAT_C_DOUBLE: {
            double *d1 = (double*)c->Re;
            double *d2 = (double*)c->Im;
            for (size_t i=0; i<v->dims[1]; ++i) result[i] = {d1[i], d2[i]};
            break;
        }
        case MAT_C_SINGLE: {
            float *d1 = (float*)c->Re;
            float *d2 = (float*)c->Im;
            for (size_t i=0; i<v->dims[1]; ++i) result[i] = {d1[i], d2[i]};
            break;
        }
        case MAT_C_INT8: {
            int8_t *d1 = (int8_t*)c->Re;
            int8_t *d2 = (int8_t*)c->Im;
            for (size_t i=0; i<v->dims[1]; ++i) result[i] = {double(d1[i]), double(d2[i])};
            break;
        }
        case MAT_C_UINT8: {
            uint8_t *d1 = (uint8_t*)c->Re;
            uint8_t *d2 = (uint8_t*)c->Im;
            for (size_t i=0; i<v->dims[1]; ++i) result[i] = {double(d1[i]), double(d2[i])};
            break;
        }
        case MAT_C_INT16: {
            int16_t *d1 = (int16_t*)c->Re;
            int16_t *d2 = (int16_t*)c->Im;
            for (size_t i=0; i<v->dims[1]; ++i) result[i] = {double(d1[i]), double(d2[i])};
            break;
        }
        case MAT_C_UINT16: {
            uint16_t *d1 = (uint16_t*)c->Re;
            uint16_t *d2 = (uint16_t*)c->Im;
            for (size_t i=0; i<v->dims[1]; ++i) result[i] = {double(d1[i]), double(d2[i])};
            break;
        }
        case MAT_C_INT32: {
            int32_t *d1 = (int32_t*)c->Re;
            int32_t *d2 = (int32_t*)c->Im;
            for (size_t i=0; i<v->dims[1]; ++i) result[i] = {double(d1[i]), double(d2[i])};
            break;
        }
        case MAT_C_UINT32: {
            uint32_t *d1 = (uint32_t*)c->Re;
            uint32_t *d2 = (uint32_t*)c->Im;
            for (size_t i=0; i<v->dims[1]; ++i) result[i] = {double(d1[i]), double(d2[i])};
            break;
        }
        case MAT_C_INT64: {
            int64_t *d1 = (int64_t*)c->Re;
            int64_t *d2 = (int64_t*)c->Im;
            for (size_t i=0; i<v->dims[1]; ++i) result[i] = {double(d1[i]), double(d2[i])};
            break;
        }
        case MAT_C_UINT64: {
            uint64_t *d1 = (uint64_t*)c->Re;
            uint64_t *d2 = (uint64_t*)c->Im;
            for (size_t i=0; i<v->dims[1]; ++i) result[i] = {double(d1[i]), double(d2[i])};
            break;
        }
        default: break;
    }
    return result;
}

QVector<cx_double> getDataFromMatVarGroupedComplex(matvar_t *v, int index)
{
    Q_ASSERT(v->dims[1]>1);
    Q_ASSERT(v->isComplex!=0);

    const int offset = v->dims[0]*index;
    auto c = (mat_complex_split_t*)v->data;
    QVector<cx_double> result(v->dims[0]);
    switch (v->class_type) {
        case MAT_C_DOUBLE: {
            double *d1 = (double*)c->Re;
            double *d2 = (double*)c->Im;
            for (size_t i=0; i<v->dims[0]; ++i) result[i] = {d1[offset+i], d2[offset+i]};
            break;
        }
        case MAT_C_SINGLE: {
            float *d1 = (float*)c->Re;
            float *d2 = (float*)c->Im;
            for (size_t i=0; i<v->dims[0]; ++i) result[i] = {d1[offset+i], d2[offset+i]};
            break;
        }
        case MAT_C_INT8: {
            int8_t *d1 = (int8_t*)c->Re;
            int8_t *d2 = (int8_t*)c->Im;
            for (size_t i=0; i<v->dims[0]; ++i) result[i] = {double(d1[offset+i]), double(d2[offset+i])};
            break;
        }
        case MAT_C_UINT8: {
            uint8_t *d1 = (uint8_t*)c->Re;
            uint8_t *d2 = (uint8_t*)c->Im;
            for (size_t i=0; i<v->dims[0]; ++i) result[i] = {double(d1[offset+i]), double(d2[offset+i])};
            break;
        }
        case MAT_C_INT16: {
            int16_t *d1 = (int16_t*)c->Re;
            int16_t *d2 = (int16_t*)c->Im;
            for (size_t i=0; i<v->dims[0]; ++i) result[i] = {double(d1[offset+i]), double(d2[offset+i])};
            break;
        }
        case MAT_C_UINT16: {
            uint16_t *d1 = (uint16_t*)c->Re;
            uint16_t *d2 = (uint16_t*)c->Im;
            for (size_t i=0; i<v->dims[0]; ++i) result[i] = {double(d1[offset+i]), double(d2[offset+i])};
            break;
        }
        case MAT_C_INT32: {
            int32_t *d1 = (int32_t*)c->Re;
            int32_t *d2 = (int32_t*)c->Im;
            for (size_t i=0; i<v->dims[0]; ++i) result[i] = {double(d1[offset+i]), double(d2[offset+i])};
            break;
        }
        case MAT_C_UINT32: {
            uint32_t *d1 = (uint32_t*)c->Re;
            uint32_t *d2 = (uint32_t*)c->Im;
            for (size_t i=0; i<v->dims[0]; ++i) result[i] = {double(d1[offset+i]), double(d2[offset+i])};
            break;
        }
        case MAT_C_INT64: {
            int64_t *d1 = (int64_t*)c->Re;
            int64_t *d2 = (int64_t*)c->Im;
            for (size_t i=0; i<v->dims[0]; ++i) result[i] = {double(d1[offset+i]), double(d2[offset+i])};
            break;
        }
        case MAT_C_UINT64: {
            uint64_t *d1 = (uint64_t*)c->Re;
            uint64_t *d2 = (uint64_t*)c->Im;
            for (size_t i=0; i<v->dims[0]; ++i) result[i] = {double(d1[offset+i]), double(d2[offset+i])};
            break;
        }
        default: break;
    }
    return result;
}

QVector<double> getDataFromMatVarUngrouped(matvar_t *v)
{
    Q_ASSERT(v->dims[0]==1);
    QVector<double> result(v->dims[1]);
    switch (v->class_type) {
        case MAT_C_DOUBLE: {
            double *d = (double*)v->data;
            for (size_t i=0; i<v->dims[1]; ++i) result[i] = d[i];
            break;
        }
        case MAT_C_SINGLE: {
            float *d = (float*)v->data;
            for (size_t i=0; i<v->dims[1]; ++i) result[i] = d[i];
            break;
        }
        case MAT_C_INT8: {
            int8_t *d = (int8_t*)v->data;
            for (size_t i=0; i<v->dims[1]; ++i) result[i] = d[i];
            break;
        }
        case MAT_C_UINT8: {
            uint8_t *d = (uint8_t*)v->data;
            for (size_t i=0; i<v->dims[1]; ++i) result[i] = d[i];
            break;
        }
        case MAT_C_INT16: {
            int16_t *d = (int16_t*)v->data;
            for (size_t i=0; i<v->dims[1]; ++i) result[i] = d[i];
            break;
        }
        case MAT_C_UINT16: {
            uint16_t *d = (uint16_t*)v->data;
            for (size_t i=0; i<v->dims[1]; ++i) result[i] = d[i];
            break;
        }
        case MAT_C_INT32: {
            int32_t *d = (int32_t*)v->data;
            for (size_t i=0; i<v->dims[1]; ++i) result[i] = d[i];
            break;
        }
        case MAT_C_UINT32: {
            uint32_t *d = (uint32_t*)v->data;
            for (size_t i=0; i<v->dims[1]; ++i) result[i] = d[i];
            break;
        }
        case MAT_C_INT64: {
            int64_t *d = (int64_t*)v->data;
            for (size_t i=0; i<v->dims[1]; ++i) result[i] = d[i];
            break;
        }
        case MAT_C_UINT64: {
            uint64_t *d = (uint64_t*)v->data;
            for (size_t i=0; i<v->dims[1]; ++i) result[i] = d[i];
            break;
        }
        default: break;
    }
    return result;
}

QVector<double> getDataFromMatVarGrouped(matvar_t *v, int index)
{
    Q_ASSERT(v->dims[1]>1);
    QVector<double> result(v->dims[0]);
    const int offset = v->dims[0]*index;
    switch (v->class_type) {
        case MAT_C_DOUBLE: {
            double *d = (double*)v->data;
            for (size_t i=0; i<v->dims[0]; ++i) result[i] = d[offset+i];
            break;
        }
        case MAT_C_SINGLE: {
            float *d = (float*)v->data;
            for (size_t i=0; i<v->dims[0]; ++i) result[i] = d[offset+i];
            break;
        }
        case MAT_C_INT8: {
            int8_t *d = (int8_t*)v->data;
            for (size_t i=0; i<v->dims[0]; ++i) result[i] = d[offset+i];
            break;
        }
        case MAT_C_UINT8: {
            uint8_t *d = (uint8_t*)v->data;
            for (size_t i=0; i<v->dims[0]; ++i) result[i] = d[offset+i];
            break;
        }
        case MAT_C_INT16: {
            int16_t *d = (int16_t*)v->data;
            for (size_t i=0; i<v->dims[0]; ++i) result[i] = d[offset+i];
            break;
        }
        case MAT_C_UINT16: {
            uint16_t *d = (uint16_t*)v->data;
            for (size_t i=0; i<v->dims[0]; ++i) result[i] = d[offset+i];
            break;
        }
        case MAT_C_INT32: {
            int32_t *d = (int32_t*)v->data;
            for (size_t i=0; i<v->dims[0]; ++i) result[i] = d[offset+i];
            break;
        }
        case MAT_C_UINT32: {
            uint32_t *d = (uint32_t*)v->data;
            for (size_t i=0; i<v->dims[0]; ++i) result[i] = d[offset+i];
            break;
        }
        case MAT_C_INT64: {
            int64_t *d = (int64_t*)v->data;
            for (size_t i=0; i<v->dims[0]; ++i) result[i] = d[offset+i];
            break;
        }
        case MAT_C_UINT64: {
            uint64_t *d = (uint64_t*)v->data;
            for (size_t i=0; i<v->dims[0]; ++i) result[i] = d[offset+i];
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
//    Mat_VarPrint(values, 1);
    if (values->isComplex) {
        if (grouped) {
            auto data = getDataFromMatVarGroupedComplex(values, indexInGroup);
            _data->setYValues(data);
        }
        else {
            auto data = getDataFromMatVarUngroupedComplex(values);
            _data->setYValues(data);
        }
    }
    else {
        if (grouped) {
            QVector<double> data = getDataFromMatVarGrouped(values, indexInGroup);
            _data->setYValues(data, _data->yValuesFormat());
        }
        else {
            QVector<double> data = getDataFromMatVarUngrouped(values);
            _data->setYValues(data, _data->yValuesFormat());
        }
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
