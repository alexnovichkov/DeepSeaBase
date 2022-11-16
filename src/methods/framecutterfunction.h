#ifndef SAMPLINGFUNCTION_H
#define SAMPLINGFUNCTION_H

#include "methods/abstractfunction.h"

#include "framecutter.h"

/* FrameCutter/type - Подряд / Перекрытие / Смещение / Триггер
 * FrameCutter/blockSize [string]
 * FrameCutter/percent - Перекрытие
 * FrameCutter/deltaTime - Время между блоками
 * FrameCutter/triggerMode - Больше чем | Меньше чем
 * FrameCutter/level - Уровень срабатывания триггера
 * FrameCutter/channel - Канал триггера
 * FrameCutter/pretrigger - Время до триггера, включаемое в блок
 *
 * Отдает:
 * ?/blockSize
 * ?/zCount
 * ?/zStep
 *
 * Спрашивает:
 *
 */
class FrameCutterFunction : public AbstractFunction
{
public:
    explicit FrameCutterFunction(QObject *parent = nullptr, const QString &name=QString());

    // AbstractFunction interface
public:
    virtual QString name() const override;
    virtual QString description() const override;
    virtual void updateParameter(const QString &property, const QVariant &val) override;
    virtual QStringList parameters() const override;
    virtual QString parameterDescription(const QString &property) const override;
    virtual DataDescription getFunctionDescription() const override;
protected:
    virtual QVariant m_getParameter(const QString &property) const override;
    virtual void m_setParameter(const QString &property, const QVariant &val) override;
    virtual bool m_parameterShowsFor(const QString &parameter) const override;
private:
    FrameCutter frameCutter;
    QMap<QString, QVariant> m_parameters; //хранит параметры в исходном виде. В frameCutter передаются уже пересчитанные значения

    // AbstractFunction interface
public:
    virtual QString displayName() const override;
    virtual void reset() override;
    virtual void resetData() override;
    virtual bool compute(FileDescriptor *file) override;
};

#endif // SAMPLINGFUNCTION_H
