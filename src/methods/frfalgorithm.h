#ifndef FRFALGORITHM_H
#define FRFALGORITHM_H

#include "abstractfunction.h"

class FRFAlgorithm : public AbstractAlgorithm
{
public:
    FRFAlgorithm(QList<FileDescriptor *> &dataBase, QObject *parent = nullptr);

public:
    virtual QString name() const override;
    virtual QString description() const override;
private:
    AbstractFunction * channelF; //отдает данные для обрабатываемого канала
    AbstractFunction * refChannelF; //отдает данные для опорного канала
    AbstractFunction * resamplingF; //изменяет частоту дискретизации
    AbstractFunction * refResamplingF; //изменяет частоту дискретизации
    AbstractFunction * samplingF; //осуществляет нарезку блоков
    //AbstractFunction * refSamplingF; //осуществляет нарезку блоков
    AbstractFunction * windowingF; //применяет оконную функцию
    AbstractFunction * refWindowingF; //применяет оконную функцию
    AbstractFunction * averagingF; //применяет усреднение
    AbstractFunction * fftF; //вычисляет FFT
    AbstractFunction * frfF; //вычисляет FRF
    AbstractFunction * saver; //сохраняет результат

    //channelF    -> resamplingF    -> samplingF    -> windowingF    ->
    //refChannelF -> refResamplingF -> refSamplingF -> refWindowingF

public:
    virtual QString displayName() const override;
    virtual bool compute(FileDescriptor *file) override;
};

#endif // FRFALGORITHM_H
