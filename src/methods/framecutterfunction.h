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
    virtual QVariant m_getProperty(const QString &property) const override;
    virtual void m_setProperty(const QString &property, const QVariant &val) override;
    virtual bool propertyShowsFor(const QString &property) const override;

    virtual QStringList properties() const override;
    virtual QString propertyDescription(const QString &property) const override;
private:
    FrameCutter frameCutter;
    QMap<QString, QVariant> parameters;
    QVector<double> output;

    // AbstractFunction interface
public:
    virtual QString displayName() const override;
    virtual void reset() override;
    virtual void resetData() override;
    virtual QVector<double> getData(const QString &id) override;
    virtual bool compute(FileDescriptor *file) override;

    // AbstractFunction interface
public slots:
    virtual void updateProperty(const QString &property, const QVariant &val) override;
};

#endif // SAMPLINGFUNCTION_H
