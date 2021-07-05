#include "abstractfunction.h"

#include <QtCore>
#include "fileformats/filedescriptor.h"
#include "logging.h"

AbstractFunction::AbstractFunction(QObject *parent, const QString &name) : QObject(parent), _name(name)
{

}

AbstractFunction::~AbstractFunction()
{
}

QString AbstractFunction::propertiesDescription() const
{DD;
    QString result="[";
    const QStringList props = properties();
    for (const QString &p: props) {
        result.append(propertyDescription(p));
        result.append(",");
    }
    result.chop(1);
    result.append("]");
    return result;
}

bool AbstractFunction::propertyShowsFor(const QString &property) const
{DD;
    Q_UNUSED(property);

    return true;
}

QVariant AbstractFunction::getProperty(const QString &property) const
{DD;
    //qDebug()<<debugName()<<"gets"<<property;
    if (m_master != nullptr) {
        //qDebug()<<debugName()<<"gets"<<property<<"from"<<m_master->debugName();
        QVariant p = m_master->m_getProperty(property);
        //qDebug()<<property<<"="<<p;
        return p;
    }
    QVariant p = m_getProperty(property);
    //qDebug()<<property<<"="<<p;
    return p;
}

void AbstractFunction::setProperty(const QString &property, const QVariant &val)
{DD;
    //qDebug()<<debugName()<<"sets"<<property<<"="<<val;
    if (m_slave != nullptr) {
        //qDebug()<<debugName()<<"sets"<<property<<"for"<<m_slave->debugName();
        m_slave->m_setProperty(property, val);
    }

    m_setProperty(property, val);
}

void AbstractFunction::pairWith(AbstractFunction *slave)
{DD;
    if (slave != nullptr) {
        m_slave = slave;
        slave->m_master = this;
    };
}

QVector<double> AbstractFunction::getData(const QString &id)
{
    if (id == "input") return output;

    return QVector<double>();
}

void AbstractFunction::setInput(AbstractFunction *input)
{DD;
    m_input = input;
}

void AbstractFunction::setInput2(AbstractFunction *input)
{DD;
    m_input2 = input;
}

void AbstractFunction::setFile(FileDescriptor *file)
{DD;
    m_file = file;
    if (m_input) m_input->setFile(file);
    if (m_input2) m_input2->setFile(file);
}

void AbstractFunction::reset()
{DD;
    // no-op
}

void AbstractFunction::resetData()
{
    if (m_input) m_input->resetData();
    if (m_input2) m_input2->resetData();
}

void AbstractFunction::updateProperty(const QString &property, const QVariant &val)
{DD;
    Q_UNUSED(property);
    Q_UNUSED(val);
    //no-op
}




AbstractAlgorithm::AbstractAlgorithm(QList<FileDescriptor *> &dataBase, QObject *parent) : QObject(parent),
    m_dataBase(dataBase)
{DD;

}

AbstractAlgorithm::~AbstractAlgorithm()
{DD;
    m_functions.clear();
}

bool AbstractAlgorithm::propertyShowsFor(const QString &property) const
{DD;
    if (property.isEmpty()) return true;

    for (AbstractFunction *f: functions()) {
        if (property.startsWith(f->name()+"/")) {
            return f->propertyShowsFor(property);
        }
    }

    return true;
}

QVariant AbstractAlgorithm::getProperty(const QString &property) const
{DD;
    if (property.isEmpty()) return QVariant();

    for (AbstractFunction *f: functions()) {
        if (property.startsWith(f->name()+"/")) {
            return f->getProperty(property);
        }
    }

    return QVariant();
}

void AbstractAlgorithm::setProperty(const QString &property, const QVariant &val)
{DD;
    if (property.isEmpty()) return;

    for (AbstractFunction *f: functions()) {
        if (property.startsWith(f->name()+"/")) {
            f->setProperty(property, val);
            return;
        }
    }
}

void AbstractAlgorithm::reset()
{DD;
    // no-op
}

void AbstractAlgorithm::start()
{DD;
    auto dt = QDateTime::currentDateTime();
    qDebug()<<"Start converting"<<dt.time();
    emit message(QString("Запуск расчета: %1").arg(dt.time().toString()));

//    QDir d;
//    if (!d.exists("C:/DeepSeaBase-temp"))  d.mkdir("C:/DeepSeaBase-temp");

//    QTemporaryDir tempDir("C:\\DeepSeaBase-temp\\temp-XXXXXX");
//    tempDir.setAutoRemove(true);
//    tempFolderName = tempDir.path();



    for (FileDescriptor *file: m_dataBase) {
        emit message(QString("Расчет для файла\n%1").arg(file->fileName()));
        if (!compute(file)) {
            emit message("Не удалось сконвертировать файл " + file->fileName());
        }
    }

    finalize();
}

void AbstractAlgorithm::finalize()
{DD;
    qDebug()<<"End converting"<<QDateTime::currentDateTime().time();
    emit message(QString("Расчет закончен в %1").arg(QDateTime::currentDateTime().time().toString()));
    emit finished();
}
