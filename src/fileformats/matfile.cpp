#include "matfile.h"

#include <QFile>

MatFile::MatFile(const QString &fileName) {
    this->fileName = fileName;

    QFile f(fileName);
    if (f.open(QFile::ReadOnly)) {
        QDataStream stream(&f);
        stream.setByteOrder(QDataStream::LittleEndian);
        stream.skipRawData(128); // текстовый заголовок файла, пропускаем

        while (!stream.atEnd()) {
            MatlabHeader *header = new MatlabHeader();
            header->read(&stream);

            MatlabRecord *rec = matlabRecordFactory(header, fileName); // <- сейчас указывает на начало данных
            if (!rec) {
                qDebug()<<"Unknown matlab data type:" << header->type;
                return;
            }

            records << rec;
            rec->readData(&stream);

            //stream.device()->seek(header->dataBegin);
            //stream.skipRawData(header->actualSize);
        }
    }
}

int MatFile::getChannelsCount() const
{
    int count = 0;
    foreach (MatlabRecord *rec, records) {
        if (rec) count += rec->getChannelsCount();
    }
    return count;
}




MatlabRecord *matlabRecordFactory(MatlabHeader *header, const QString &fileName)
{
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
            qDebug()<<"zlib data";
            return rec;
            break;
        }
        default: return 0;
    }
    if (rec) rec->fileName = fileName;
    return rec;
}

