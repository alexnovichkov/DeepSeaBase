#ifndef ABSTRACTFORMATFACTORY_H
#define ABSTRACTFORMATFACTORY_H

#include <QStringList>
#include <QVector>
#include <QVariant>

class FileDescriptor;
class Channel;
class FileIO;

class AbstractFormatFactory
{
public:
    virtual ~AbstractFormatFactory() {}
    virtual QStringList allFilters() = 0;
    virtual QStringList allSuffixes(bool strip = false) = 0;
    //Эта функция предназначена в первую очередь для DFD файлов, которые не умеют хранить
    //каналы разных типов.
    //Эта функция создает несколько файлов, с каналами, сгруппированными по типу
    virtual QList<FileDescriptor *> createDescriptors(const FileDescriptor &source,
                                                     const QString &fileName,
                                                     const QVector<int> &indexes = QVector<int>()) = 0;
    virtual bool fileExists(const QString &s, const QString &suffix) = 0;

    virtual FileDescriptor *createDescriptor(const QString &fileName) = 0;
    virtual FileDescriptor *createDescriptor(const FileDescriptor &source,
                                            const QString &fileName,
                                            const QVector<int> &indexes = QVector<int>()) = 0;
    virtual FileDescriptor *createDescriptor(const QVector<Channel*> &source,
                                            const QString &fileName) = 0;
    virtual FileIO *createIO(const QVector<Channel*> &source,
                             const QString &fileName,
                             const QMap<QString, QVariant> & parameters) = 0;
};

#endif // ABSTRACTFORMATFACTORY_H
