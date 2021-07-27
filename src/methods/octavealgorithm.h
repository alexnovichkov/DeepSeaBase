#ifndef OCTAVEALGORITHM_H
#define OCTAVEALGORITHM_H

#include "abstractfunction.h"

class OctaveAlgorithm : public AbstractAlgorithm
{
public:
    OctaveAlgorithm(QList<FileDescriptor *> &dataBase, QObject *parent = nullptr);

public:
    virtual QString description() const override;
    virtual QString displayName() const override;
private:
    AbstractFunction * channelF = nullptr; //отбирает обрабатываемые каналы
    AbstractFunction * samplingF = nullptr; //осуществляет нарезку блоков
    AbstractFunction * averagingF = nullptr; //применяет усреднение
    AbstractFunction * octaveF = nullptr;
    AbstractFunction * saver = nullptr; //сохраняет результат

protected:
    virtual void resetChain() override;
    virtual void initChain(FileDescriptor *file) override;
};

#endif // OCTAVEALGORITHM_H
