#ifndef FORMATFACTORY_H
#define FORMATFACTORY_H

#include <QStringList>
#include "dfdfiledescriptor.h"
#include "ufffile.h"
#include "data94file.h"
#include "matfile.h"

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
    //result << suffixes<MatFile>();

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
    //result << filters<MatFile>();
    return result;
}

inline FileDescriptor *createDescriptor(const QString &fileName)
{
    const QString suffix = QFileInfo(fileName).suffix().toLower();
    if (suffix=="dfd") return new DfdFileDescriptor(fileName);
    if (suffix=="uff") return new UffFileDescriptor(fileName);
    if (suffix=="d94") return new Data94File(fileName);
   // if (suffix=="mat") return new MatFile(fileName);
    return 0;
}

inline FileDescriptor *createDescriptor(const FileDescriptor &source,
                                        const QString &fileName,
                                        const QVector<int> &indexes = QVector<int>())
{
    QString suffix = QFileInfo(fileName).suffix();
    if (suffix=="dfd") return new DfdFileDescriptor(source, fileName, indexes);
    if (suffix=="uff") return new UffFileDescriptor(source, fileName, indexes);
    if (suffix=="d94") return new Data94File(source, fileName, indexes);
    return 0;
}

}

#endif // FORMATFACTORY_H
