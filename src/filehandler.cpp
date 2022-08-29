#include "filehandler.h"

#include <QDir>
#include <QFileInfo>
#include <QFileSystemWatcher>

#include "logging.h"

FileHandler::FileHandler(QObject *parent) : QObject(parent)
{DDD;
    watcher = new QFileSystemWatcher(this);
    connect(watcher, &QFileSystemWatcher::fileChanged, this, &FileHandler::fileChanged);
    connect(watcher, &QFileSystemWatcher::directoryChanged, this, &FileHandler::directoryChanged);
}

void FileHandler::trackFiles(const QStringList &fileNames)
{DDD;
    for (const QString &file: fileNames) {
        //1. Файл может уже отслеживаться
        //-либо как отдельный файл
        //-либо в составе папки
        if (!tracking(file)) {
            bool added = watcher->addPath(file);
            files.append({file, File, added});
            if (!added) qDebug() <<"tracking"<<file<<"failed";
        }
    }
}

void FileHandler::trackFolder(const QString &folder, bool withSubfolders)
{DDD;
    if (!tracking(folder)) {
        //может быть, эта папка вытеснит какие-то уже добавленные файлы и папки
        optimizeFiles(folder, withSubfolders);

        bool added = watcher->addPath(folder);
        files.append({folder, withSubfolders ? FolderWithSubfolders : Folder, added});
        if (!added) qDebug() <<"tracking"<<folder<<"failed";
    }
}

void FileHandler::untrackFile(const QString &fileName)
{DDD;
    const auto fi = QFileInfo(fileName);
    const QString path = fi.canonicalPath();

    //убирает файл из отслеживаемых, но не проверяет отслеживаемые папки
    for (int i=files.size()-1; i>=0; --i) {
        if (files.at(i).path == fileName && files.at(i).type == File)
            files.removeAt(i);
    }

    //TODO: добавить возможность отключать отслеживание файлов в папках,
    //то есть реализовать списки исключения
}

void FileHandler::untrack(const FileHandler::TrackedItem &item)
{DDD;
    files.removeAll(item);
}

void FileHandler::setFileNames(const QStringList &fileNames)
{DDD;
    clear();
    for (QString f: fileNames) {
        bool withSubfolders = false;
        if (f.endsWith(":1")) {
            f.chop(2);
            files.append({f, FolderWithSubfolders, true});
            withSubfolders = true;
        }
        else if (f.endsWith(":0")) {
            f.chop(2);
            files.append({f, Folder, true});
        }
        else {
            files.append({f, File, true});
        }
        if (!watcher->addPath(f)) {
            qDebug() <<"tracking"<<f<<"failed";
            files.last().good = false;
        }
        emit fileAdded(f, withSubfolders, true);
    }
}

QStringList FileHandler::fileNames() const
{DDD;
    QStringList result;
    for (auto item: files) {
        if (item.type==FolderWithSubfolders) item.path.append(":1");
        else if (item.type==Folder) item.path.append(":0");
        result << item.path;
    }

    return result;
}

void FileHandler::clear()
{DDD;
    files.clear();
}

bool FileHandler::tracking(const QString &file) const
{DDD;
    const auto fi = QFileInfo(file);
    const QString path = fi.canonicalPath()+"/";

    for (const auto &item: qAsConst(files)) {
        switch (item.type) {
            case File:
            case Folder:
                if (item.path == file) return true;
                break;
            case FolderWithSubfolders: if (path.startsWith(item.path+"/")) return true;
        }
    }

    return false;
}

void FileHandler::optimizeFiles(const QString &folder, bool withSubfolders)
{DDD;
    //Удаляем из списка все файлы и папки, которые находятся в папке folder
    for(int i=files.size()-1; i>=0; --i) {
        auto &f = files.at(i);
        QFileInfo fi(f.path);
        if (f.type == File) {
            if (fi.canonicalPath() == folder) files.removeAt(i);
            else if (withSubfolders && fi.canonicalPath().startsWith(folder+"/")) files.removeAt(i);
        }
        else if (withSubfolders && f.path.startsWith(folder+"/")) files.removeAt(i);
    }
}

void FileHandler::fileChanged(const QString &file)
{DDD;
    qDebug()<<"Changed"<<file;
    if (!QFileInfo::exists(file)) {
        emit fileDeleted(file);
    }
}

void FileHandler::directoryChanged(const QString &dir)
{DDD;
    Q_UNUSED(dir);
}
