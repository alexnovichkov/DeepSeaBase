#ifndef FILTERINGALGORITHM_H
#define FILTERINGALGORITHM_H

#include "abstractfunction.h"

class FilteringAlgorithm : public AbstractAlgorithm
{
public:
    FilteringAlgorithm(QList<FileDescriptor *> &dataBase, QObject *parent = nullptr);

    virtual QString description() const override;
    virtual QString displayName() const override;
private:
    AbstractFunction * channelF; //фильтрует обрабатываемые каналы
    AbstractFunction * filteringF; //применяет фильтр к временным данным
    AbstractFunction * saver; //сохраняет результат
protected:
    virtual void resetChain() override;
    virtual void initChain(FileDescriptor *file) override;
};

#endif // FILTERINGALGORITHM_H
