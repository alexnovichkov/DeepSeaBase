#ifndef FORMATFACTORY_H
#define FORMATFACTORY_H

#include <QStringList>
#include "dfdfiledescriptor.h"
#include "ufffile.h"
#include "data94file.h"
#ifdef WITH_MATIO
#include "matlabfile.h"
#endif

namespace FormatFactory {

template<typename T>
QStringList suffixes()
{
    return T::suffixes();
}

inline QStringList allSuffixes(bool strip = false)
{
    QStringList result;
    result << suffixes<DfdFileDescriptor>();
    result << suffixes<UffFileDescriptor>();
    result << suffixes<Data94File>();
#ifdef WITH_MATIO
    result << suffixes<MatlabFile>();
#endif

    if (strip) result.replaceInStrings("*.","");
    return result;
}

template<typename T>
QStringList filters()
{
    return T::fileFilters();
}

inline QStringList allFilters()
{
    QStringList result;
    result << filters<DfdFileDescriptor>();
    result << filters<UffFileDescriptor>();
    result << filters<Data94File>();
#ifdef WITH_MATIO
    result << filters<MatlabFile>();
#endif
    return result;
}

inline FileDescriptor *createDescriptor(const QString &fileName)
{
    const QString suffix = QFileInfo(fileName).suffix().toLower();
    if (suffix=="dfd") return new DfdFileDescriptor(fileName);
    if (suffix=="uff") return new UffFileDescriptor(fileName);
    if (suffix=="d94") return new Data94File(fileName);
#ifdef WITH_MATIO
    if (suffix=="mat") return new MatlabFile(fileName);
#endif
    return 0;
}

//Эта функция предназначена в первую очередь для DFD файлов, которые не умеют хранить
//каналы разных типов.
//Эта функция создает несколько файлов, с каналами, сгруппированными по типу
inline QList<FileDescriptor *> createDescriptors(const FileDescriptor &source,
                                                 const QString &fileName,
                                                 const QVector<int> &indexes = QVector<int>())
{
    QString suffix = QFileInfo(fileName).suffix();
    if (suffix!="dfd") {
        if (suffix=="uff") return {new UffFileDescriptor(source, fileName, indexes)};
        if (suffix=="d94") return {new Data94File(source, fileName, indexes)};
    }
    else {
        //Сортируем каналы по типу
        QVector<int> idx = indexes;
        if (idx.isEmpty()) {
            idx = QVector<int>(source.channelsCount());
            std::iota(idx.begin(), idx.end(), 0);
        }
        QMap<Descriptor::DataType, QVector<int>> map;
        for (int index : idx) {
            auto type = source.channel(index)->type();
            map[type].append(index);
        }
        qDebug()<<map;
        //Создаем файлы
        QList<FileDescriptor *> result;
        for (const auto &[type, indexes]: asKeyValueRange(map)) {
            QString name = createUniqueFileName("", fileName, Descriptor::functionTypeDescription(type), "dfd", true);
            result << new DfdFileDescriptor(source, name, indexes);
        }
        return result;
    }
    return QList<FileDescriptor *>();
}

inline FileDescriptor *createDescriptor(const FileDescriptor &source,
                                        const QString &fileName,
                                        const QVector<int> &indexes = QVector<int>())
{
    QString suffix = QFileInfo(fileName).suffix();
    if (suffix=="dfd") return new DfdFileDescriptor(source, fileName, indexes);
    if (suffix=="uff") return new UffFileDescriptor(source, fileName, indexes);
    if (suffix=="d94") return new Data94File(source, fileName, indexes);
#ifdef WITH_MATIO
    if (suffix=="mat") return new MatlabFile(source, fileName, indexes);
#endif
    return 0;
}

inline FileDescriptor *createDescriptor(const QVector<Channel*> &source,
                                        const QString &fileName)
{
    QString suffix = QFileInfo(fileName).suffix();
    if (suffix=="dfd") return new DfdFileDescriptor(source, fileName);
    if (suffix=="uff") return new UffFileDescriptor(source, fileName);
    if (suffix=="d94") return new Data94File(source, fileName);
#ifdef WITH_MATIO
    if (suffix=="mat") return new MatlabFile(source, fileName);
#endif
    return 0;
}

}

#endif // FORMATFACTORY_H
