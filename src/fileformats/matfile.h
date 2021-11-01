#ifndef MATFILE_H
#define MATFILE_H

#include <QtGlobal>
#include <QtDebug>
#include <QDataStream>

#include "filedescriptor.h"

struct XChannel
{
    QString name;
    QString units;
    double logRef;
    double scale;
    QString generalName;
    QString catLabel;
    QString sensorId;
    QString sensorSerial;
    QString sensorName;
    double fd;
    QString chanUnits;
    QString pointId;
    QString direction;
    QStringList info;
    QString expression;
    int fftDataType = 1; //из Analysis.xml: 0 = complex, 1 = real, 2 = phase
};

struct Dataset
{
    QString id;
    QString fileName;
    QString filePath;
    QString titles[3];
    QString date;
    QString time;
    QList<XChannel> channels;
    DataDescription toDataDescription() const {
        DataDescription r;
        r.put("source.name", filePath);
        QDate d = QDate::fromString(date, "dd.MM.yy");
        if (d.year()<1950) d=d.addYears(100);
        QTime t = QTime::fromString(time, "hh:mm:ss");
        r.put("dateTime", QDateTime(d,t).toString("dd.MM.yyyy hh:mm:ss"));
        r.put("description.title1", titles[0]);
        r.put("description.title2", titles[1]);
        r.put("description.title3", titles[2]);
        return r;
    }
};

class MatlabRecord;
class MatlabNumericRecord;
class MatlabChannel;
class MatlabStructArray;

class MatFile : public FileDescriptor {
public:
    MatFile(const QString &fileName);
    virtual ~MatFile();
    int getChannelsCount() const;

    void setXml(Dataset xml) {this->xml = xml;}
    void read() override;

    static QStringList fileFilters();
    static QStringList suffixes();

    QList<QVector<int>> groupChannels() const;

    QByteArray toJson() const;

    QList<MatlabRecord *> records;
    QList<MatlabChannel*> channels;
private:
    Dataset xml;

    // FileDescriptor interface
public:
    virtual void write() override;
//    virtual DescriptionList dataDescriptor() const override;
    virtual void deleteChannels(const QVector<int> &) override;
    virtual void copyChannelsFrom(const QVector<Channel*> &) override {}
    virtual int channelsCount() const override;
    virtual void move(bool, const QVector<int> &, const QVector<int> &) override;
    virtual Channel *channel(int index) const override;
};

class MatlabChannel : public Channel
{
public:
    explicit MatlabChannel(MatFile *parent);

    MatFile *parent;
    MatlabNumericRecord *real_values;
    MatlabNumericRecord *imag_values;
    XChannel xml;
    QString _name;
    QString _primaryChannel;
    QString _type;
    int _octaveType = 0;
    bool grouped = false;
    int indexInGroup = 0;
    int groupSize = 1;
    bool complex = false;

    // Channel interface
public:
    virtual Descriptor::DataType type() const override;
    virtual void populate() override;
    virtual FileDescriptor *descriptor() const override;
    virtual int index() const override;

};

template <typename T>
T findSubrecord(const QString &name, MatlabStructArray *rec);

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
    bool smallData = false;

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
    virtual QByteArray getRaw(qint64 position, qint64 length) const;

    virtual int getChannelsCount() const {return 0;}

    virtual QJsonValue toJson() const;

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
    virtual QJsonValue toJson() const override;

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
    virtual QString getString() const override {return data;}
    virtual QJsonValue toJson() const override;
};

class MatlabUtf16Record : public MatlabRecord
{
public:
    MatlabUtf16Record(MatlabHeader *header) : MatlabRecord(header) {}
    virtual void readData(QDataStream *f) override;
    QString data;
    virtual QJsonValue toJson() const override;
    virtual QString getString() const override {return data;}
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

    virtual QJsonValue toJson() const override;

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
    QJsonValue toJson() const override;
};
class MatlabStructArray : public MatlabArrayRecord
{
public:
    MatlabStructArray(MatlabHeader *header) : MatlabArrayRecord(header) {}
    virtual void readData(QDataStream *f) override;
    QJsonValue toJson() const override;

    QStringList fieldNames;
};
class MatlabObjectArray : public MatlabArrayRecord
{
public:
    MatlabObjectArray(MatlabHeader *header) : MatlabArrayRecord(header) {}
    virtual void readData(QDataStream *f) override;
    QJsonValue toJson() const override;
    QString className;
    QStringList fieldNames;
};
class MatlabCharacterArray : public MatlabArrayRecord
{
public:
    MatlabCharacterArray(MatlabHeader *header) : MatlabArrayRecord(header) {}
    virtual void readData(QDataStream *f) override;
    QJsonValue toJson() const override;

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
    QJsonValue toJson() const override;
};

class MatlabNumericArray : public MatlabArrayRecord
{
public:
    MatlabNumericArray(MatlabHeader *header) : MatlabArrayRecord(header) {}
    virtual ~MatlabNumericArray() {delete realValues; delete imagValues;}
    QJsonValue toJson() const override;
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



#endif // MATFILE_H
