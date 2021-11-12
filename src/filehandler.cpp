#include "filehandler.h"

#include <QDir>
#include <QFileInfo>

#include "logging.h"

FileHandler::FileHandler(QObject *parent) : QObject(parent)
{DD;

}

void FileHandler::trackFiles(const QStringList &fileNames)
{DD;
    for (const QString &file: fileNames) {
        //1. Файл может уже отслеживаться
        //-либо как отдельный файл
        //-либо в составе папки
        if (!tracking(file))
            files.append({file,File});
    }
}

void FileHandler::trackFolder(const QString &folder, bool withSubfolders)
{DD;
    if (!tracking(folder))
        files.append({folder, withSubfolders ? FolderWithSubfolders : Folder});
}

void FileHandler::untrackFile(const QString &fileName)
{DD;
    const auto fi = QFileInfo(fileName);
    const QString path = fi.canonicalPath();

    //убирает файл из отслеживаемых, но не проверяет отслеживаемые папки
    files.removeAll({fileName, File});

    //TODO: добавить возможность отключать отслеживание файлов в папках,
    //то есть реализовать списки исключения
}

void FileHandler::setFileNames(const QStringList &fileNames)
{DD;
    clear();
    for (QString f: fileNames) {
        bool withSubfolders = false;
        if (f.endsWith(":1")) {
            f.chop(2);
            files.append({f, FolderWithSubfolders});
            withSubfolders = true;
        }
        else if (f.endsWith(":0")) {
            f.chop(2);
            files.append({f, Folder});
        }
        else {
            files.append({f, File});
        }
        emit fileAdded(f,withSubfolders,true);
    }
}

QStringList FileHandler::fileNames() const
{DD;
    QStringList result;
    for (auto item: files) {
        if (item.second==FolderWithSubfolders) item.first.append(":1");
        else if (item.second==Folder) item.first.append(":0");
        result << item.first;
    }

    return result;
}

void FileHandler::clear()
{DD;
    files.clear();
}

bool FileHandler::tracking(const QString &file) const
{DD;
    const auto fi = QFileInfo(file);
    const QString path = fi.canonicalPath();

    for (const auto &item: qAsConst(files)) {
        if (item.second==FolderWithSubfolders) {
            if (path.startsWith(item.first+"/")) return true;
        }
        else if (item.second == Folder) {
            if (item.first == path) return true;
        }
        else {
            if (item.first == file) return true;
        }
    }

    return false;
}
