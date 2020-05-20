#ifndef PSDFUNCTION_H
#define PSDFUNCTION_H

#include "abstractfunction.h"

class Saver;

class SpectreAlgorithm : public AbstractAlgorithm
{
public:
    SpectreAlgorithm(QList<FileDescriptor *> &dataBase, QObject *parent = nullptr);

public:
    virtual QString name() const override;
    virtual QString description() const override;
private:
    AbstractFunction * channelF; //фильтрует обрабатываемые каналы
    AbstractFunction * filteringF; //применяет фильтр к временным данным
    AbstractFunction * resamplingF; //изменяет частоту дискретизации
    AbstractFunction * samplingF; //осуществляет нарезку блоков
    AbstractFunction * windowingF; //применяет оконную функцию
    AbstractFunction * averagingF; //применяет усреднение
    AbstractFunction * fftF; //вычисляет БПФ
    AbstractFunction * saver; //сохраняет результат

public:
    virtual QString displayName() const override;
    virtual bool compute(FileDescriptor *file) override;
};

#endif // PSDFUNCTION_H
