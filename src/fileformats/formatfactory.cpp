#include "formatfactory.h"

#include "dfdfiledescriptor.h"
#include "ufffile.h"
#include "data94file.h"
#ifdef WITH_MATIO
#include "matlabfile.h"
#endif
#include "anafile.h"
#include "wavfile.h"
#include "logging.h"

QList<FileDescriptor *> FormatFactory::createDescriptors(const FileDescriptor &source, const QString &fileName, const QVector<int> &indexes)
{DD;
    QString suffix = QFileInfo(fileName).suffix();
    if (suffix=="wav" || suffix=="dfd") {
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

        //Создаем файлы
        QList<FileDescriptor *> result;
        for (const auto &[type, indexes]: asKeyValueRange(map)) {
            if (suffix=="wav" && type != Descriptor::TimeResponse) continue;
            QString name = createUniqueFileName("", fileName, Descriptor::functionTypeDescription(type), suffix, true);
            result << createDescriptor(source, name, indexes);
        }
        return result;
    }
    return {createDescriptor(source, fileName, indexes)};
}

QStringList FormatFactory::allSuffixes(bool strip)
{DD;
    QStringList result;
    result << suffixes<DfdFileDescriptor>();
    result << suffixes<UffFileDescriptor>();
    result << suffixes<Data94File>();
    result << suffixes<WavFile>();
    result << suffixes<AnaFile>();
#ifdef WITH_MATIO
    result << suffixes<MatlabFile>();
#endif

    if (strip) result.replaceInStrings("*.","");
    return result;
}

QStringList FormatFactory::allFilters()
{DD;
    QStringList result;
    result << filters<DfdFileDescriptor>();
    result << filters<UffFileDescriptor>();
    result << filters<Data94File>();
    result << filters<WavFile>();
    result << filters<AnaFile>();
#ifdef WITH_MATIO
    result << filters<MatlabFile>();
#endif
    return result;
}

FileDescriptor *FormatFactory::createDescriptor(const QString &fileName)
{DD;
    const QString suffix = QFileInfo(fileName).suffix().toLower();
    if (suffix=="dfd") return new DfdFileDescriptor(fileName);
    if (suffix=="uff") return new UffFileDescriptor(fileName);
    if (suffix=="d94") return new Data94File(fileName);
    if (suffix == "wav") return new WavFile(fileName);
    if (suffix == "anp") return new AnaFile(fileName);
#ifdef WITH_MATIO
    if (suffix=="mat") return new MatlabFile(fileName);
#endif
    return 0;
}

FileDescriptor *FormatFactory::createDescriptor(const FileDescriptor &source, const QString &fileName, const QVector<int> &indexes)
{DD;
    QString suffix = QFileInfo(fileName).suffix();
    if (suffix=="dfd") return new DfdFileDescriptor(source, fileName, indexes);
    if (suffix=="uff") return new UffFileDescriptor(source, fileName, indexes);
    if (suffix=="d94") return new Data94File(source, fileName, indexes);
    if (suffix=="wav") return new WavFile(source, fileName, indexes, WavFormat::WavFloat);

#ifdef WITH_MATIO
    if (suffix=="mat") return new MatlabFile(source, fileName, indexes);
#endif
    return 0;
}

FileDescriptor *FormatFactory::createDescriptor(const QVector<Channel *> &source, const QString &fileName)
{DD;
    QString suffix = QFileInfo(fileName).suffix();
    if (suffix=="dfd") return new DfdFileDescriptor(source, fileName);
    if (suffix=="uff") return new UffFileDescriptor(source, fileName);
    if (suffix=="d94") return new Data94File(source, fileName);
    if (suffix=="wav") return new WavFile(source, fileName, WavFormat::WavFloat);
#ifdef WITH_MATIO
    if (suffix=="mat") return new MatlabFile(source, fileName);
#endif
    return 0;
}


bool FormatFactory::fileExists(const QString &s, const QString &suffix)
{DD;
    QString f = changeFileExt(s, suffix);
    if (suffix == "dfd") {
        QString f1 = changeFileExt(s, "raw");
        return (QFile::exists(f) && QFile::exists(f1));
    }
    else if (suffix == "anp") {
        QString f1 = changeFileExt(s, "ana");
        return (QFile::exists(f) && QFile::exists(f1));
    }
    return QFile::exists(f);
}
