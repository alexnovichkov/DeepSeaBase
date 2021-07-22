#ifndef RESAMPLINGALGORITHM_H
#define RESAMPLINGALGORITHM_H

#include "abstractfunction.h"

class ResamplingAlgorithm : public AbstractAlgorithm
{
public:
    ResamplingAlgorithm(QList<FileDescriptor *> &dataBase, QObject *parent = nullptr);

public:
    virtual QString description() const override;
    virtual QString displayName() const override;
private:
    AbstractFunction * channelF; //фильтрует обрабатываемые каналы
    AbstractFunction * resamplingF; //изменяет частоту дискретизации
    AbstractFunction * saver; //сохраняет результат

protected:
    virtual void resetChain() override;
    virtual void initChain(FileDescriptor *file) override;
};


#endif // RESAMPLINGALGORITHM_H
