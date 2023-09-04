#ifndef FORMATFACTORY_H
#define FORMATFACTORY_H

#include <QStringList>
#include "filedescriptor.h"

#include "abstractformatfactory.h"

template<typename T>
QStringList suffixes()
{
    return T::suffixes();
}

template<typename T>
QStringList filters()
{
    return T::fileFilters();
}

class FormatFactory : public AbstractFormatFactory
{
public:
    virtual QStringList allSuffixes(bool strip = false) override;
    virtual QStringList allFilters() override;

    //Эта функция предназначена в первую очередь для DFD файлов, которые не умеют хранить
    //каналы разных типов.
    //Эта функция создает несколько файлов, с каналами, сгруппированными по типу
    virtual QList<FileDescriptor *> createDescriptors(const FileDescriptor &source,
                                                     const QString &fileName,
                                                     const QVector<int> &indexes = QVector<int>()) override;
    /**
     * @brief createDescriptor читает файл fileName и создает дескриптор
     * @param fileName название файла
     * @return указатель на созданный объект
     */
    virtual FileDescriptor *createDescriptor(const QString &fileName) override;
    /**
     * @brief createDescriptor создает новый файл на основе данных из дескриптора source
     * @param source источник данных
     * @param fileName название нового файла
     * @param indexes список каналов, которые берутся из источника
     * @return
     */
    virtual FileDescriptor *createDescriptor(const FileDescriptor &source,
                                            const QString &fileName,
                                            const QVector<int> &indexes = QVector<int>()) override;
    /**
     * @brief createDescriptor создает новый файл из списка каналов source
     * @param source
     * @param fileName
     * @return
     */
    virtual FileDescriptor *createDescriptor(const QVector<Channel*> &source,
                                            const QString &fileName) override;

    virtual bool fileExists(const QString &s, const QString &suffix) override;
    virtual FileIO * createIO(const QVector<Channel *> &source, const QString &fileName,
                              const QMap<QString, QVariant> & parameters) override;
};

#endif // FORMATFACTORY_H
