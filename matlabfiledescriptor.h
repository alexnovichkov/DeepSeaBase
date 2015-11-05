#ifndef MATLABFILEDESCRIPTOR_H
#define MATLABFILEDESCRIPTOR_H

#include "filedescriptor.h"
#include <QtGlobal>
#include <QtDebug>

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

//class MatlabData {
//public:
//    MatlabData() {}
//    virtual ~MatlabData() {}
//    virtual void readHeader(QDataStream *f) {
//        *f >> dataType;
//        *f >> size;
//        dataBegin = f->device()->pos();
//        actualSize = size;
//        if (size % 8 != 0) actualSize += 8-(size%8);
//        //qDebug()<<"Block size:"<<size << "actually"<<actualSize<<"starts with:"<<dataBegin;
//    }
//    virtual void readData(QDataStream *f) = 0;

//    quint32 dataType; //MatlabDataType
//    quint32 size; // if dataType==miMATRIX, then size includes padding, otherwise size does not include padding,
//                  // but data is always padded to 64-bit boundary
//    quint32 dataBegin;
//    quint32 actualSize; //including padding
//};

template <typename T>
QVector<T> readBlock(QDataStream *readStream, ulong itemCount, double scale)
{
    QVector<T> result(itemCount);
    T v;

    for (quint32 i=0; i<itemCount; ++i) {
        if (readStream->atEnd()) {
            break;
        }

        *readStream >> v;
        result[i] = T(v) /** scale*/;
    }

    return result;
}

//class MatlabMatrixData;

//class MatlabArray {
//public:
//    MatlabArray(MatlabMatrixData *parent) : parent(parent) {}
//    virtual ~MatlabArray() {}
//    virtual void read(QDataStream *f) = 0;
//    void setComplex(bool complex) {m_complex = complex;}
//MatlabMatrixData *parent;
//private:
//    bool m_complex;

//};

//class MatlabPlainArray : public MatlabArray
//{
//public:
//    MatlabPlainArray(MatlabMatrixData *parent) : MatlabArray(parent) {qDebug()<<"  plain array";}
//    virtual void read(QDataStream *f);
//private:
//    QVector<double> data;
//};

//class MatlabCharArray : public MatlabArray
//{
//public:
//    MatlabCharArray(MatlabMatrixData *parent) : MatlabArray(parent) {qDebug()<<"  character array";}
//    virtual void read(QDataStream *f);
//private:
//    QString data;
//};


//class MatlabStructArray : public MatlabArray
//{
//public:
//    MatlabStructArray(MatlabMatrixData *parent) : MatlabArray(parent) {/*qDebug()<<"  struct array";*/}
//    ~MatlabStructArray() {
//        qDeleteAll(fields);
//    }
//    virtual void read(QDataStream *f);
//private:
//    QList<MatlabMatrixData*> fields;
//    QStringList fieldNames;
//};

//class MatlabMatrixData : public MatlabData {
//public:
//    enum MatrixFlags {
//        FlagLogical = 2,
//        FlagGlobal = 4,
//        FlagComplex = 8
//    };
//    enum MatlabArrayType {
//        mxCELL_CLASS   = 1,//Cell array
//        mxSTRUCT_CLASS = 2,//Structure
//        mxOBJECT_CLASS = 3,//Object
//        mxCHAR_CLASS   = 4,//Character array
//        mxSPARSE_CLASS = 5,//Sparse array
//        mxDOUBLE_CLASS = 6,//Double precision array
//        mxSINGLE_CLASS = 7,//Single precision array
//        mxINT8_CLASS   = 8,//8-bit, signed integer
//        mxUINT8_CLASS  = 9,//8-bit, unsigned integer
//        mxINT16_CLASS  = 10,//16-bit, signed integer
//        mxUINT16_CLASS = 11,//16-bit, unsigned integer
//        mxINT32_CLASS  = 12,//32-bit, signed integer
//        mxUINT32_CLASS = 13,//32-bit, unsigned integer
//        mxINT64_CLASS  = 14,//64-bit, signed integer
//        mxUINT64_CLASS = 15//64-bit, unsigned integer
//    };
//    MatlabMatrixData() : MatlabData() {
//        //qDebug()<<"Matrix";
//    }

//    ~MatlabMatrixData() {
//        qDeleteAll(dataArray);
//    }

//    virtual void readData(QDataStream *f) {
//        quint64 curPos = f->device()->pos();
//        f->skipRawData(8); //skipping flagsDataType and flagsSize
//        *f >> arrayType; //qDebug()<<"  array type:"<<arrayType;
//        *f >> flags; //qDebug()<<"  flags:"<<flags;
//        f->skipRawData(10); //skipping flags padding

//        quint32 dimensionsSize;
//        *f >> dimensionsSize;
//        f->skipRawData(dimensionsSize+8); //skipping dimensions

//        f->skipRawData(4); //skipping nameDataType
//        quint32 nameSize;
//        *f >> nameSize;
//        f->skipRawData(nameSize);
//        if (nameSize % 8 != 0) f->skipRawData(8-(nameSize%8)); //name padding

