#ifndef PSDALGORITHM_H
#define PSDALGORITHM_H

#include "abstractfunction.h"

class PsdAlgorithm : public AbstractAlgorithm
{
public:
    PsdAlgorithm(QList<FileDescriptor *> &dataBase, QObject *parent = nullptr);

public:
    virtual QString name() const override;
    virtual QString description() const override;
private:
    AbstractFunction * channelF; //фильтрует обрабатываемые каналы
//    AbstractFunction * filteringF; //применяет фильтр к временным данным
//    AbstractFunction * resamplingF; //изменяет частоту дискретизации
    AbstractFunction * samplingF; //осуществляет нарезку блоков
    AbstractFunction * windowingF; //применяет оконную функцию
    AbstractFunction * averagingF; //применяет усреднение
    AbstractFunction * psdF; //вычисляет БПФ
    AbstractFunction * saver; //сохраняет результат

public:
    virtual QString displayName() const override;
    virtual bool compute(FileDescriptor *file) override;
};

#endif
