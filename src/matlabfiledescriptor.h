#ifndef MATLABFILEDESCRIPTOR_H
#define MATLABFILEDESCRIPTOR_H

#include "filedescriptor.h"
#include <QtGlobal>
#include <QtDebug>

#include "matfile.h"

struct XChannel
{
    QString name;
    QString units;
//    double logRef;
//    double scale;
    QString generalName;
    QString catLabel;
//    QString sensorId;
    QString sensorSerial;
    QString sensorName;
    double fd;
    QString chanUnits;
    QString pointId;
    QString direction;
    QStringList info;
};

struct Dataset
{
    QString id;
    QString fileName;
    QStringList titles;
    QString date;
    QString time;
    QList<XChannel> channels;
};

class MChannel {
public:
    QString label;
    double xStart;
    double xStep;
    ulong size;

    qint64 startPos;
    MatlabRecord::MatlabDataType dataType;
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

                stream.skipRawData(4); // пропускаем тип переменной

                quint32 recSize; // размер переменной в байтах
                stream >> recSize; //qDebug()<<"  rec size"<<recSize;
                qint64 recPos = stream.device()->pos(); //qDebug() << "  rec pos"<<recPos;


                stream.skipRawData(0x20);  // пропускаем 32 байта, описывающие:
                                           // тип массива - 16 байт
                                           // размерности массива - 16 байт

                quint32 nameFormat; stream >> nameFormat;
                if (nameFormat > 0xffff) {//short name
                    c.label=QString(stream.device()->read(4)); //qDebug()<<c.label;
                }
                else {
                    quint32 nameSize; stream >> nameSize;
                    c.label=QString(stream.device()->read(nameSize)); //qDebug()<<c.label;
                    if (nameSize%8!=0) stream.skipRawData(8-(nameSize%8));
                }
                c.label = c.label.section("_",0,0);

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

                stream.skipRawData(0x8); // пропускаем пустое имя массива

                // data type,  7 == single, 9 == double
                quint32 dataType; stream >> dataType; qDebug()<<"  type"<<dataType;
                c.dataType = (MatlabRecord::MatlabDataType)dataType;

                quint32 totalSize; stream >> totalSize; qDebug()<<"  totalSize in bytes" << totalSize;
                c.startPos = stream.device()->pos(); //qDebug()<<"  startPos"<<c.startPos;

                channels.append(c);
                stream.device()->seek(recSize+recPos);
                i++;
            }
        }
        int i=0;
        foreach (MChannel c, channels) {
            qDebug()<<"Channel"<<i++<<"type"<<c.dataType
                   <<"label" <<c.label
                   <<"length"<<c.size<<"xStart"<<c.xStart<<"xStep"<<c.xStep
                   <<"start"<<c.startPos;
        }
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
    void setFilesToConvert(const QStringList &toConvert) {filesToConvert = toConvert;}
    void setRawFileFormat(int format) {rawFileFormat = format;} // 0 = float, 1 = quint16

    QStringList getNewFiles() const {return newFiles;}

    QString xmlFileName;
public slots:
    bool convert();
signals:
    void tick();
    void finished();
    void message(const QString &s);
private:
    QList<Dataset> readXml(bool &success);
    QString folderName;
    QStringList newFiles;
    QStringList filesToConvert;
    int rawFileFormat;
};

#endif // MATLABFILEDESCRIPTOR_H
