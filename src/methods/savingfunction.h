#ifndef SAVINGFUNCTION_H
#define SAVINGFUNCTION_H

#include "methods/abstractfunction.h"
#include <QVector>
class FileDescriptor;
class Channel;

/* Свойства
 * --------
 * ?/ - ничего не знает
 * Saver/type - [int] file type (FileType)
 * Saver/destination - [string] destination folder
 * Saver/name - [string] file name
 *
 * Спрашивает:
 * ----------
 * ?/dataType <- ResamplingFunction, FftFunction
 * ?/functionType <- ResamplingFunction, FftFunction
 * ?/functionDescription <- ResamplingFunction, FftFunction
 * ?/dataFormat <- ChannelFunction, FftFunction
 * ?/channelIndex <- ChannelFunction
 *
 * ?/abscissaEven <- убрать <- ChannelFunction
 * ?/xStep <- ResamplingFunction, FftFunction
 * ?/xName <- ChannelFunction, FftFunction
 * ?/xBegin <- ChannelFunction (всегда 0)
 * ?/xType <- ChannelFunction, FftFunction
 * ?/abscissaData <- OctaveFunction
 *
 * ?/yType <- ChannelFunction
 * ?/yName <- ChannelFunction, FftFunction
 * ?/yValuesUnits <- ChannelFunction, FftFunction
 * ?/threshold <- ChannelFunction
 *
 * ?/zName <- ChannelFunction
 * ?/zCount <- ChannelFunction, FrameCutterFunction
 * ?/zStep <- ChannelFunction, FftFunction
 * ?/zBegin <- ChannelFunction
 * ?/zData
 *
 * ?/processData <- ResamplingFunction, FftFunction
 * ?/octaveFormat <- OctaveFunction
 * ?/weightingType
 * ?/windowType <- WindowingFunction
 * ?/amplitudeScaling
 * ?/normalization <- FftFunction
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
    virtual QVariant m_getProperty(const QString &property) const override;
    virtual void m_setProperty(const QString &property, const QVariant &val) override;
    virtual QVector<double> getData(const QString &id) override;
    virtual bool compute(FileDescriptor *file) override;
    virtual void reset() override;
private:
    FileDescriptor *createFile(FileDescriptor *file);
    FileDescriptor *createDfdFile(FileDescriptor *file);
    FileDescriptor *createUffFile(FileDescriptor *file);
    FileDescriptor *createD94File(FileDescriptor *file);
    Channel *createChannel(FileDescriptor *file, int dataSize, int blocksCount);
    Channel *createDfdChannel(FileDescriptor *file, int dataSize, int blocksCount);
    Channel *createUffChannel(FileDescriptor *file, int dataSize, int blocksCount);
    Channel *createD94Channel(FileDescriptor *file, int dataSize, int blocksCount);
    FileDescriptor *m_file;
    QString newFileName;
    QStringList newFiles;

    int type = 2;
    QString destination;
    QVector<QVector<double>> data;
};

#endif // SAVINGFUNCTION_H
