#ifndef PSALGORITHM_H
#define PSALGORITHM_H

#include "abstractfunction.h"

class PsAlgorithm : public AbstractAlgorithm
{
public:
    PsAlgorithm(QList<FileDescriptor *> &dataBase, QObject *parent = nullptr);

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
    AbstractFunction * psF; //вычисляет БПФ
    AbstractFunction * saver; //сохраняет результат

public:
    virtual QString displayName() const override;
    virtual bool compute(FileDescriptor *file) override;
};

#endif
