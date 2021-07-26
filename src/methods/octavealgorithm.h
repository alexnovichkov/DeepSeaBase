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
    AbstractFunction * channelF; //отбирает обрабатываемые каналы
    AbstractFunction * samplingF; //осуществляет нарезку блоков
    AbstractFunction * averagingF; //применяет усреднение
    AbstractFunction * octaveF;
    AbstractFunction * saver; //сохраняет результат

protected:
    virtual void resetChain() override;
    virtual void initChain(FileDescriptor *file) override;
};

#endif // OCTAVEALGORITHM_H
