#ifndef SAVINGFUNCTION_H
#define SAVINGFUNCTION_H

#include "methods/abstractfunction.h"
#include <QVector>
class FileDescriptor;
class Channel;

class SavingFunction : public AbstractFunction
{
public:
    enum FileType {
        DfdFile = 0,
        UffFile = 1
    };
    explicit SavingFunction(QObject *parent = nullptr);

    // AbstractFunction interface
public:
    virtual QString name() const override;
    virtual QString displayName() const override;
    virtual QString description() const override;
    virtual QStringList properties() const override;
    virtual QString propertyDescription(const QString &property) const override;
    virtual bool propertyShowsFor(const QString &property) const override;
    virtual QVariant getProperty(const QString &property) const override;
    virtual void setProperty(const QString &property, const QVariant &val) override;
    virtual QVector<double> getData(const QString &id) override;
    virtual bool compute(FileDescriptor *file) override;
    virtual void reset() override;
private:
    FileDescriptor *createFile(FileDescriptor *file);
    FileDescriptor *createDfdFile(FileDescriptor *file);
    FileDescriptor *createUffFile(FileDescriptor *file);
    Channel *createChannel(FileDescriptor *file, int dataSize);
    Channel *createDfdChannel(FileDescriptor *file, int dataSize);
    Channel *createUffChannel(FileDescriptor *file, int dataSize);
    FileDescriptor *m_file;
    QString newFileName;
    QStringList newFiles;

    int type = 1;
    QString destination;
    QVector<QVector<double>> data;
};

#endif // SAVINGFUNCTION_H
