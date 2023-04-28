#ifndef DeepseaProcessor_H
#define DeepseaProcessor_H

#include <QObject>

#include "methods/dfdmethods/abstractmethod.h"

#include "algorithms.h"


class QProcess;
class AbstractMethod;
class DfdFileDescriptor;
class UffFileDescriptor;
class DfdChannel;

class DeepseaProcessor : public QObject
{
    Q_OBJECT
public:
    explicit DeepseaProcessor(const QList<FileDescriptor *> &base, const Parameters &p, QObject *parent = 0);
    virtual ~DeepseaProcessor();

    QStringList getNewFiles() const {return newFiles;}
signals:
    void tick();
    void tick(const QString &path);
    void finished();
    void message(const QString &s);
public slots:
    void stop();
    void start();
    void processTimer();
private:
//    bool convert(FileDescriptor *file, const QString &tempFolderName);
    void moveFilesFromTempDir(const QString &tempFolderName, const QString &destDir);
    QStringList getSpfFile(const QString &dir);
    void finalize();

    QList<FileDescriptor *> dataBase;
    Parameters p;
    bool stop_;

    QString tempFolderName;
    QDateTime dt;

    QStringList newFiles;
    QStringList newFiles_;

    QProcess *process;
};

#endif // DeepseaProcessor_H
