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
QVector<double> powerSpectre(const QVector<float> &values, const Parameters &p);
QVector<double> autoSpectre(const QVector<float> &values, const Parameters &p);
QVector<double> coSpectre(const QVector<float> &values1, const QVector<float> &values2, const Parameters &p);
QVector<QPair<double, double> > coSpectreComplex(const QVector<float> &values1, const QVector<float> &values2, const Parameters &p);
QVector<double> transferFunctionH1(const QVector<double> &values1, const QVector<double> &values2, const Parameters &p);
QVector<QPair<double, double> > transferFunctionH1Complex(const QVector<double> &values1,
                                                          const QVector<QPair<double, double> > &values2, const Parameters &p);
void changeScale(QVector<double> &output, const Parameters &p);
void average(QVector<double> &result, const QVector<double> &input, const Parameters &p, int averagesMade);

class Converter : public QObject
{
    Q_OBJECT
public:
    explicit Converter(QList<DfdFileDescriptor *> &base, const Parameters &p, QObject *parent = 0);
    virtual ~Converter();

    QStringList getNewFiles() const {return newFiles;}
    //DfdChannel * addDfdChannel(DfdFileDescriptor *dfd, QVector<double> spectrum, int i, DfdFileDescriptor *newDfd);
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
    AbstractMethod *method;
};

#endif // CONVERTER_H
