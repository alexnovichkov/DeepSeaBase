#ifndef ANACONVERTER_H
#define ANACONVERTER_H

#include <QStringList>
#include <QObject>

class AnaConverter : public QObject
{
    Q_OBJECT
public:
    AnaConverter(QObject *parent = nullptr) : QObject(parent)
    {}
    void setFilesToConvert(const QStringList &files);
    QStringList getNewFiles() const;
    void setDestinationFormat(const QString &format);
    void setTrimFiles(bool trim);
    void setTargetFolder(const QString &folder);
public slots:
    bool convert();
signals:
    void tick();
    void finished();
    void message(const QString &s);
private:
    QStringList files;
    QStringList newFiles;
    QString format;
    QString targetFolder;
    bool trimFiles;
};

#endif // ANACONVERTER_H


