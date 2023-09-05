#ifndef FILEIO_H
#define FILEIO_H

#include <QObject>
#include <QMap>
#include <QVariant>

class Channel;
class DataDescription;
class DataHolder;

class FileIO : public QObject
{
    Q_OBJECT
public:
    explicit FileIO(QString fileName, QObject *parent = nullptr, const QMap<QString, QVariant> & parameters = {});
    virtual ~FileIO() {}
    void setParameter(const QString &name, const QVariant &value);
    void addChannel(Channel *channel);
    virtual void addChannel(DataDescription *description, DataHolder *data) = 0;
    virtual void finalize() = 0;
signals:
    void tick();
protected:
    QString fileName;
    QMap<QString, QVariant> m_parameters;
};

#endif // FILEIO_H
