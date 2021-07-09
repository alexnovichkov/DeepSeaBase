#ifndef WINDOWINGFUNCTION_H
#define WINDOWINGFUNCTION_H

#include "abstractfunction.h"
#include "windowing.h"

/* Windowing/type - Прямоугольное / Треугольное / Хеннинга / Хемминга /
 *                  Наттолла / Гаусса / Сила / Экспоненциальное / Тьюки
 * Windowing/parameter
 * Windowing/correction
 *
 * Отдает:
 * ?/windowDescription -
 * ?/windowType
 * ?/referenceWindowDescription
 * ?/referenceWindowType
 *
 * Спрашивает:
 *
 */
class WindowingFunction : public AbstractFunction
{

public:
    WindowingFunction(QObject *parent = nullptr, const QString &name=QString());

    // AbstractFunction interface
public:
    virtual QString name() const override;
    virtual QString description() const override;
    virtual QStringList properties() const override;
    virtual QString propertyDescription(const QString &property) const override;
    virtual QVariant m_getProperty(const QString &property) const override;
    virtual void m_setProperty(const QString &property, const QVariant &val) override;
    virtual bool propertyShowsFor(const QString &property) const override;
    virtual DataDescription getFunctionDescription() const override;

protected:
    Windowing windowing;

    // AbstractFunction interface
public:
    virtual QString displayName() const override;
    virtual bool compute(FileDescriptor *file) override;
    virtual void reset() override;
};

class RefWindowingFunction : public WindowingFunction
{

public:
    RefWindowingFunction(QObject *parent = nullptr, const QString &name=QString());
    virtual QString displayName() const override;
};

#endif // WINDOWINGFUNCTION_H
