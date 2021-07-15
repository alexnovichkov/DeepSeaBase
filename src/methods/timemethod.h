#ifndef TIMEMETHOD_H
#define TIMEMETHOD_H

#include <QWidget>
#include "abstractmethod.h"

class QComboBox;
class QSlider;
class QLabel;

class TimeMethod : public QWidget, public AbstractMethod
{
    Q_OBJECT
public:
    explicit TimeMethod(QList<FileDescriptor *> &dataBase, QWidget *parent = 0);

    virtual int id();
    virtual QStringList methodSettings(FileDescriptor *dfd, const Parameters &p);
    virtual Parameters parameters();
    virtual QString methodDll();
    virtual int panelType();
    virtual QString methodName();
    virtual int dataType();
    virtual DescriptionList processData(const Parameters &p);
private:
    QComboBox *resolutionCombo;
    QComboBox *valuesCombo;
    QSlider *minTimeSlider;
    QSlider *maxTimeSlider;
    QLabel *minTimeLabel;
    QLabel *maxTimeLabel;

    double sampleRate;
    double bandWidth;
    double xStep;

    // AbstractMethod interface
public:
//    virtual DfdFileDescriptor *createNewDfdFile(const QString &fileName, FileDescriptor *dfd, Parameters &p);
//    virtual UffFileDescriptor *createNewUffFile(const QString &fileName, FileDescriptor *dfd, Parameters &p);
//    virtual Channel *createDfdChannel(DfdFileDescriptor *newDfd, FileDescriptor *dfd, const QVector<double> &spectrum, Parameters &p, int i);
//    virtual Channel *addUffChannel(UffFileDescriptor *newUff, FileDescriptor *dfd, int spectrumSize, Parameters &p, int i);
};

#endif // TIMEMETHOD_H
