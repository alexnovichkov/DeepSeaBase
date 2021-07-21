#ifndef FRFALGORITHM_H
#define FRFALGORITHM_H

#include "abstractfunction.h"

class FRFAlgorithm : public AbstractAlgorithm
{
public:
    FRFAlgorithm(QList<FileDescriptor *> &dataBase, QObject *parent = nullptr);

public:
    virtual QString description() const override;
    virtual QString displayName() const override;
private:
    AbstractFunction * channelF; //отдает данные для обрабатываемого канала
    AbstractFunction * refChannelF; //отдает данные для опорного канала

//    AbstractFunction * resamplingF; //изменяет частоту дискретизации
//    AbstractFunction * refResamplingF; //изменяет частоту дискретизации

    AbstractFunction * samplingF; //осуществляет нарезку блоков
    AbstractFunction * refSamplingF; //осуществляет нарезку блоков

    AbstractFunction * windowingF; //применяет оконную функцию
    AbstractFunction * refWindowingF; //применяет оконную функцию

    AbstractFunction * averagingF; //применяет усреднение
    AbstractFunction * refAveragingF; //применяет усреднение

    AbstractFunction * fftF; //вычисляет FFT
    AbstractFunction * refFftF; //вычисляет FFT

    AbstractFunction *apsF; //вычисляет S_AA или S_BB
    AbstractFunction *gxyF; //вычисляет S_AB или S_BA
    AbstractFunction *refGxyF; //вычисляет S_AB или S_BA

    AbstractFunction * frfF; //вычисляет FRF
    AbstractFunction * saver; //сохраняет результат

    //channelF    -> samplingF    -> windowingF    ->
    //refChannelF -> refSamplingF -> refWindowingF
protected:
    virtual void resetChain() override;
    virtual void initChain(FileDescriptor *file) override;
};

#endif // FRFALGORITHM_H
