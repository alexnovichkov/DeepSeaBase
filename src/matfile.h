#ifndef MATFILE_H
#define MATFILE_H

#include <QtGlobal>
#include <QtDebug>
#include <QDataStream>

template <typename T, typename D>
QVector<D> readBlock(QDataStream *readStream, quint64 itemCount)
{
    QVector<D> result(itemCount);
    T v;

    for (quint64 i=0; i<itemCount; ++i) {
        if (readStream->atEnd())
            break;

        *readStream >> v;
        result[i] = D(v);
    }

    return result;
}

template <typename T>
QVector<T> readNumeric(QDataStream *stream, quint64 itemCount, int type)
{
    QVector<T> data;

    switch(type) {
        case 1: data = readBlock<qint8, T>(stream, itemCount); break;
        case 2: data = readBlock<quint8, T>(stream, itemCount); break;
        case 3: data = readBlock<qint16, T>(stream, itemCount); break;
        case 4: data = readBlock<quint16, T>(stream, itemCount); break;
        case 5: data = readBlock<qint32, T>(stream, itemCount); break;
        case 6: data = readBlock<quint32, T>(stream, itemCount); break;
        case 7: stream->setFloatingPointPrecision(QDataStream::SinglePrecision);
            data = readBlock<float, T>(stream, itemCount); break;
        case 9: stream->setFloatingPointPrecision(QDataStream::DoublePrecision);
            data = readBlock<double, T>(stream, itemCount); break;
        case 12: data = readBlock<qint64, T>(stream, itemCount); break;
        case 13: data = readBlock<quint64, T>(stream, itemCount); break;
        default: break;
    }
    return data;
}

class MatlabHeader {
public:
    MatlabHeader() {}
    void read(QDataStream *f);

    quint32 type = 0; //MatlabDataType
    quint32 sizeInBytesWithoutPadding = 0; // if dataType==miMATRIX, then size includes padding, otherwise size does not include padding,
                  // but data is always padded to 64-bit boundary

    quint64 headerBegin = 0;
    quint64 dataBegin = 0;
    quint32 actualSize = 0; //including padding
    bool compressed = false;

    quint8 flags = 0;
    quint8 arrayClass = 0;
};


class MatlabRecord {
public:
    enum MatlabDataType {
        miUndefined = 0,
        miINT8 = 1,// 8 bit, signed
        miUINT8 = 2,// 8 bit, unsigned
        miINT16 = 3,// 16-bit, signed
        miUINT16 = 4,// 16-bit, unsigned
        miINT32 = 5,// 32-bit, signed
        miUINT32 = 6,// 32-bit, unsigned
        miSINGLE = 7,// IEEE® 754 single format
        miDOUBLE = 9,// IEEE 754 double format
        miINT64 = 0x0C,// 64-bit, signed
        miUINT64 = 0x0D,// 64-bit, unsigned
        miMATRIX = 0x0E,//MATLAB array
        miCOMPRESSED = 0x0F,//Compressed Data
        miUTF8 = 0x10,// Unicode UTF-8 Encoded Character Data
        miUTF16 = 0x11,// Unicode UTF-16 Encoded Character Data
        miUTF32 = 0x12// Unicode UTF-32 Encoded Character Data
    };

    MatlabRecord(MatlabHeader *header) : header(header) {}
    virtual ~MatlabRecord() {delete header;}
    virtual void readData(QDataStream *f) = 0;

    MatlabHeader *header = 0;
    virtual QString getString() const {return QString();}
    virtual QVector<double> getNumericAsDouble() const {return QVector<double>();}
    virtual QVector<qint64> getNumericAsInt() const {return QVector<qint64>();}
    virtual QByteArray getRaw() const;

    virtual int getChannelsCount() const {return 0;}

    int actualDataType; // будет отличаться, если числовые данные записаны со сжатием.
    quint64 actualDataSize; // количество единиц данных

    QString fileName;
};

class MatlabNumericRecord : public MatlabRecord
{
public:
    MatlabNumericRecord(MatlabHeader *header) : MatlabRecord(header) {}
    virtual void readData(QDataStream *f) override;
    virtual QVector<double> getNumericAsDouble() const override;
    virtual QVector<qint64> getNumericAsInt() const override;

    template <typename T>
    QVector<T> getNumeric() const;
};

//template <typename T>
//QVector<T> getNumeric(const MatlabNumericRecord *rec);

