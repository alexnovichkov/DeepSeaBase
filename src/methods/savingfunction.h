#ifndef SAVINGFUNCTION_H
#define SAVINGFUNCTION_H

#include "methods/abstractfunction.h"
#include <QVector>
class FileDescriptor;
class Channel;

/* Свойства
 * ?/ - ничего не знает
 * Saver/type - [int] file type (FileType)
 * Saver/destination - [string] destination folder
 * Saver/name - [string] file name
 *
 * Спрашивает:
 * ?/dataType <- ResamplingFunction, FftFunction
 * ?/functionType <- ResamplingFunction, FftFunction
 * ?/functionDescription <- ResamplingFunction, FftFunction
 * ?/dataFormat <- ResamplingFunction, FftFunction
 * ?/channelIndex <- ChannelFunction
 *
 * ?/abscissaEven <- убрать <- ResamplingFunction
 * ?/xStep <- ResamplingFunction, FftFunction
 * ?/xName <- ResamplingFunction, FftFunction
 * ?/xBegin <- ResamplingFunction (всегда 0)
 * ?/xType <- ResamplingFunction, FftFunction
 * ?/abscissaData <- OctaveFunction
 *
 * ?/yType <- ResamplingFunction
 * ?/yName
 * ?/yValuesUnits
 * ?/threshold
 *
 * ?/zName
 * ?/zCount
 * ?/zStep
 * ?/zBegin
 * ?/zData
 *
 * ?/processData
 * ?/octaveFormat
 * ?/weightingType
 * ?/windowType
 * ?/amplitudeScaling
 * ?/normalization
 *
 */

class SavingFunction : public AbstractFunction
{
public:
    enum FileType {
        DfdFile = 0,
        UffFile = 1,
        D94File = 2
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
    FileDescriptor *createD94File(FileDescriptor *file);
    Channel *createChannel(FileDescriptor *file, int dataSize);
    Channel *createDfdChannel(FileDescriptor *file, int dataSize);
    Channel *createUffChannel(FileDescriptor *file, int dataSize);
    Channel *createD94Channel(FileDescriptor *file, int dataSize);
    FileDescriptor *m_file;
    QString newFileName;
    QStringList newFiles;

    int type = 2;
    QString destination;
    QVector<QVector<double>> data;
};

#endif // SAVINGFUNCTION_H
