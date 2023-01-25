#ifndef ANACONVERTER_H
#define ANACONVERTER_H

#include <QStringList>
#include <QObject>

class AnaConverter : public QObject
{
public:
    void setFilesToConvert(const QStringList &files);
    QStringList getNewFiles() {}
    void setResultFile(const QString &resultFile) {}
};

#endif // ANACONVERTER_H


