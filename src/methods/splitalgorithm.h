#ifndef SPLITALGORITHM_H
#define SPLITALGORITHM_H

#include "abstractfunction.h"

class SplitAlgorithm : public AbstractAlgorithm
{
public:
    SplitAlgorithm(QList<FileDescriptor *> &dataBase, QObject *parent = nullptr);

public:
    virtual QString description() const override;
    virtual QString displayName() const override;
private:
    AbstractFunction * channelF; //фильтрует обрабатываемые каналы
    AbstractFunction * splitF; //изменяет частоту дискретизации
    AbstractFunction * saver; //сохраняет результат

protected:
    virtual void resetChain() override;
    virtual void initChain(FileDescriptor *file) override;
};

#endif // SPLITALGORITHM_H
