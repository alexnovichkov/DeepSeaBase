#ifndef CONVERTER_H
#define CONVERTER_H

#include <QObject>

#include "methods/abstractmethod.h"

class QProcess;
class AbstractMethod;
class DfdFileDescriptor;
class UffFileDescriptor;
class DfdChannel;

void applyWindow(QVector<float> &values, const Parameters &p);
QVector<double> FFTAnalysis(const QVector<float> &AVal);
QVector<double> powerSpectre(const QVector<float> &values, int bufferSize, int outputSize);
QVector<double> autoSpectre(const QVector<float> &values, int bufferSize, int outputSize);
QVector<double> coSpectre(const QVector<float> &values1, const QVector<float> &values2, int bufferSize, int outputSize);
QVector<QPair<double, double> > coSpectreComplex(const QVector<float> &values1, const QVector<float> &values2, int bufferSize, int outputSize);
QVector<double> transferFunctionH1(const QVector<double> &values1, const QVector<double> &values2);
QVector<QPair<double, double> > transferFunctionH1Complex(const QVector<double> &values1,
                                                          const QVector<QPair<double, double> > &values2);
void changeScale(QVector<double> &output, const Parameters &p);

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
    void processTimer();
private:
    bool convert(DfdFileDescriptor *dfd, const QString &tempFolderName);
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
};

#endif // CONVERTER_H