QVector<float> getNumeric(const MatlabNumericRecord *rec);

class MatlabUtf8Record : public MatlabRecord
{
public:
    MatlabUtf8Record(MatlabHeader *header) : MatlabRecord(header) {}
    virtual void readData(QDataStream *f) override;
    QString data;
    virtual QString getString() const {return data;}
};

class MatlabUtf16Record : public MatlabRecord
{
public:
    MatlabUtf16Record(MatlabHeader *header) : MatlabRecord(header) {}
    virtual void readData(QDataStream *f) override;
    QString data;
    virtual QString getString() const {return data;}
};

class MatlabArrayRecord : public MatlabRecord
{
public:
    enum MatlabArrayType {
        mxCELL_CLASS   = 1,//Cell array
        mxSTRUCT_CLASS = 2,//Structure
        mxOBJECT_CLASS = 3,//Object
        mxCHAR_CLASS   = 4,//Character array
        mxSPARSE_CLASS = 5,//Sparse array
        mxDOUBLE_CLASS = 6,//Double precision array
        mxSINGLE_CLASS = 7,//Single precision array
        mxINT8_CLASS   = 8,//8-bit, signed integer
        mxUINT8_CLASS  = 9,//8-bit, unsigned integer
        mxINT16_CLASS  = 10,//16-bit, signed integer
        mxUINT16_CLASS = 11,//16-bit, unsigned integer
        mxINT32_CLASS  = 12,//32-bit, signed integer
        mxUINT32_CLASS = 13,//32-bit, unsigned integer
        mxINT64_CLASS  = 14,//64-bit, signed integer
        mxUINT64_CLASS = 15//64-bit, unsigned integer
    };
    MatlabArrayRecord(MatlabHeader *header) : MatlabRecord(header) {}
    virtual ~MatlabArrayRecord();

    int arrayClass = 0;
    bool complex = false;
    bool global = false;
    bool logical = false;
    QVector<int> dimensions;
    QString name;

    QList<MatlabRecord *> subRecords;

    virtual void readData(QDataStream *f) override;
private:

};

class MatlabCellArray : public MatlabArrayRecord
{
public:
    MatlabCellArray(MatlabHeader *header) : MatlabArrayRecord(header) {}
    virtual void readData(QDataStream *f) override;
};
class MatlabStructArray : public MatlabArrayRecord
{
public:
    MatlabStructArray(MatlabHeader *header) : MatlabArrayRecord(header) {}
    virtual void readData(QDataStream *f) override;

    QStringList fieldNames;
};
class MatlabObjectArray : public MatlabArrayRecord
{
public:
    MatlabObjectArray(MatlabHeader *header) : MatlabArrayRecord(header) {}
    virtual void readData(QDataStream *f) override;
    QString className;
    QStringList fieldNames;
};
class MatlabCharacterArray : public MatlabArrayRecord
{
public:
    MatlabCharacterArray(MatlabHeader *header) : MatlabArrayRecord(header) {}
    virtual void readData(QDataStream *f) override;

    QStringList values;

    // MatlabRecord interface
public:
    virtual QString getString() const override;
};
class MatlabSparseArray : public MatlabArrayRecord
{
public:
    MatlabSparseArray(MatlabHeader *header) : MatlabArrayRecord(header) {}
    virtual void readData(QDataStream *f) override;
};

class MatlabNumericArray : public MatlabArrayRecord
{
public:
    MatlabNumericArray(MatlabHeader *header) : MatlabArrayRecord(header) {}
    virtual ~MatlabNumericArray() {delete realValues; delete imagValues;}

    virtual void readData(QDataStream *f) override;


    MatlabRecord *realValues = 0;
    MatlabRecord *imagValues = 0;
private:

    //void readElement(QDataStream *f, QVector<T> &val);

    // MatlabRecord interface
public:
    // returns real values only
    virtual QVector<double> getNumericAsDouble() const override;
    virtual QVector<qint64> getNumericAsInt() const override;
};


MatlabRecord *matlabRecordFactory(MatlabHeader *header, const QString &fileName);

class MatFile {
public:
    MatFile(const QString &fileName);
    ~MatFile() {
        qDeleteAll(records);
    }
    int getChannelsCount() const;
    QString fileName;
    QList<MatlabRecord *> records;
};





#endif // MATFILE_H
