#ifndef TIMEMETHOD_H
#define TIMEMETHOD_H

#include <QWidget>
#include "abstractmethod.h"

class QComboBox;

class TimeMethod : public QWidget, public AbstractMethod
{
    Q_OBJECT
public:
    explicit TimeMethod(QList<DfdFileDescriptor *> &dataBase, QWidget *parent = 0);

    virtual int id();
    virtual QStringList methodSettings(DfdFileDescriptor *dfd, const Parameters &p);
//    virtual QStringList settings(DfdFileDescriptor *dfd, int bandStrip);
    virtual Parameters parameters();
    virtual QString methodDll();
    virtual int panelType();
    virtual QString methodName();
    virtual int dataType();
    virtual DescriptionList processData(const Parameters &p);
private:
//    QComboBox *rangeCombo;
    QComboBox *resolutionCombo;
//    QComboBox *windowCombo;
//    QComboBox *averCombo;
//    QComboBox *nAverCombo;
//    QComboBox *typeCombo;
    QComboBox *valuesCombo;
//    QComboBox *scaleCombo;
//    QComboBox *addProcCombo;

    double sampleRate;
    double bandWidth;

    // AbstractMethod interface
public:
    virtual DfdFileDescriptor *createNewDfdFile(const QString &fileName, DfdFileDescriptor *dfd, Parameters &p);
    virtual UffFileDescriptor *createNewUffFile(const QString &fileName, DfdFileDescriptor *dfd, Parameters &p);
    virtual DfdChannel *createDfdChannel(DfdFileDescriptor *newDfd, DfdFileDescriptor *dfd, const QVector<double> &spectrum, Parameters &p, int i);
    virtual Function *addUffChannel(UffFileDescriptor *newUff, DfdFileDescriptor *dfd, quint32 spectrumSize, Parameters &p, int i);
};

#endif // TIMEMETHOD_H
