#ifndef WINDOWINGALGORITHM_H
#define WINDOWINGALGORITHM_H

#include "abstractfunction.h"

class WindowingAlgorithm : public AbstractAlgorithm
{
public:
    WindowingAlgorithm(QList<FileDescriptor *> &dataBase, QObject *parent = nullptr);

public:
    virtual QString description() const override;
    virtual QString displayName() const override;
private:
    AbstractFunction * channelF; //фильтрует обрабатываемые каналы
    AbstractFunction * samplingF; //осуществляет нарезку блоков
    AbstractFunction * windowingF; //применяет оконную функцию
    AbstractFunction * saver; //сохраняет результат

protected:
    virtual void resetChain() override;
    virtual void initChain(FileDescriptor *file) override;
};

#endif // WINDOWINGALGORITHM_H
