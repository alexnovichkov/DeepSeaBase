#ifndef SAVINGFUNCTION_H
#define SAVINGFUNCTION_H

#include "methods/abstractfunction.h"
#include <QVector>
#include "fileformats/filedescriptor.h"
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
 * ?/weighting
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
    explicit SavingFunction(QObject *parent = nullptr, const QString &name=QString());

    // AbstractFunction interface
public:
    virtual QString name() const override;
    virtual QString displayName() const override;
    virtual QString description() const override;
    virtual QStringList properties() const override;
    virtual QString propertyDescription(const QString &property) const override;
    virtual QVariant m_getProperty(const QString &property) const override;
    virtual void m_setProperty(const QString &property, const QVariant &val) override;
    virtual bool compute(FileDescriptor *file) override;
    virtual void reset() override;
private:
    FileDescriptor *createFile(FileDescriptor *file);
    FileDescriptor *createDfdFile(FileDescriptor *file);
    FileDescriptor *createUffFile(FileDescriptor *file);
    FileDescriptor *createD94File(FileDescriptor *file);
    Channel *createChannel(FileDescriptor *file, DataHolder *data);
    Channel *createDfdChannel(FileDescriptor *file, DataHolder *data);
    Channel *createUffChannel(FileDescriptor *file, DataHolder *data);
    Channel *createD94Channel(FileDescriptor *file, DataHolder *data);
    FileDescriptor *m_file = nullptr;
    QString newFileName;
    QStringList newFiles;

    int type = 2;
    QString destination;
    QVector<QVector<double>> data;
};

#endif // SAVINGFUNCTION_H
