#ifndef MATLABCONVERTOR_H
#define MATLABCONVERTOR_H

#include "fileformats/filedescriptor.h"
#include <QtGlobal>
#include <QtDebug>
#include <QMap>

#include "fileformats/matfile.h"



#include <QObject>
#include <QFileInfoList>

class MatlabConvertor : public QObject
{
    Q_OBJECT
public:
    MatlabConvertor(QObject *parent = 0);
    void setFilesToConvert(const QStringList &toConvert) {filesToConvert = toConvert;}
    void setRawFileFormat(int format) {rawFileFormat = format;} // 0 = float, 1 = quint16
    void setDatasetForFile(const QString &file, int dataset) {datasets.insert(file, dataset);}
    void setDestinationFormat(const QString &format) {destinationFormat = format;} //dfd, uff, d94
    void setOnlyTimeChannels(bool onlyTimeChannels) {this->onlyTimeChannels = onlyTimeChannels;}

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
    void converted(const QString &file);
private:
    QMap<QString, int> datasets;
    QString folderName;
    QStringList newFiles;
    QStringList filesToConvert;
    int rawFileFormat = 0;
    QString destinationFormat;
    bool onlyTimeChannels = false;
};

#endif // MATLABCONVERTOR_H
