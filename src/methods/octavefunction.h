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
    virtual QStringList parameters() const override;
    virtual QString m_parameterDescription(const QString &property) const override;
    virtual QString displayName() const override;
    virtual void updateParameter(const QString &parameter, const QVariant &val) override;

    virtual bool compute(FileDescriptor *file) override;
    virtual DataDescription getFunctionDescription() const override;
protected:
    virtual QVariant m_getParameter(const QString &property) const override;
    virtual void m_setParameter(const QString &property, const QVariant &val) override;
private:
//    OctaveType type = OctaveType::Octave3;
    OctaveFilterBank bank;
    int portionsCount = 0;
};

#endif // OCTAVEFUNCTION_H
