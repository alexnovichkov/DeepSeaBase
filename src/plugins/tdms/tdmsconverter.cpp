#include "tdmsconverter.h"

#include "logging.h"
#include "algorithms.h"
#include "fileformats/filedescriptor.h"
#include "fileformats/abstractformatfactory.h"
#include "tdmsfile.h"

TDMSFileConverter::TDMSFileConverter(AbstractFormatFactory *factory, QObject *parent) : QObject(parent), factory(factory)
{DDD;

}

bool TDMSFileConverter::convert()
{DDD;
    if (QThread::currentThread()->isInterruptionRequested()) return false;
    bool noErrors = true;

    //Converting
    for (const QString &tdmsFileName: filesToConvert) {
        if (QThread::currentThread()->isInterruptionRequested()) return false;

        emit message("Конвертируем файл " + tdmsFileName);

        //reading tdms file structure
        TDMSFile tdmsFile(tdmsFileName);
        if (!tdmsFile.isValid()) {
            emit message("Ошибка: Файл поврежден и будет пропущен!");
            emit tick();
            noErrors = false;
            continue;
        }
        emit message(QString("-- Файл TDMS содержит %1 переменных").arg(tdmsFile.groups.size()));

        //ищем группу каналов, которая содержит все данные
        TDMSGroup *group = 0;
        for (TDMSGroup *g: tdmsFile.groups) {
            if (g->properties.value("DecimationLevel").toString() == "0") {
                group = g;
                break;
            }
        }
        if (!group && tdmsFile.groups.size()>0) group = tdmsFile.groups.first();
        if (!group) {
            emit message("В файле отсутствует нужная группа каналов");
            emit tick();
            continue;
        }

        QString destinationFileName = changeFileExt(tdmsFileName, destinationFormat);

        QList<FileDescriptor *> destinationFiles = factory->createDescriptors(*group,
                                                                          destinationFileName);
        if (!destinationFiles.isEmpty())
            for (auto f : destinationFiles) {
                newFiles << f->fileName();
            }

        for (auto f : destinationFiles) delete f;

        emit message("Готово.");
        emit tick();
    }

    if (noErrors) emit message("<font color=blue>Конвертация закончена без ошибок.</font>");
    else emit message("<font color=red>Конвертация закончена с ошибками.</font>");
    emit finished();

    return true;
}

