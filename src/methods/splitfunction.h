#ifndef SPLITFUNTCION_H
#define SPLITFUNTCION_H

#include "abstractfunction.h"

/**
 * @brief The SplitFunction class
 * Осуществляет разбиение файла на отдельные блоки
 */

/* Resampling/resampleType - Коэффициент / Частотный диапазон / Частота дискретизации
 * Resampling/factor - Коэффициент
 * Resampling/frequencyRange - Частотный диапазон
 * Resampling/sampleRate - Частота дискретизации
 *
 * Отдает:
 * ?/sampleRate
 * ?/functionDescription = RSMPL
 * ?/functionType = 1 (Time response)
 * ?/dataType = 1 (Cutted data)
 * ?/processData - для dfd
 * ?/xStep - пересчитывает
 *
 * Спрашивает:
 *
 */

class SplitFunction : public AbstractFunction
{
public:
    SplitFunction(QObject *parent = nullptr, const QString &name=QString());

    // AbstractFunction interface
public:
    virtual QString name() const override;
    virtual QString description() const override;
    virtual QStringList parameters() const override;
    virtual QString m_parameterDescription(const QString &property) const override;
protected:
    virtual QVariant m_getParameter(const QString &property) const override;
    virtual void m_setParameter(const QString &property, const QVariant &val) override;
    virtual bool m_parameterShowsFor(const QString &parameter) const override;
private:
    int blockSize = 0;
    int sampleRate = 1;
    int blocksCount = 0;

    // AbstractFunction interface
public:
    virtual QString displayName() const override;
    virtual bool compute(FileDescriptor *file) override;
    virtual DataDescription getFunctionDescription() const override;
};

#endif // SPLITFUNTCION_H
