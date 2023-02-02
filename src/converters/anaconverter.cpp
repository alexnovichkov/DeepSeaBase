#include "anaconverter.h"
#include <QThread>
#include "fileformats/anafile.h"
#include "app.h"
#include "fileformats/formatfactory.h"
#include "algorithms.h"
#include "methods/calculations.h"

void AnaConverter::setFilesToConvert(const QStringList &files)
{
    this->files = files;
}

QStringList AnaConverter::getNewFiles() const
{
    return newFiles;
}

void AnaConverter::setDestinationFormat(const QString &format)
{
    this->format = format;
}

void AnaConverter::setTrimFiles(bool trim)
{
    trimFiles = trim;
}

void AnaConverter::setTargetFolder(const QString &folder)
{
    targetFolder = folder;
}

bool AnaConverter::convert()
{
    if (QThread::currentThread()->isInterruptionRequested()) return false;
    bool noErrors = true;
    newFiles.clear();

    //Converting

    QString folder = QFileInfo(files.first()).canonicalPath();
    QString folderName = folder.split("/").last();
    if (!targetFolder.isEmpty()) folder = targetFolder;

    QString destinationFileName = createUniqueFileName(folder, folderName, "", format);

    QList<AnaFile*> anaFiles;
    QVector<Channel*> anaChannels;

    //Сначала собираем каналы в один список
    for (const QString &fileName: files) {
        if (QThread::currentThread()->isInterruptionRequested()) return false;

        emit message("Читаю файл " + fileName);

        //reading file structure
        AnaFile *file = new AnaFile(fileName);
        file->read();
        anaFiles << file;
        anaChannels << file->channel(0);

        emit tick();
    }

    auto min = *(std::min_element(anaChannels.begin(), anaChannels.end(), [](Channel *a, Channel *b) {
        return a->data()->samplesCount() < b->data()->samplesCount();
    }));
    qint64 minCount = min->data()->samplesCount();
    bool truncate = !std::all_of(anaChannels.begin(), anaChannels.end(), [minCount](Channel *a)
    {
        return a->data()->samplesCount() == minCount;
    });
    truncate &= trimFiles;
    truncate |= format.toLower() == "dfd";

    //Затем сохраняем целевой файл
    emit message("Сохраняю итоговый файл. Не закрывайте это окно");

    QString fn = destinationFileName;
    QTemporaryFile temp;
    temp.setAutoRemove(true);
    if (truncate) {
        //Сохраненный файл мы должны обрезать по минимальной длине. Для этого:
        //1. сохраняем во временный файл
        //2. сохраняем временную вырезку
        //3. переименовываем временную вырезку в нужный нам файл
        temp.open();
        fn = temp.fileName();
        fn.append("."+format);
    }
    FileDescriptor *destinationFile = App->formatFactory->createDescriptor(anaChannels, fn);
    if (truncate) {
        emit message("Каналы имеют разную длину. Они будут обрезаны по самому короткому.");
        QString newFn = saveTimeSegment(destinationFile, 0, double(minCount) * anaChannels.first()->data()->xStep(), false);
        FileDescriptor *newFd = App->formatFactory->createDescriptor(newFn);
        newFd->read();
        newFd->rename(destinationFileName);
    }
    newFiles << destinationFileName;


    emit tick();
    emit message("Готово.");
    if (truncate) {
        //мы должны удалить сконвертирвоанный не обрезанный файл
        destinationFile->remove();
    }
    delete destinationFile;
    qDeleteAll(anaFiles);

    if (noErrors) emit message("<font color=blue>Конвертация закончена без ошибок.</font>");
    else emit message("<font color=red>Конвертация закончена с ошибками.</font>");
    emit finished();

    return true;
}
