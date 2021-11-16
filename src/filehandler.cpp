#include "filehandler.h"

#include <QDir>
#include <QFileInfo>
#include <QFileSystemWatcher>

#include "logging.h"

FileHandler::FileHandler(QObject *parent) : QObject(parent)
{DD;
    watcher = new QFileSystemWatcher(this);
    connect(watcher, &QFileSystemWatcher::fileChanged, this, &FileHandler::fileChanged);
    connect(watcher, &QFileSystemWatcher::directoryChanged, this, &FileHandler::directoryChanged);
}

void FileHandler::trackFiles(const QStringList &fileNames)
{DD;
    for (const QString &file: fileNames) {
        //1. Файл может уже отслеживаться
        //-либо как отдельный файл
        //-либо в составе папки
        if (!tracking(file)) {
            files.append({file,File});
            if (!watcher->addPath(file)) qDebug() <<"tracking"<<file<<"failed";
        }
    }
}

void FileHandler::trackFolder(const QString &folder, bool withSubfolders)
{DD;
    if (!tracking(folder)) {
        //может быть, эта папка вытеснит какие-то уже добавленные файлы и папки
        optimizeFiles(folder, withSubfolders);
        files.append({folder, withSubfolders ? FolderWithSubfolders : Folder});
        if (!watcher->addPath(folder)) qDebug() <<"tracking"<<folder<<"failed";
    }
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
        if (!watcher->addPath(f)) qDebug() <<"tracking"<<f<<"failed";
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
    const QString path = fi.canonicalPath()+"/";

    for (const auto &item: qAsConst(files)) {
        switch (item.second) {
            case File:
            case Folder:
                if (item.first == file) return true;
            case FolderWithSubfolders: if (path.startsWith(item.first+"/")) return true;
        }
    }

    return false;
}

void FileHandler::optimizeFiles(const QString &folder, bool withSubfolders)
{
    //Удаляем из списка все файлы и папки, которые находятся в папке folder
    for(int i=files.size()-1; i>=0; --i) {
        auto &f = files.at(i);
        QFileInfo fi(f.first);
        if (f.second == File) {
            if (fi.canonicalPath() == folder) files.removeAt(i);
            else if (withSubfolders && fi.canonicalPath().startsWith(folder+"/")) files.removeAt(i);
        }
        else if (withSubfolders && f.first.startsWith(folder+"/")) files.removeAt(i);
    }
}

void FileHandler::fileChanged(const QString &file)
{
    qDebug()<<"Changed"<<file;
    if (!QFileInfo::exists(file)) {
        emit fileDeleted(file);
    }
}

void FileHandler::directoryChanged(const QString &dir)
{

}
