#ifndef TIMEALGORITHM_H
#define TIMEALGORITHM_H

#include "abstractfunction.h"

class TimeAlgorithm : public AbstractAlgorithm
{
public:
    TimeAlgorithm(QList<FileDescriptor *> &dataBase, QObject *parent = nullptr);

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


#endif // TIMEALGORITHM_H
