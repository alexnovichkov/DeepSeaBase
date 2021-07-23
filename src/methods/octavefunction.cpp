#include "octavefunction.h"

#include "fileformats/filedescriptor.h"
#include "logging.h"

OctaveFunction::OctaveFunction(QObject *parent, const QString &name) :
    AbstractFunction(parent, name)
{DD;

}

QString OctaveFunction::name() const
{
    return "OCTF";
}

QString OctaveFunction::description() const
{
    return "Октавный спектр";
}

QStringList OctaveFunction::properties() const
{
    return {"type", "method", };
}

QString OctaveFunction::propertyDescription(const QString &property) const
{

}

QVariant OctaveFunction::m_getProperty(const QString &property) const
{

}

void OctaveFunction::m_setProperty(const QString &property, const QVariant &val)
{

}

QString OctaveFunction::displayName() const
{
    return description();
}

bool OctaveFunction::compute(FileDescriptor *file)
{

}

void OctaveFunction::reset()
{

}

DataDescription OctaveFunction::getFunctionDescription() const
{

}

