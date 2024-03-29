#include "matfile.h"

#include <QFile>
#include "logging.h"
#include <QJsonDocument>
#include <QJsonArray>

//#define PRINT_CONTENT

MatFile::MatFile(const QString &fileName) : FileDescriptor(fileName)
{DD;

}

MatFile::~MatFile()
{DD;
    qDeleteAll(records);
    qDeleteAll(channels);
}

int MatFile::getChannelsCount() const
{DD;
    int count = 0;
    foreach (MatlabRecord *rec, records) {
        if (rec) count += rec->getChannelsCount();
    }
    return count;
}

void MatFile::read()
{DD;
    QFile f(fileName());
    if (f.open(QFile::ReadOnly)) {
        QDataStream stream(&f);
        stream.setByteOrder(QDataStream::LittleEndian);
        stream.skipRawData(128); // текстовый заголовок файла, пропускаем

        while (!stream.atEnd()) {
            MatlabHeader *header = new MatlabHeader();
            header->read(&stream);

            MatlabRecord *rec = matlabRecordFactory(header, fileName()); // <- сейчас указывает на начало данных
            if (!rec) {
                LOG(ERROR)<<"Unknown matlab data type:" << header->type;
                continue;
            }

            records << rec;
            rec->readData(&stream);
        }
    }
    setDataDescription(xml.toDataDescription());

    if (records.isEmpty()) return;

    ///теперь реорганизуем данные - если каналы в файле сгруппированы,
    /// то мы уже не имеем прямого соответствия record -> channel

    for (MatlabRecord *r: records) {
        MatlabStructArray *rec = dynamic_cast<MatlabStructArray *>(r);
        if (!rec) continue;

        //проверяем, сгруппирован ли канал
        //запись function_record.name будет иметь размерность > 1
        if (MatlabStructArray *function_record = findSubrecord<MatlabStructArray *>("function_record", rec)) {

            QList<MatChannel *> toAppend;

            MatlabCellArray *name = findSubrecord<MatlabCellArray*>("name", function_record);
            if (name) {//сгруппированные данные
                int count = name->dimensions.value(1);
                for (int i=0; i<count; ++i) {
                    MatChannel *channel = new MatChannel(this);
                    channel->grouped = true;
                    channel->indexInGroup = i;
                    channel->groupSize = count;
                    channel->_name = name->subRecords[i]->getString().section(" ", 0, 0);
                    //channel->_type =

                    toAppend << channel;
                }
            }
            else {//несгруппированные данные, record -> channel
                MatChannel *channel = new MatChannel(this);
                channel->_name = rec->name.section("_", 0, 0);
                toAppend << channel;
            }

            for (int i=0; i<toAppend.size(); ++i) {
                MatChannel *channel = toAppend.at(i);

                channel->_type = function_record->subRecords[function_record->fieldNames.indexOf("type")]->getString();
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
                QString bandtype;
                QVector<double> xvalues;
                double startfrequency = 0.0;

                if (MatlabStructArray *x_values = findSubrecord<MatlabStructArray *>("x_values", rec)) {
                    if (auto *startValue = findSubrecord<MatlabNumericArray*>("start_value", x_values))
                        xbegin = startValue->getNumericAsDouble().constFirst();
                    if (auto *increment = findSubrecord<MatlabNumericArray*>("increment", x_values))
                        xstep = increment->getNumericAsDouble().constFirst();
                    if (auto *numberOfValues = findSubrecord<MatlabNumericArray*>("number_of_values", x_values))
                        samplescount = numberOfValues->getNumericAsInt().constFirst();
                    if (auto *bandType = findSubrecord<MatlabCharacterArray*>("band_type",x_values))
                        bandtype = bandType->getString();
                    if (auto *startFr = findSubrecord<MatlabNumericArray*>("start_frequency", x_values))
                        startfrequency = startFr->getNumericAsDouble().constFirst();
                    if (auto *quantity = findSubrecord<MatlabStructArray*>("quantity", x_values))
                        channel->dataDescription().put("xname", quantity->subRecords[quantity->fieldNames.indexOf("label")]->getString());
                    if (MatlabNumericArray* values = findSubrecord<MatlabNumericArray*>("values", x_values))
                        xvalues = values->getNumericAsDouble();
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
                if (c.expression.startsWith("OCTF1(")) octave = 1;
                else if (c.expression.startsWith("OCTF3(")) octave = 3;
                else if (c.expression.startsWith("OCTF6(")) octave = 6;
                else if (c.expression.startsWith("OCTF2(")) octave = 2;
                else if (c.expression.startsWith("OCTF12(")) octave = 12;
                else if (c.expression.startsWith("OCTF24(")) octave = 24;
                channel->dataDescription().put("function.octaveFormat", octave);

                if (xvalues.isEmpty()) {
                    channel->data()->setXValues(xbegin, xstep, samplescount);
                    if (!qFuzzyIsNull(xstep) && channel->type()==Descriptor::TimeResponse)
                        channel->dataDescription().put("samplerate", 1.0/xstep);
                }
                else {
                    channel->data()->setXValues(xvalues);
                }

                //y_values
                if (MatlabStructArray *y_values = findSubrecord<MatlabStructArray *>("y_values", rec)) {
                    if (MatlabNumericArray *values = findSubrecord<MatlabNumericArray *>("values", y_values)) {
                        channel->real_values = dynamic_cast<MatlabNumericRecord *>(values->realValues);
                        channel->imag_values = dynamic_cast<MatlabNumericRecord *>(values->imagValues);
                        channel->complex = values->complex;
                        channel->dataDescription().put("function.precision", "double");
                    }
                }

                DataHolder::YValuesFormat yformat = DataHolder::YValuesReals;
                if (channel->complex) yformat = DataHolder::YValuesComplex;
                else if (c.expression.startsWith("FFT(") ||
                         c.expression.startsWith("GXY(") ||
                         c.expression.startsWith("GXYN(")||
                         c.expression.startsWith("FRF(")) {
                    if (c.fftDataType == 1) yformat = DataHolder::YValuesAmplitudes;
                    else if (c.fftDataType == 2) yformat = DataHolder::YValuesPhases;
                }
                else if (c.expression.startsWith("PSD(") ||
                         c.expression.startsWith("ESD(") ||
                         c.expression.startsWith("APS(") ||
                         c.expression.startsWith("OCT"))
                    yformat = DataHolder::YValuesAmplitudes;
                channel->data()->setYValuesFormat(yformat);
                switch (yformat) {
                    case DataHolder::YValuesAmplitudes: channel->dataDescription().put("function.format", "amplitude"); break;
                    case DataHolder::YValuesPhases: channel->dataDescription().put("function.format", "phase"); break;
                    case DataHolder::YValuesReals: channel->dataDescription().put("function.format", "real"); break;
                    case DataHolder::YValuesComplex: channel->dataDescription().put("function.format", "complex"); break;
                    default: channel->dataDescription().put("function.format", "real");
                }

                channel->data()->setThreshold(c.logRef);
                channel->dataDescription().put("function.logref", c.logRef);

                auto units = DataHolder::UnitsUnknown;
                if (c.scale == 10) units = DataHolder::UnitsQuadratic;
                else if (c.scale == 20) units = DataHolder::UnitsLinear;
                channel->data()->setYValuesUnits(units);
                channel->dataDescription().put("function.logscale", c.scale == 10?"quadratic":"linear");

                //ЗАГЛУШКА
                channel->data()->setZValues(0.0, 0.0, 1);

                channel->dataDescription().put("yname", channel->xml.units);
                QString ChanAddress = QString("SCADAS\\")+channel->xml.catLabel;
                QStringList info = channel->xml.info;
                info.append(ChanAddress);
                channel->dataDescription().put("description",info.join(" \\"));

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
                    QString format = function_record->subRecords[function_record->fieldNames.indexOf("spectrum_format")]->getString();
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

                QStringList l;
                l<<channel->xml.generalName<<channel->xml.pointId<<channel->xml.direction;
                channel->dataDescription().put("name",l.join("-"));
            }
            channels << toAppend;
        }
    }
}

QStringList MatFile::fileFilters()
{
    return QStringList()<< "Файлы Matlab (*.mat)";
}

QStringList MatFile::suffixes()
{
    return QStringList()<<"*.mat";
}

QList<QVector<int> > MatFile::groupChannels() const
{
    QList<QVector<int>> groupedChannelsIndexes;
    //сортируем каналы по типу и по размеру

    QMap<QPair<int, Descriptor::DataType>, int> map;

    for (int i=0; i<channelsCount(); ++i) {
        int size = channels.at(i)->data()->samplesCount();
        Descriptor::DataType type = channels.at(i)->type();
        QPair<int, Descriptor::DataType> pair = qMakePair(size, type);
        if (!map.contains(pair)) {
            //встретили первый раз
            QVector<int> indexes;
            indexes << i;
            groupedChannelsIndexes << indexes;
            map.insert(pair, groupedChannelsIndexes.size()-1);
        }
        else {
            //уже был
            const int pos = map.value(pair);
            QVector<int> indexes = groupedChannelsIndexes.at(pos);
            indexes.append(i);
            groupedChannelsIndexes.replace(pos, indexes);
        }
    }

    return groupedChannelsIndexes;
}

QByteArray MatFile::toJson() const
{
    QJsonDocument doc;

    QJsonArray array;
    for (MatlabRecord *r: records) {
        QJsonValue a = r->toJson();
        array.append(a);
    }
    doc.setArray(array);

    return doc.toJson();
}




MatlabRecord *matlabRecordFactory(MatlabHeader *header, const QString &fileName)
{DD;
    MatlabRecord *rec = 0;
    switch (header->type) {
        case 0: return rec; break;
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 0xC://12 miINT64
        case 0xD://13 miUINT64
        case 7:
        case 9: rec = new MatlabNumericRecord(header); break;
        case 0x10: rec = new MatlabUtf8Record(header); break;//16
        case 0x11: rec = new MatlabUtf16Record(header); break;//17
        case 0xE: {//14 miMATRIX
            switch (header->arrayClass) {
                case 1: rec = new MatlabCellArray(header); break;
                case 2: rec = new MatlabStructArray(header); break;
                case 3: rec = new MatlabObjectArray(header); break;
                case 4: rec = new MatlabCharacterArray(header); break;
                case 5: rec = new MatlabSparseArray(header); break;
                case 6:
                case 7:
                case 8:
                case 9:
                case 10:
                case 11:
                case 12:
                case 13:
                case 14:
                case 15: rec = new MatlabNumericArray(header); break;
                default: return 0;
            }
            break;
        }
        case 0xF: {//15 miCOMPRESSED
//            LOG(DEBUG)<<"zlib data";
            return rec;
            break;
        }
        default: return 0;
    }
    if (rec) rec->fileName = fileName;
    return rec;
}

void MatlabNumericRecord::readData(QDataStream *f)
{DD;
//    f->setFloatingPointPrecision(QDataStream::DoublePrecision);

    switch(header->type) {
        case 1:
        case 2: actualDataSize = header->sizeInBytesWithoutPadding; /*qint8, quint8*/
            break;
        case 3:
        case 4: actualDataSize = header->sizeInBytesWithoutPadding / 2; /*qint16, quint16*/
            break;
        case 5:
        case 6:
        case 7: actualDataSize = header->sizeInBytesWithoutPadding / 4; /*qint32, quint32, float*/
            break;
        case 9:
        case 12:
        case 13: actualDataSize = header->sizeInBytesWithoutPadding / 8; /*qint64, quint64, double*/
            break;
        default: actualDataSize = header->sizeInBytesWithoutPadding; /*unknown type, treat as char*/
            break;
    }

    f->skipRawData(header->actualSize/* - header->sizeInBytesWithoutPadding*/);
}

QVector<double> MatlabNumericRecord::getNumericAsDouble() const
{DD;
    QVector<double> data = this->getNumeric<double>();

    return data;
}

QVector<qint64> MatlabNumericRecord::getNumericAsInt() const
{DD;
    QVector<qint64> data = this->getNumeric<qint64>();

    return data;
}

QJsonValue MatlabNumericRecord::toJson() const
{
    QJsonObject o = MatlabRecord::toJson().toObject();
    o.insert("samples", qint64(actualDataSize));
    if (actualDataSize==1) o.insert("value", getNumericAsDouble().constFirst());
    return o;
}

template<typename T>
QVector<T> MatlabNumericRecord::getNumeric() const
{DD;
    QVector<T> data;

    QFile f(fileName);
    if (!f.open(QFile::ReadOnly))  return data;

    QDataStream stream(&f);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream.device()->seek(header->dataBegin);

    switch(header->type) {
        case 1: data = readBlock<qint8, T>(&stream, actualDataSize); break;
        case 2: data = readBlock<quint8, T>(&stream, actualDataSize); break;
        case 3: data = readBlock<qint16, T>(&stream, actualDataSize); break;
        case 4: data = readBlock<quint16, T>(&stream, actualDataSize); break;
        case 5: data = readBlock<qint32, T>(&stream, actualDataSize); break;
        case 6: data = readBlock<quint32, T>(&stream, actualDataSize); break;
        case 7: stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
            data = readBlock<float, T>(&stream, actualDataSize); break;
        case 9: stream.setFloatingPointPrecision(QDataStream::DoublePrecision);
            data = readBlock<double, T>(&stream, actualDataSize); break;
        case 12: data = readBlock<qint64, T>(&stream, actualDataSize); break;
        case 13: data = readBlock<quint64, T>(&stream, actualDataSize); break;
        default: break;
    }

    return data;
}

MatlabArrayRecord::~MatlabArrayRecord()
{DD;
    qDeleteAll(subRecords);

}

QJsonValue MatlabArrayRecord::toJson() const
{
    QJsonObject o = MatlabRecord::toJson().toObject();

    if (!name.isEmpty())
        o.insert("name", name);
    QStringList l;
    for(int i: dimensions) l << QString::number(i);
    if (dimensions.constFirst() > 1 || dimensions.at(1) > 1)
        o.insert("dimensions", l.join(" x "));

    QJsonArray array;
    for (MatlabRecord *r: subRecords)
        array << r->toJson();
    if (!array.isEmpty())
        o.insert("subrecords", array);

    return o;
}

void MatlabArrayRecord::readData(QDataStream *f)
{DD;
    uint alreadyRead = 16;

    if (header->flags & 0b00001000) complex = true;
    if (header->flags & 0b00000100) global = true;
    if (header->flags & 0b00000010) logical = true;

    alreadyRead = 16; // flags

    // reading dimensions
    quint32 matrixDimensionsType;
    *f >> matrixDimensionsType; // 4 bytes;
    Q_ASSERT_X(matrixDimensionsType == 5, "MatlabArrayRecord::readData",
               "Unknown matlab matrix subelement instead of Dimensions");

    quint32 matrixDimensionsSize;
    *f >> matrixDimensionsSize; // 4 bytes;

    alreadyRead += 8;

    for (uint i=0; i< matrixDimensionsSize/sizeof(qint32); ++i) {
        qint32 dim;
        *f >> dim;
        dimensions << dim;
        alreadyRead += 4;
    } // 4 bytes * dimensions.size
    int padding = matrixDimensionsSize % 8;
    if (padding > 0) {
        f->skipRawData(8 - padding);
        alreadyRead += 8 - padding;
    }

    //reading name
    quint32 matrixNameType;
    *f >> matrixNameType; // 4 bytes;
    alreadyRead += 4;
    if (matrixNameType > 1) {
        // compressed name
        name = QString::fromLocal8Bit(f->device()->read(4)); // 4 bytes
        alreadyRead += 4;
    }
    else {
        quint32 matrixNameSize;
        *f >> matrixNameSize; // 4 bytes
        if (matrixNameSize > 0)
            name = QString::fromLocal8Bit(f->device()->read(matrixNameSize));

        int padding = matrixNameSize % 8;
        if (padding > 0)
            f->skipRawData(8 - padding);

        alreadyRead += (matrixNameSize + padding);
    } // matrixNameSize bytes + padding

    QStringList dimsL; foreach(int i, dimensions) dimsL << QString::number(i);
#ifdef PRINT_CONTENT
    LOG(DEBUG)<<"Array name:"<<name<<"Array dimensions:"<<dimsL.join("x");
#endif
}

void MatlabHeader::read(QDataStream *f)
{DD;
    headerBegin = f->device()->pos();

    *f >> type;

    if (type > 0xffff) {//compressed
        sizeInBytesWithoutPadding = (type >> 16);
        type = (type % 65536);
        actualSize = 4;
        smallData = true;
    }
    else {
        *f >> sizeInBytesWithoutPadding;
        actualSize = sizeInBytesWithoutPadding;
        if (sizeInBytesWithoutPadding % 8 != 0) actualSize += 8-(sizeInBytesWithoutPadding%8);
    }

    dataBegin = f->device()->pos();

    // peek into array flags to determine array class
    if (type == MatlabRecord::miMATRIX) {
        //reading flags and type
        quint32 matrixFlagsType; // 4 bytes
        quint32 matrixFlagsSize; // 4 bytes
        *f >> matrixFlagsType >> matrixFlagsSize;
        Q_ASSERT_X(matrixFlagsType == 6 && matrixFlagsSize == 8, "MatlabArrayRecord::readData",
                   "Unknown matlab matrix subelement instead of Flags");


        *f >> arrayClass >> flags; // 2 bytes

        f->skipRawData(6);

#ifdef PRINT_CONTENT
        LOG(DEBUG)<<"Array class ="<<arrayClass<<"flags="<<flags<< "Block size:"<<sizeInBytesWithoutPadding
               << "actually"<<actualSize
               <<"header starts with:" <<hex<<headerBegin<<dec
               <<"data starts with:" <<hex<<dataBegin<<dec
              << "compressed:"<<smallData;
#endif

    }
    else {
#ifdef PRINT_CONTENT
        LOG(DEBUG)<<"Matlab Data Type=" <<type
               << "Block size:"<<sizeInBytesWithoutPadding
               << "actually"<<actualSize
               <<"header starts with:" <<hex<<headerBegin<<dec
               <<"data starts with:" <<hex<<dataBegin<<dec
              << "compressed:"<<smallData;
#endif

    }
}

void MatlabCellArray::readData(QDataStream *f)
{DD;
    MatlabArrayRecord::readData(f);

    // читаем поля - это могут быть только miMatrix
    // это может быть также массив, поэтому вектор будет длиной dims1*dims2*dims3
    int totalSize = 1;
    foreach (int dim, dimensions) totalSize *= dim;

    for (int cellNumber = 0; cellNumber < totalSize; ++cellNumber) {
#ifdef PRINT_CONTENT
    LOG(DEBUG)<<"reading cell"<<cellNumber+1<<"of"<<totalSize;
#endif

        MatlabHeader *header = new MatlabHeader();
        header->read(f);

        MatlabRecord *rec = matlabRecordFactory(header, fileName); // <- сейчас указывает на начало данных
        if (!rec) {
            LOG(ERROR)<<"Unknown matlab data type:" << header->type;
            return;
        }
        rec->readData(f);

        subRecords << rec;
#ifdef PRINT_CONTENT
    LOG(DEBUG)<<"after reading cell" <<cellNumber+1<< "pos at"<<hex<<f->device()->pos()<<dec;
#endif
    }
}

QJsonValue MatlabCellArray::toJson() const
{
    QJsonObject o = MatlabArrayRecord::toJson().toObject();



    return o;
}

void MatlabStructArray::readData(QDataStream *f)
{DD;
    MatlabArrayRecord::readData(f);

    // dims, name были прочитаны ранее

    // field name length
    quint16 fieldNameType;
    quint16 fieldNameLength;
    *f >> fieldNameType >> fieldNameLength; // 2 bytes
    Q_ASSERT_X(fieldNameType == 5 && fieldNameLength == 4, "MatlabArrayRecord::readData",
               "Unknown matlab struct subelement instead of field name length");
    qint32 maxLength;
    *f >> maxLength;

    //field names
    quint32 fieldNamesType;
    quint32 fieldNamesLength;
    *f >> fieldNamesType >> fieldNamesLength; // 8 bytes
    Q_ASSERT_X(fieldNamesType == 1, "MatlabArrayRecord::readData",
               "Unknown matlab struct subelement instead of field names");
    if (fieldNamesLength > 0) {
        for (uint i=0; i<fieldNamesLength/maxLength; ++i)
            fieldNames << QString::fromLocal8Bit(f->device()->read(maxLength));
    }
    int padding = fieldNamesLength % 8;
    if (padding > 0)
        f->skipRawData(8 - padding);
#ifdef PRINT_CONTENT
    LOG(DEBUG)<<"  fields names"<<fieldNames;
#endif
//

    // читаем поля структуры - это могут быть любые типы
    // это может быть также массив структур, поэтому вектор будет длиной dims1*dims2*dims3
    int totalSize = 1;
    foreach (int dim, dimensions) totalSize *= dim;

    for (int structNumber = 0; structNumber < totalSize; ++structNumber) {
#ifdef PRINT_CONTENT
    LOG(DEBUG)<<"reading struct"<<structNumber+1<<"of"<<totalSize;
#endif
//
        for (int fieldNumber = 0; fieldNumber < fieldNames.size(); ++fieldNumber) {
#ifdef PRINT_CONTENT
    LOG(DEBUG)<<"reading field"<<fieldNames.at(fieldNumber)<<"of"<<fieldNames.size();
#endif
//

            MatlabHeader *header = new MatlabHeader();
            header->read(f);

            MatlabRecord *rec = matlabRecordFactory(header, fileName); // <- сейчас указывает на начало данных
            if (!rec) {
                LOG(ERROR)<<"Unknown matlab data type:" << header->type;
                return;
            }
            rec->readData(f);

            subRecords << rec;
#ifdef PRINT_CONTENT
    LOG(DEBUG)<<"end reading struct"<<fieldNames.at(fieldNumber)<<structNumber+1<<"of"<<totalSize <<"at"<<hex<<f->device()->pos()<<dec;
#endif
//
        }
    }

}

QJsonValue MatlabStructArray::toJson() const
{
    QJsonObject o = MatlabArrayRecord::toJson().toObject();

    QJsonArray array;
    for (int i=0; i<dimensions.at(1); ++i) {
        QJsonObject val;
        for (int j=0; j<fieldNames.size(); ++j) {
            val.insert(fieldNames.at(j), subRecords.at(i*fieldNames.size() + j)->toJson());
        }
        array << val;
    }

    if (!array.isEmpty()) {
        if (dimensions.at(1) == 1)
            o.insert("subrecords", array.first());
        else
            o.insert("subrecords", array);
    }
    return o;
}

void MatlabObjectArray::readData(QDataStream *f)
{DD;
    MatlabArrayRecord::readData(f);

    //reading class name
    quint32 classNameType;
    *f >> classNameType; // 4 bytes;
    if (classNameType > 1) {
        // compressed name
        className = QString::fromLocal8Bit(f->device()->read(4)); // 4 bytes
    }
    else {
        quint32 classNameSize;
        *f >> classNameSize; // 4 bytes
        if (classNameSize > 0)
            className = QString::fromLocal8Bit(f->device()->read(classNameSize));

        int padding = classNameSize % 8;
        if (padding > 0)
            f->skipRawData(8 - padding);
    }

    // reading field name length
    quint16 fieldNameType;
    quint16 fieldNameLength;
    *f >> fieldNameType >> fieldNameLength; // 2 bytes
    Q_ASSERT_X(fieldNameType == 5 && fieldNameLength == 4, "MatlabArrayRecord::readData",
               "Unknown matlab struct subelement instead of field name length");
    qint32 maxLength;
    *f >> maxLength;

    // reading field names
    quint32 fieldNamesType;
    quint32 fieldNamesLength;
    *f >> fieldNamesType >> fieldNamesLength; // 8 bytes
    Q_ASSERT_X(fieldNamesType == 1, "MatlabArrayRecord::readData",
               "Unknown matlab struct subelement instead of field names");
    if (fieldNamesLength > 0) {
        for (uint i=0; i<fieldNamesLength/maxLength; ++i)
            fieldNames << QString::fromLocal8Bit(f->device()->read(maxLength));
    }
    int padding = fieldNamesLength % 8;
    if (padding > 0)
        f->skipRawData(8 - padding);
#ifdef PRINT_CONTENT
    LOG(DEBUG)<<"  object fields names"<<fieldNames;
#endif
//


    //readnig fields
    int totalSize = 1;
    foreach (int dim, dimensions) totalSize *= dim;
    for (int fieldNumber = 0; fieldNumber < fieldNames.size(); ++fieldNumber) {
#ifdef PRINT_CONTENT
    LOG(DEBUG)<<"reading field"<<fieldNames.at(fieldNumber)<<"of"<<fieldNames.size();
#endif
//
        for (int cellNumber = 0; cellNumber < totalSize; ++cellNumber) {
#ifdef PRINT_CONTENT
    LOG(DEBUG)<<"reading cell"<<cellNumber+1<<"of"<<totalSize;
#endif
//
            MatlabHeader *header = new MatlabHeader();
            header->read(f);

            MatlabRecord *rec = matlabRecordFactory(header, fileName); // <- сейчас указывает на начало данных
            if (!rec) {
                LOG(ERROR)<<"Unknown matlab data type:" << header->type;
                return;
            }
            rec->readData(f);

            subRecords << rec;
#ifdef PRINT_CONTENT
    LOG(DEBUG)<<"after reading pos at"<<hex<<f->device()->pos()<<dec;
#endif
//
        }
    }
}

QJsonValue MatlabObjectArray::toJson() const
{
    QJsonObject o = MatlabArrayRecord::toJson().toObject();



    return o;
}

void MatlabCharacterArray::readData(QDataStream *f)
{DD;
    MatlabArrayRecord::readData(f);

    MatlabHeader *h = new MatlabHeader;
    h->read(f);
    MatlabRecord *rec = matlabRecordFactory(h, fileName);
    if (rec) {
        rec->readData(f);
        values << rec->getString();
        delete rec;
    }
#ifdef PRINT_CONTENT
    LOG(DEBUG)<<"values"<<values;
#endif
    //
}

QJsonValue MatlabCharacterArray::toJson() const
{
    return values.join(", ");
}

QString MatlabCharacterArray::getString() const
{DD;
    return values.join("");
}

void MatlabSparseArray::readData(QDataStream *f)
{DD;
    MatlabArrayRecord::readData(f);
}

QJsonValue MatlabSparseArray::toJson() const
{
    QJsonObject o = MatlabArrayRecord::toJson().toObject();



    return o;
}

QJsonValue MatlabNumericArray::toJson() const
{
    if (dimensions.constFirst()==1 && dimensions.at(1) == 1) {
        //единственное число
        return getNumericAsDouble().constFirst();
    }
    QJsonObject o = MatlabArrayRecord::toJson().toObject();

    return o;
}

void MatlabNumericArray::readData(QDataStream *f)
{DD;
    MatlabArrayRecord::readData(f);

    MatlabHeader *h1 = new MatlabHeader;
    h1->read(f);
    realValues = matlabRecordFactory(h1, fileName);
    if (realValues) realValues->readData(f);

    if (complex) {
        MatlabHeader *h2 = new MatlabHeader;
        h2->read(f);
        imagValues = matlabRecordFactory(h2, fileName);
        if (imagValues) imagValues->readData(f);
    }
#ifdef PRINT_CONTENT
//    if (realValues->header->==1) LOG(DEBUG)<<"Value:"<<realValues.constFirst();
//    else
//        LOG(DEBUG)<<"    values size"<<realValues.size()<<"imags size"<<valuesImag.size();
#endif

}

//template<typename T>
//void MatlabNumericArray<T>::readElement(QDataStream *f, QVector<T> &val)
//{
//    int total = 1;
//    foreach(int dim, dimensions) total *= dim;

//    quint32 type;
//    quint32 size;
//    *f >> type;
//    if (type > 0xFFFF) {//compressed element
//        size = type / 0x10000;
//        type = type % 0x10000;
//        switch(type) {
//            case 1: val = readBlock<qint8, T>(f, total,0); break;
//            case 2: val = readBlock<quint8, T>(f, total,0); break;
//            case 3: val = readBlock<qint16, T>(f, total,0); break;
//            case 4: val = readBlock<quint16, T>(f, total,0); break;
//            case 5: val = readBlock<qint32, T>(f, total,0); break;
//            case 6: val = readBlock<quint32, T>(f, total,0); break;
//            case 7: f->setFloatingPointPrecision(QDataStream::SinglePrecision);
//                val = readBlock<float, T>(f, total,0); break;
//            case 9: f->setFloatingPointPrecision(QDataStream::DoublePrecision);
//                val = readBlock<double, T>(f, total,0); break;
//            case 12: val = readBlock<qint64, T>(f, total,0); break;
//            case 13: val = readBlock<quint64, T>(f, total,0); break;
//            default: f->device()->skip(size);
//        }
//        int padding = size % 4;
//        if (padding > 0) f->device()->skip(4 - padding);
//    }
//    else {
//        *f >> size;
//        switch(type) {
//            case 1: val = readBlock<qint8, T>(f, total,0); break;
//            case 2: val = readBlock<quint8, T>(f, total,0); break;
//                case 3: val = readBlock<qint16, T>(f, total,0); break;
//                case 4: val = readBlock<quint16, T>(f, total,0); break;
//            case 5: val = readBlock<qint32, T>(f, total,0); break;
//            case 6: val = readBlock<quint32, T>(f, total,0); break;
//            case 7: f->setFloatingPointPrecision(QDataStream::SinglePrecision);
//                val = readBlock<float, T>(f, total,0); break;
//            case 9: f->setFloatingPointPrecision(QDataStream::DoublePrecision);
//                val = readBlock<double, T>(f, total,0); break;
//            case 12: val = readBlock<qint64, T>(f, total,0); break;
//            case 13: val = readBlock<quint64, T>(f, total,0); break;
//            default: f->device()->skip(size);
//        }
//        int padding = size % 8;
//        if (padding > 0) f->device()->skip(8 - padding);
//    }
//}

void MatlabUtf8Record::readData(QDataStream *f)
{DD;
    if (header->smallData) {
        data = QString::fromUtf8(f->device()->read(4));
    }
    else {
        data = QString::fromUtf8(f->device()->read(header->sizeInBytesWithoutPadding));
        f->device()->skip(header->actualSize - header->sizeInBytesWithoutPadding);
    }
}

QJsonValue MatlabUtf8Record::toJson() const
{
    QJsonObject o = MatlabRecord::toJson().toObject();
    o.insert("value", data);
    return o;
}

void MatlabUtf16Record::readData(QDataStream *f)
{DD;
#ifdef PRINT_CONTENT
    LOG(DEBUG)<<hex<<f->device()->pos()<<dec;
#endif
//
    if (header->smallData) {
        data = QString::fromUtf16((ushort*)(f->device()->read(4).data()));
    }
    else {
        data = QString::fromUtf16((ushort*)(f->device()->read(header->sizeInBytesWithoutPadding).data()));
        f->device()->skip(header->actualSize - header->sizeInBytesWithoutPadding);
    }
}

QJsonValue MatlabUtf16Record::toJson() const
{
    QJsonObject o = MatlabRecord::toJson().toObject();
    o.insert("value", data);
    return o;
}


QVector<double> MatlabNumericArray::getNumericAsDouble() const
{DD;
    QVector<double> data;
    if (!realValues) return data;

    if (MatlabNumericRecord *rec = dynamic_cast<MatlabNumericRecord*>(realValues)) {
        return rec->getNumericAsDouble();
    }
    return data;
}

QVector<qint64> MatlabNumericArray::getNumericAsInt() const
{DD;
    QVector<qint64> data;
    if (!realValues) return data;

    if (MatlabNumericRecord *rec = dynamic_cast<MatlabNumericRecord*>(realValues)) {
        return rec->getNumericAsInt();
    }
    return data;
}

QByteArray MatlabRecord::getRaw(qint64 position, qint64 length) const
{DD;
    QFile f(fileName);
    if (!f.open(QFile::ReadOnly))  return QByteArray();

    //QDataStream stream(&f);
    //stream.setByteOrder(QDataStream::LittleEndian);
    f.seek(header->dataBegin + position);
    return f.read(length /*header->sizeInBytesWithoutPadding*/);
}

QString matlabDataTypeToString(int type)
{
    switch (type) {
        case 0: return "miUndefined"; break;
        case 1: return "miINT8"; break;
        case 2: return "miUINT8"; break;
        case 3: return "miINT16"; break; //
        case 4: return "miUINT16"; break; // = 4,// 16-bit, unsigned
        case 5: return "miINT32"; break; // = 5,// 32-bit, signed
        case 6: return "miUINT32"; break; // = 6,// 32-bit, unsigned
        case 7: return "miSINGLE"; break; // = 7,// IEEE® 754 single format
        case 9: return "miDOUBLE"; break; // = 9,// IEEE 754 double format
        case 0x0C: return "miINT64"; break; // = ,// 64-bit, signed
        case 0x0D: return "miUINT64"; break; // = ,// 64-bit, unsigned
        case 0x0E: return "miMATRIX"; break; // = ,//MATLAB array
        case 0x0F: return "miCOMPRESSED"; break; // = ,//Compressed Data
        case 0x10: return "miUTF8"; break; // = ,// Unicode UTF-8 Encoded Character Data
        case 0x11: return "miUTF16"; break; // = ,// Unicode UTF-16 Encoded Character Data
        case 0x12: return "miUTF32"; break; //2 = // Unicode UTF-32 Encoded Character Data
    }
    return "";
}

QString matlabArrayClassToString(int val)
{
    switch (val) {
            case 1: return "Cell Array"; break;
            case 2: return "Struct Array"; break;
            case 3: return "Object Array"; break;
            case 4: return "Character Array"; break;
            case 5: return "Sparse Array"; break;
            case 6:
            case 7:
            case 8:
            case 9:
            case 10:
            case 11:
            case 12:
            case 13:
            case 14:
            case 15: return "NumericArray"; break;
    }
    return "";
}

QJsonValue MatlabRecord::toJson() const
{
    QJsonObject val;
    val.insert("type", matlabArrayClassToString(header->arrayClass)
               +", "+matlabDataTypeToString(header->type));
    //val.insert("sizeInBytesWithoutPadding", qint64(header->sizeInBytesWithoutPadding));
    //val.insert("headerBegin", qint64(header->headerBegin));
    //val.insert("dataBegin", qint64(header->dataBegin));
    //val.insert("actualSize", qint64(header->actualSize));
    //val.insert("smallData", header->smallData);
    QStringList fl;
    if (header->flags & 0b00001000) fl << "complex";
    if (header->flags & 0b00000100) fl << "global";
    if (header->flags & 0b00000010) fl << "logical";
    if (!fl.isEmpty())
        val.insert("flags", fl.join(", "));

    return val;
}

//template<typename T>
//QVector<T> getNumeric(const MatlabNumericRecord *rec)
//{
//    QVector<T> data;

//    QFile f(rec->fileName);
//    if (!f.open(QFile::ReadOnly))  return data;

//    QDataStream stream(&f);
//    stream.setByteOrder(QDataStream::LittleEndian);
//    stream.device()->seek(rec->header->dataBegin);

//    switch(rec->header->type) {
//        case 1: data = readBlock<qint8, T>(&stream, rec->actualDataSize); break;
//        case 2: data = readBlock<quint8, T>(&stream, rec->actualDataSize); break;
//        case 3: data = readBlock<qint16, T>(&stream, rec->actualDataSize); break;
//        case 4: data = readBlock<quint16, T>(&stream, rec->actualDataSize); break;
//        case 5: data = readBlock<qint32, T>(&stream, rec->actualDataSize); break;
//        case 6: data = readBlock<quint32, T>(&stream, rec->actualDataSize); break;
//        case 7: stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
//            data = readBlock<float, T>(&stream, rec->actualDataSize); break;
//        case 9: stream.setFloatingPointPrecision(QDataStream::DoublePrecision);
//            data = readBlock<double, T>(&stream, rec->actualDataSize); break;
//        case 12: data = readBlock<qint64, T>(&stream, rec->actualDataSize); break;
//        case 13: data = readBlock<quint64, T>(&stream, rec->actualDataSize); break;
//        default: break;
//    }

//    return data;
//}


QVector<float> getNumeric(const MatlabNumericRecord *rec)
{DD;
    QVector<float> data;

    QFile f(rec->fileName);
    if (!f.open(QFile::ReadOnly))  return data;

    QDataStream stream(&f);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream.device()->seek(rec->header->dataBegin);

    switch(rec->header->type) {
        case 1: data = readBlock<qint8, float>(&stream, rec->actualDataSize); break;
        case 2: data = readBlock<quint8, float>(&stream, rec->actualDataSize); break;
        case 3: data = readBlock<qint16, float>(&stream, rec->actualDataSize); break;
        case 4: data = readBlock<quint16, float>(&stream, rec->actualDataSize); break;
        case 5: data = readBlock<qint32, float>(&stream, rec->actualDataSize); break;
        case 6: data = readBlock<quint32, float>(&stream, rec->actualDataSize); break;
        case 7: stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
            data = readBlock<float, float>(&stream, rec->actualDataSize); break;
        case 9: stream.setFloatingPointPrecision(QDataStream::DoublePrecision);
            data = readBlock<double, float>(&stream, rec->actualDataSize); break;
        case 12: data = readBlock<qint64, float>(&stream, rec->actualDataSize); break;
        case 13: data = readBlock<quint64, float>(&stream, rec->actualDataSize); break;
        default: break;
    }

    return data;
}


void MatFile::write()
{
}

//DescriptionList MatFile::dataDescriptor() const
//{
//    return DescriptionList()<<DescriptionEntry("Заголовок 1", xml.titles.at(0))
//                           << DescriptionEntry("Заголовок 2", xml.titles.at(1))
//                           << DescriptionEntry("Заголовок 3", xml.titles.at(2));
//}

int MatFile::channelsCount() const
{
    return channels.size();
}

Channel *MatFile::channel(int index) const
{
    if (index < 0 || index >= channels.size()) return 0;
    return channels[index];
}


MatChannel::MatChannel(MatFile *parent) : Channel(), parent(parent)
{

}

Descriptor::DataType MatChannel::type() const
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

void MatChannel::populate()
{DD;
    _data->clear();

    setPopulated(false);

    qint64 idx = real_values->header->sizeInBytesWithoutPadding;
    idx /= groupSize;
    QByteArray raw = real_values->getRaw(idx * indexInGroup, idx);
    LOG(DEBUG)<<"Read"<<raw.length()<<"bytes in channel";

    QDataStream stream(&raw, QIODevice::ReadOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    //stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
    QVector<double> data = readNumeric<double>(&stream, real_values->actualDataSize / groupSize,
                                              real_values->header->type);
    LOG(DEBUG)<<"Converted to"<<data.size()<<"double values";

    if (!complex) {
        _data->setYValues(data, _data->yValuesFormat());
    }
    else {
        QVector<cx_double> valuesComplex = QVector<cx_double>(_data->samplesCount(), cx_double());

        int idx = real_values->header->sizeInBytesWithoutPadding / groupSize;
        QByteArray img = imag_values->getRaw(idx * indexInGroup, idx);

        QDataStream stream(&img, QIODevice::ReadOnly);
        stream.setByteOrder(QDataStream::LittleEndian);
        //stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
        QVector<double> imgData = readNumeric<double>(&stream, imag_values->actualDataSize / groupSize,
                                                  imag_values->header->type);
        for(int i=0; i<data.size(); ++i) {
            valuesComplex[i] = cx_double(data[i], imgData[i]);
        }
        _data->setYValues(valuesComplex);
    }

    setPopulated(true);
}


FileDescriptor *MatChannel::descriptor() const
{
    return parent;
}

int MatChannel::index() const
{
    return parent->channels.indexOf(const_cast<MatChannel*>(this), 0);
}

template<typename T>
T findSubrecord(const QString &name, MatlabStructArray *rec)
{DD;
    T result = T(0);
    int index = rec->fieldNames.indexOf(name);
    if (index >=0) result = dynamic_cast<T>(rec->subRecords[index]);
    return result;
}

