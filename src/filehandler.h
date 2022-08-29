#ifndef FILEHANDLER_H
#define FILEHANDLER_H

#include <QObject>
#include <QVector>
#include <QPair>

class QFileSystemWatcher;

class FileHandler : public QObject
{
    Q_OBJECT
public:
    enum FileType {
        File,
        Folder,
        FolderWithSubfolders
    };
    class TrackedItem {
    public:
        QString path;
        FileType type;
        bool good = false;
        bool operator==(const TrackedItem &other) const {
            return (path == other.path && type == other.type);
        }
    };

    //using Item = QPair<QString, FileType>;
    explicit FileHandler(QObject *parent = nullptr);
    void trackFiles(const QStringList &fileNames);
    void trackFolder(const QString &folder, bool withSubfolders);
    void untrackFile(const QString &fileName);
    void untrack(const TrackedItem &item);
    void setFileNames(const QStringList &fileNames);
    void clear();

    int count() const {return files.size();}
    TrackedItem item(int index) const {return files.at(index);}

    QStringList fileNames() const;
    bool tracking(const QString &file) const;
signals:
    void fileAdded(const QString &fileName);
    void fileAdded(const QString &fileName, bool withSubfolders, bool silent);
    void filesAdded(const QStringList &fileNames);
    void fileRemoved(const QString &fileName);
    void filesRemoved(const QStringList &fileNames);

    //emits when file is deleted from the drive
    void fileDeleted(const QString &file);
public:
    QVector<TrackedItem> files;
private:
    void optimizeFiles(const QString &folder, bool withSubfolders);
    void fileChanged(const QString &file);
    void directoryChanged(const QString &dir);

    QFileSystemWatcher *watcher = nullptr;
};

#endif // FILEHANDLER_H
