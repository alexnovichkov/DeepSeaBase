#ifndef SPECTREALGORITHM_H
#define SPECTREALGORITHM_H

#include "abstractfunction.h"

class SpectreAlgorithm : public AbstractAlgorithm
{
public:
    SpectreAlgorithm(QList<FileDescriptor *> &dataBase, QObject *parent = nullptr);

public:
    virtual QString description() const override;
    virtual QString displayName() const override;
private:
    AbstractFunction * channelF; //фильтрует обрабатываемые каналы
//    AbstractFunction * filteringF; //применяет фильтр к временным данным
    AbstractFunction * resamplingF; //изменяет частоту дискретизации
    AbstractFunction * samplingF; //осуществляет нарезку блоков
    AbstractFunction * windowingF; //применяет оконную функцию
    AbstractFunction * averagingF; //применяет усреднение
    AbstractFunction * fftF; //вычисляет БПФ
    AbstractFunction * saver; //сохраняет результат

protected:
    virtual void resetChain() override;
    virtual void initChain(FileDescriptor *file) override;
};

#endif
