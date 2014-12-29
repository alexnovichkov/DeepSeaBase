#ifndef CONVERTER_H
#define CONVERTER_H

#include <QObject>

#include "methods/abstractmethod.h"

class QProcess;
class AbstractMethod;

class Converter : public QObject
{
    Q_OBJECT
public:
    explicit Converter(QList<DfdFileDescriptor *> &base, const Parameters &p, QObject *parent = 0);
    virtual ~Converter();

    QStringList getNewFiles() const {return newFiles;}
signals:
    void tick();
    void tick(const QString &path);
    void finished();
    void message(const QString &s);
public slots:
    void stop();
    void start();
private:
    bool convert(DfdFileDescriptor *dfd, const QString &tempFolderName);
    void applyWindow(QVector<float> &values, const Parameters &p);
    QVector<double> computeSpectre(const QVector<float> &values, const Parameters &p);
    void moveFilesFromTempDir(const QString &tempFolderName, QString destDir);
    QStringList getSpfFile(QString dir);
    void finalize();

    QList<DfdFileDescriptor *> dataBase;
    Parameters p;
    bool stop_;

    QString tempFolderName;
    QDateTime dt;

    QStringList newFiles;
    QStringList newFiles_;

    QProcess *process;
    AbstractMethod *method;
};

#endif // CONVERTER_H
