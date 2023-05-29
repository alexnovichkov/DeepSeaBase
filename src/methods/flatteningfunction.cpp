#include "flatteningfunction.h"
#include "logging.h"

FlatteningFunction::FlatteningFunction(QObject *parent, const QString &name)
    : AbstractFunction(parent, name)
{DD0;

}


QString FlatteningFunction::name() const
{DD;
    return "Flattener";
}

QString FlatteningFunction::displayName() const
{DD0;
    return "Выпрямитель данных";
}

QString FlatteningFunction::description() const
{DD0;
    return "Объединение блоков данных";
}

QStringList FlatteningFunction::parameters() const
{DD0;
    return QStringList();
}

QString FlatteningFunction::m_parameterDescription(const QString &property) const
{DD0;
    Q_UNUSED(property);
    return "";
}

bool FlatteningFunction::compute(FileDescriptor *file)
{DD0;
    if (!m_input) return false;
    LOG(INFO) << QString("Запуск расчета для функции объединения блоков данных");
    output.clear();
    if (!m_input->compute(file)) {
        return false;
    }
    output = m_input->getData("input");
    return !output.isEmpty();
}

QVariant FlatteningFunction::m_getParameter(const QString &property) const
{DD0;
    if (property == "?/zCount") {
        //количество блоков
        return 1;
    }
    // do not know anything about these broadcast properties
    if (m_input) return m_input->getParameter(property);
    return QVariant();
}

void FlatteningFunction::m_setParameter(const QString &property, const QVariant &val)
{DD0;
    Q_UNUSED(property);
    Q_UNUSED(val);
}
