#ifndef PSDFUNCTION_H
#define PSDFUNCTION_H

#include "abstractfunction.h"
#include <QMap>

class Saver;

class SpectreAlgorithm : public AbstractAlgorithm
{
public:
    SpectreAlgorithm(QList<FileDescriptor *> &dataBase, QObject *parent = nullptr);

public:
    virtual QString name() const override;
    virtual QString description() const override;
private:
    AbstractFunction * channelF;
    AbstractFunction * filteringF;
    AbstractFunction * resamplingF;
    AbstractFunction * samplingF;
    AbstractFunction * windowingF;
    AbstractFunction * averagingF;
    AbstractFunction * fftF;
    AbstractFunction * saver;

public:
    virtual QString displayName() const override;
    virtual bool compute(FileDescriptor *file) override;
};

#endif // PSDFUNCTION_H
