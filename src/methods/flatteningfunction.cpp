#include "flatteningfunction.h"

FlatteningFunction::FlatteningFunction(QObject *parent, const QString &name)
    : AbstractFunction(parent, name)
{

}


QString FlatteningFunction::name() const
{
    return "Flattener";
}

QString FlatteningFunction::displayName() const
{
    return "Выпрямитель данных";
}

QString FlatteningFunction::description() const
{
    return "Объединение блоков данных";
}

QStringList FlatteningFunction::properties() const
{
    return QStringList();
}

QString FlatteningFunction::propertyDescription(const QString &property) const
{
    Q_UNUSED(property);
    return "";
}

bool FlatteningFunction::compute(FileDescriptor *file)
{
    if (!m_input) return false;
    output.clear();
    if (!m_input->compute(file)) {
        return false;
    }
    output = m_input->getData("input");
    return !output.isEmpty();
}

QVariant FlatteningFunction::m_getProperty(const QString &property) const
{
    if (property == "?/zCount") {
        //количество блоков
        return 1;
    }
    // do not know anything about these broadcast properties
    if (m_input) return m_input->getParameter(property);
    return QVariant();
}

void FlatteningFunction::m_setProperty(const QString &property, const QVariant &val)
{
    Q_UNUSED(property);
    Q_UNUSED(val);
}