//        MatlabArray *data = 0;
//        switch (arrayType) {
//            case mxDOUBLE_CLASS:
//            case mxSINGLE_CLASS:
//            case mxINT8_CLASS:
//            case mxUINT8_CLASS:
//            case mxINT16_CLASS:
//            case mxUINT16_CLASS:
//            case mxINT32_CLASS:
//            case mxUINT32_CLASS:
//            case mxINT64_CLASS:
//            case mxUINT64_CLASS: data = new MatlabPlainArray(this); break;
//            case mxSTRUCT_CLASS: data = new MatlabStructArray(this); break;
//            case mxCHAR_CLASS: data = new MatlabCharArray(this); break;
//            default: break;
//        }
//        if (data) {
//            data->read(f);
//            dataArray << data;
//        }

//        f->device()->seek(curPos);
//        f->skipRawData(actualSize); return;
//    }
//private:
//    friend class MatlabPlainArray;
//    quint8 flags;
//    quint8 arrayType; // MatlabArrayType

//    QString name;
//    QList<MatlabArray *> dataArray;
//};



class MChannel {
public:
    QString label;
    double xStart;
    double xStep;
    ulong size;

    qint64 startPos;
    MatlabDataType dataType;
};

class MatlabFile {
public:
    MatlabFile(const QString &fileName) : m_fileName(fileName) {}

    void read() {
        writtenAsMatrix = false;
        QFile f(m_fileName);
        if (f.open(QFile::ReadOnly)) {
            QDataStream stream(&f);
            stream.setByteOrder(QDataStream::LittleEndian);
            stream.skipRawData(128);

            int i=0;
            while (!f.atEnd()) {//qDebug()<<"Now at"<<stream.device()->pos();
                MChannel c; //qDebug()<<"Channel"<<i<<stream.device()->pos();

                stream.skipRawData(4);

                quint32 recSize;
                stream >> recSize; //qDebug()<<"  rec size"<<recSize;
                qint64 recPos = stream.device()->pos(); //qDebug() << "  rec pos"<<recPos;


                stream.skipRawData(0x20);
                quint32 nameFormat; stream >> nameFormat;
                if (nameFormat > 0xffff) {//short name
                    c.label=QString(stream.device()->read(4)); //qDebug()<<c.label;
                    c.label = c.label.section("_",0,0);
                }
                else {
                    quint32 nameSize; stream >> nameSize;
                    c.label=QString(stream.device()->read(nameSize)); //qDebug()<<c.label;
                    c.label = c.label.section("_",0,0);
                    if (nameSize%8!=0) stream.skipRawData(8-(nameSize%8));
                }

                stream.skipRawData(0x44);
                quint32 xValuesSize; stream >> xValuesSize;
                qint64 xValuesPos = stream.device()->pos(); //qDebug()<<"  xValues Pos"<<xValuesPos<<xValuesSize;

                stream.skipRawData(0x88);
                stream.skipRawData(0x2c);
                quint32 xStart; stream >> xStart;
                c.xStart = xStart;

                stream.skipRawData(0x30);
                quint32 type; stream >> type; //qDebug()<<"  type"<<type;
                stream.skipRawData(4);
                if (type==9) stream >> c.xStep;
                else {
                    //qDebug()<<"Unknown xStep type at"<<(stream.device()->pos()-4);
                    stream.skipRawData(8);
                }
                //qDebug()<<"  xStep "<<c.xStep;

                stream.skipRawData(0x34);
                quint32 size;
                stream >> size;
                c.size = size; //qDebug()<<"size"<<c.size;

                //reading yValues
                stream.device()->seek(xValuesPos+xValuesSize); //qDebug()<<"  current ypos"<<stream.device()->pos();

                stream.skipRawData(0x58); //yValues header;
                stream.skipRawData(0x20); //yValues header;
                quint32 s1,s2;
                stream >> s1 >> s2; //qDebug()<<s1<<s2;
                if (s1 != 1 && s2 != 1) {
                    writtenAsMatrix = true; //qDebug()<<"matrix size mismatch at"<<stream.device()->pos();
                    return;
                }

                stream.skipRawData(0x8);
                quint32 dataType; stream >> dataType; //qDebug()<<"  type"<<dataType;
                c.dataType = (MatlabDataType)dataType;
                quint32 totalSize; stream >> totalSize; //qDebug()<<"  totalSize"<<totalSize;
                c.startPos = stream.device()->pos(); //qDebug()<<"  startPos"<<c.startPos;

                channels.append(c);
                stream.device()->seek(recSize+recPos);
                i++;
            }
        }
//        int i=0;
//        foreach (MChannel c, channels) {
//            qDebug()<<"Channel"<<i++<<"type"<<c.dataType<<"length"<<c.size<<"xStart"<<c.xStart<<"xStep"<<c.xStep
//                   <<"start"<<c.startPos;
//        }
    }

    QList<MChannel> channels;
    QString m_fileName;
    bool writtenAsMatrix;
private:

};

#include <QObject>
#include <QFileInfoList>

class MatlabConvertor : public QObject
{
    Q_OBJECT
public:
    MatlabConvertor(QObject *parent = 0);
    void setFolder(const QString &folder);
    void setFilesToConvert(const QStringList &toConvert) {filesToConvert = toConvert;}

    QStringList getMatFiles() const;
    QStringList getNewFiles() const {return newFiles;}
public slots:
    bool convert();
signals:
    void tick();
    void finished();
    void message(const QString &s);
private:
    QString folderName;
    QFileInfoList matFiles;
    QStringList newFiles;
    QStringList filesToConvert;
};

#endif // MATLABFILEDESCRIPTOR_H
