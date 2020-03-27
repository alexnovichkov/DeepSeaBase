#ifndef TDMSFILE_H
#define TDMSFILE_H

#include <QList>
#include <QFlag>
#include <QVariant>
#include <QObject>

#include "nilibddc.h"

class TDMSChannel
{
public:
    TDMSChannel(DDCChannelHandle channel);
    ~TDMSChannel();
    QVector<double> getDouble();
    QVector<float> getFloat();
    QVariantMap properties;
    quint64 numberOfValues = 0;
    DDCDataType dataType;
private:
    DDCChannelHandle channel;
};

class TDMSGroup
{
public:
    TDMSGroup(DDCChannelGroupHandle group);
    ~TDMSGroup();
    QList<TDMSChannel*> channels;
    QVariantMap properties;
private:
    DDCChannelHandle *_channels;
    DDCChannelGroupHandle group;
};

class TDMSFile
{
public:
    TDMSFile(const QString &fileName);
    ~TDMSFile();
    bool isValid() const {return _isValid;}
    QList<TDMSGroup*> groups;
    QVariantMap properties;
private:
    QString fileName;
    DDCFileHandle file;
    bool _isValid = true;
    DDCChannelGroupHandle *_groups;
};

#include <QObject>
#include <QFileInfoList>

class TDMSFileConvertor : public QObject
{
    Q_OBJECT
public:
    TDMSFileConvertor(QObject *parent = 0);
    void setFilesToConvert(const QStringList &toConvert) {filesToConvert = toConvert;}
    void setRawFileFormat(int format) {rawFileFormat = format;} // 0 = float, 1 = quint16

    QStringList getNewFiles() const {return newFiles;}
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
    int rawFileFormat = 0;
};

#endif // TDMSFILE_H
