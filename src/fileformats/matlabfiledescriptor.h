#ifndef MATLABFILEDESCRIPTOR_H
#define MATLABFILEDESCRIPTOR_H

#include "fileformats/filedescriptor.h"
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
    void readXml(bool &success);

    QString xmlFileName;
    QList<Dataset> xml;
public slots:
    bool convert();
signals:
    void tick();
    void finished();
    void message(const QString &s);
private:

    QString folderName;
    QStringList newFiles;
    QStringList filesToConvert;
    int rawFileFormat;
};

#endif // MATLABFILEDESCRIPTOR_H
