#ifndef TDMSCONVERTER_H
#define TDMSCONVERTER_H

#include <QObject>
#include "fileformats/tdmsfile.h"

class TDMSFileConverter : public QObject
{
    Q_OBJECT
public:
    TDMSFileConverter(QObject *parent = 0);
    void setFilesToConvert(const QStringList &toConvert) {filesToConvert = toConvert;}
    void setDestinationFormat(const QString &format)  {destinationFormat = format;} //dfd, uff, d94

    QStringList getNewFiles() const {return newFiles;}
public slots:
    bool convert();
signals:
    void tick();
    void finished();
    void message(const QString &s);
private:
    QString destinationFormat;
    QString folderName;
    QStringList newFiles;
    QStringList filesToConvert;
};

#endif // TDMSCONVERTER_H
