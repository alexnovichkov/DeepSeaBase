#ifndef FILEHANDLER_H
#define FILEHANDLER_H

#include <QObject>
#include <QVector>
#include <QPair>

class FileHandler : public QObject
{
    Q_OBJECT
public:
    enum FileType {
        File,
        Folder,
        FolderWithSubfolders
    };
    using Item = QPair<QString, FileType>;
    explicit FileHandler(QObject *parent = nullptr);
    void trackFiles(const QStringList &fileNames);
    void trackFolder(const QString &folder, bool withSubfolders);
    void untrackFile(const QString &fileName);
    void setFileNames(const QStringList &fileNames);
    void clear();

    QStringList fileNames() const;
    bool tracking(const QString &file) const;
signals:
    void fileAdded(const QString &fileName);
    void fileAdded(const QString &fileName, bool withSubfolders, bool silent);
    void filesAdded(const QStringList &fileNames);
    void fileRemoved(const QString &fileName);
    void filesRemoved(const QStringList &fileNames);
public:
    QVector<Item> files;
};

#endif // FILEHANDLER_H
