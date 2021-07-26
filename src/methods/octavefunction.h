#ifndef OCTAVEFUNCTION_H
#define OCTAVEFUNCTION_H

#include "abstractfunction.h"
#include <QMap>
#include <QVector>
#include "octavefilterbank.h"

class OctaveFunction : public AbstractFunction
{
public:
    OctaveFunction(QObject *parent = nullptr, const QString &name=QString());

    // AbstractFunction interface
public:
    virtual QString name() const override;
    virtual QString description() const override;
    virtual QStringList properties() const override;
    virtual QString propertyDescription(const QString &property) const override;
    virtual QVariant m_getProperty(const QString &property) const override;
    virtual void m_setProperty(const QString &property, const QVariant &val) override;
    virtual QString displayName() const override;

    virtual bool compute(FileDescriptor *file) override;
    virtual void reset() override;
    virtual DataDescription getFunctionDescription() const override;
private:
//    OctaveType type = OctaveType::Octave3;
    OctaveFilterBank bank;
    int portionsCount = 0;

    // AbstractFunction interface
public slots:
    virtual void updateProperty(const QString &property, const QVariant &val) override;
};

#endif // OCTAVEFUNCTION_H