void MatlabNumericRecord::readData(QDataStream *f)
{
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
{
    QVector<double> data = this->getNumeric<double>();

    return data;
}

QVector<qint64> MatlabNumericRecord::getNumericAsInt() const
{
    QVector<qint64> data = this->getNumeric<qint64>();

    return data;
}

template<typename T>
QVector<T> MatlabNumericRecord::getNumeric() const
{
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
{
    qDeleteAll(subRecords);

}

void MatlabArrayRecord::readData(QDataStream *f)
{
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
//    qDebug()<<"Array name:"<<name<<"Array dimensions:"<<dimsL.join("x");

}

void MatlabHeader::read(QDataStream *f)
{
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

//        qDebug()<<"Array class ="<<arrayClass<<"flags="<<flags<< "Block size:"<<sizeInBytesWithoutPadding
//               << "actually"<<actualSize
//               <<"header starts with:" <<hex<<headerBegin<<dec
//               <<"data starts with:" <<hex<<dataBegin<<dec
//              << "compressed:"<<smallData;
    }
    else {
//        qDebug()<<"Matlab Data Type=" <<type
//               << "Block size:"<<sizeInBytesWithoutPadding
//               << "actually"<<actualSize
//               <<"header starts with:" <<hex<<headerBegin<<dec
//               <<"data starts with:" <<hex<<dataBegin<<dec
//              << "compressed:"<<smallData;
    }
}

void MatlabCellArray::readData(QDataStream *f)
{
    MatlabArrayRecord::readData(f);

    // читаем поля - это могут быть только miMatrix
    // это может быть также массив, поэтому вектор будет длиной dims1*dims2*dims3
    int totalSize = 1;
    foreach (int dim, dimensions) totalSize *= dim;

    for (int cellNumber = 0; cellNumber < totalSize; ++cellNumber) {
//        qDebug()<<"reading cell"<<cellNumber+1<<"of"<<totalSize;
        MatlabHeader *header = new MatlabHeader();
        header->read(f);

        MatlabRecord *rec = matlabRecordFactory(header, fileName); // <- сейчас указывает на начало данных
        if (!rec) {
            qDebug()<<"Unknown matlab data type:" << header->type;
            return;
        }
        rec->readData(f);

        subRecords << rec;
//        qDebug()<<"after reading cell" <<cellNumber+1<< "pos at"<<hex<<f->device()->pos()<<dec;
    }
}

void MatlabStructArray::readData(QDataStream *f)
{
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
//    qDebug()<<"  fields names"<<fieldNames;

    // читаем поля структуры - это могут быть любые типы
    // это может быть также массив структур, поэтому вектор будет длиной dims1*dims2*dims3
    int totalSize = 1;
    foreach (int dim, dimensions) totalSize *= dim;
    for (int structNumber = 0; structNumber < totalSize; ++structNumber) {
//        qDebug()<<"reading struct"<<structNumber+1<<"of"<<totalSize;
        for (int fieldNumber = 0; fieldNumber < fieldNames.size(); ++fieldNumber) {
//            qDebug()<<"reading field"<<fieldNames.at(fieldNumber)<<"of"<<fieldNames.size();

            MatlabHeader *header = new MatlabHeader();
            header->read(f);

            MatlabRecord *rec = matlabRecordFactory(header, fileName); // <- сейчас указывает на начало данных
            if (!rec) {
                qDebug()<<"Unknown matlab data type:" << header->type;
                return;
            }
            rec->readData(f);

            subRecords << rec;
//            qDebug()<<"end reading struct"<<fieldNames.at(fieldNumber)<<structNumber+1<<"of"<<totalSize <<"at"<<hex<<f->device()->pos()<<dec;
        }
    }
}

void MatlabObjectArray::readData(QDataStream *f)
{
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
//    qDebug()<<"  object fields names"<<fieldNames;


    //readnig fields
    int totalSize = 1;
    foreach (int dim, dimensions) totalSize *= dim;
    for (int fieldNumber = 0; fieldNumber < fieldNames.size(); ++fieldNumber) {
//        qDebug()<<"reading field"<<fieldNames.at(fieldNumber)<<"of"<<fieldNames.size();
        for (int cellNumber = 0; cellNumber < totalSize; ++cellNumber) {
//        qDebug()<<"reading cell"<<cellNumber+1<<"of"<<totalSize;
            MatlabHeader *header = new MatlabHeader();
            header->read(f);

            MatlabRecord *rec = matlabRecordFactory(header, fileName); // <- сейчас указывает на начало данных
            if (!rec) {
                qDebug()<<"Unknown matlab data type:" << header->type;
                return;
            }
            rec->readData(f);

            subRecords << rec;
//            qDebug()<<"after reading pos at"<<hex<<f->device()->pos()<<dec;
        }
    }
}

void MatlabCharacterArray::readData(QDataStream *f)
{
    MatlabArrayRecord::readData(f);

    MatlabHeader *h = new MatlabHeader;
    h->read(f);
    MatlabRecord *rec = matlabRecordFactory(h, fileName);
    if (rec) {
        rec->readData(f);
        values << rec->getString();
        delete rec;
    }

//    qDebug()<<"values"<<values;
}

QString MatlabCharacterArray::getString() const
{
    return values.join("");
}

void MatlabSparseArray::readData(QDataStream *f)
{
    MatlabArrayRecord::readData(f);
}

void MatlabNumericArray::readData(QDataStream *f)
{
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
//    if (values.size()==1) qDebug()<<"Value:"<<values.first();
//    else
//        qDebug()<<"    values size"<<values.size()<<"imags size"<<valuesImag.size();
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
{
    if (header->smallData) {
        data = QString::fromUtf8(f->device()->read(4));
    }
    else {
        data = QString::fromUtf8(f->device()->read(header->sizeInBytesWithoutPadding));
        f->device()->skip(header->actualSize - header->sizeInBytesWithoutPadding);
    }
}

void MatlabUtf16Record::readData(QDataStream *f)
{
//    qDebug()<<hex<<f->device()->pos()<<dec;
    if (header->smallData) {
        data = QString::fromUtf16((ushort*)(f->device()->read(4).data()));
    }
    else {
        data = QString::fromUtf16((ushort*)(f->device()->read(header->sizeInBytesWithoutPadding).data()));
        f->device()->skip(header->actualSize - header->sizeInBytesWithoutPadding);
    }
}


QVector<double> MatlabNumericArray::getNumericAsDouble() const
{
    QVector<double> data;
    if (!realValues) return data;

    if (MatlabNumericRecord *rec = dynamic_cast<MatlabNumericRecord*>(realValues)) {
        return rec->getNumericAsDouble();
    }
    return data;
}

QVector<qint64> MatlabNumericArray::getNumericAsInt() const
{
    QVector<qint64> data;
    if (!realValues) return data;

    if (MatlabNumericRecord *rec = dynamic_cast<MatlabNumericRecord*>(realValues)) {
        return rec->getNumericAsInt();
    }
    return data;
}

QByteArray MatlabRecord::getRaw() const
{
    QFile f(fileName);
    if (!f.open(QFile::ReadOnly))  return QByteArray();

    //QDataStream stream(&f);
    //stream.setByteOrder(QDataStream::LittleEndian);
    f.seek(header->dataBegin);
    return f.read(header->sizeInBytesWithoutPadding);
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
{
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
