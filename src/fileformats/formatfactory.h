#ifndef FORMATFACTORY_H
#define FORMATFACTORY_H

#include <QStringList>
#include "filedescriptor.h"

namespace FormatFactory {

template<typename T>
QStringList suffixes()
{
    return T::suffixes();
}

QStringList allSuffixes(bool strip = false);

template<typename T>
QStringList filters()
{
    return T::fileFilters();
}

QStringList allFilters();

FileDescriptor *createDescriptor(const QString &fileName);

//Эта функция предназначена в первую очередь для DFD файлов, которые не умеют хранить
//каналы разных типов.
//Эта функция создает несколько файлов, с каналами, сгруппированными по типу
QList<FileDescriptor *> createDescriptors(const FileDescriptor &source,
                                                 const QString &fileName,
                                                 const QVector<int> &indexes = QVector<int>());

FileDescriptor *createDescriptor(const FileDescriptor &source,
                                        const QString &fileName,
                                        const QVector<int> &indexes = QVector<int>());

FileDescriptor *createDescriptor(const QVector<Channel*> &source,
                                        const QString &fileName);

}

#endif // FORMATFACTORY_H
