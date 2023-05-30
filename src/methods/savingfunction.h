#ifndef SAVINGFUNCTION_H
#define SAVINGFUNCTION_H

#include "methods/abstractfunction.h"
#include <QVector>
#include "fileformats/filedescriptor.h"
class Channel;
class FileIO;

/* Свойства
 * --------
 * ?/ - ничего не знает
 * Saver/type - [int] file type (FileType)
 * Saver/destination - [url] destination folder
 * Saver/name - [string] file name
 * Saver/append - [string] Append channels / Write into new file (append/new)
 * Saver/precision - [enum] precision (single/double)
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
 * ?/logref <- ChannelFunction
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
    virtual QStringList parameters() const override;
    virtual QString m_parameterDescription(const QString &parameter) const override;
    virtual QVariant m_getParameter(const QString &parameter) const override;
    virtual void m_setParameter(const QString &parameter, const QVariant &val) override;
    virtual bool compute(FileDescriptor *file) override;
    virtual void reset() override;
protected:
    virtual bool m_parameterShowsFor(const QString &parameter) const override;
private:
    void updateAvailableTypes();
    FileDescriptor *createFile(FileDescriptor *file);
    FileDescriptor *createDfdFile();
    FileDescriptor *createUffFile();
    FileDescriptor *createD94File();

    FileIO *createFileIO(FileDescriptor *file);

    FileDescriptor *m_file = nullptr;
    QString newFileName;
    QString sourceFileName;
    QStringList newFiles;

    int type = 2; //тип файла согласно индексу в списке types
    QStringList availableTypes = {"DFD", "UFF", "D94"}; //список доступных типов
    QString destination;
    bool append = false;
    int precision = 0; // 0=single, 1 = double
    QVector<QVector<double>> data;

    // AbstractFunction interface
public:
    virtual void updateParameter(const QString &parameter, const QVariant &val) override;
};

#endif // SAVINGFUNCTION_H
