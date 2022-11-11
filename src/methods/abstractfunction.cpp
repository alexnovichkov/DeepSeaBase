#include "abstractfunction.h"

#include <QtCore>
#include "fileformats/filedescriptor.h"
#include "logging.h"
#include "settings.h"

AbstractFunction::AbstractFunction(QObject *parent, const QString &name) : QObject(parent), _name(name)
{DD;

}

AbstractFunction::~AbstractFunction()
{DD;
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

QVariant AbstractFunction::getParameter(const QString &property) const
{DD;
    QVariant p;
    if (paired()) {
        p = m_master->m_getProperty(property);
    }
    if (!p.isValid())
        p = m_getProperty(property);
    return p;
}

void AbstractFunction::setParameter(const QString &property, const QVariant &val)
{DD;
    if (m_slave != nullptr) {
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
{DD;
    if (id == "input") return output;
    if (id == "triggerInput") return triggerData;

    return QVector<double>();
}

DataDescription AbstractFunction::getFunctionDescription() const
{DD;
    DataDescription result;
    if (m_input) result = m_input->getFunctionDescription();
    return result;
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
{DD;
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

bool AbstractAlgorithm::propertyShowsFor(AbstractFunction *function, const QString &property) const
{DD;
    if (property.isEmpty()) return true;

    const auto list = functions();
    for (AbstractFunction *f: list) {
        if (f == function && property.startsWith(f->name()+"/")) {
            return f->propertyShowsFor(property);
        }
    }

    return true;
}

QVariant AbstractAlgorithm::getParameter(AbstractFunction *function, const QString &property) const
{DD;
    if (property.isEmpty()) return QVariant();

    const auto list = functions();
    for (AbstractFunction *f: list) {
        if (f == function && property.startsWith(f->name()+"/")) {
            return f->getParameter(property);
        }
    }

    return QVariant();
}

void AbstractAlgorithm::setParameter(AbstractFunction *function, const QString &property, const QVariant &val)
{DD;
    if (property.isEmpty()) return;

    const auto list = functions();
    for (AbstractFunction *f: list) {
        if (f == function && property.startsWith(f->name()+"/")) {
            f->setParameter(property, val);
            return;
        }
    }
}

void AbstractAlgorithm::saveSettings()
{DD;
    for (auto f: m_functions) {
        for (const auto &property: f->properties()) {
            QVariant val = f->getParameter(f->name()+"/"+property);
            Settings::setSetting(displayName()+"/"+f->name()+"/"+property, val);
        }
    }
}

void AbstractAlgorithm::restoreSettings()
{DD;
    for (auto f: m_functions) {
        for (const auto &property: f->properties()) {
            QVariant val = Settings::getSetting(displayName()+"/"+f->name()+"/"+property);
            f->setParameter(f->name()+"/"+property, val);
        }
    }
}

bool AbstractAlgorithm::compute(FileDescriptor *file)
{DD;
    if (QThread::currentThread()->isInterruptionRequested()) {
        finalize();
        return false;
    }
    if (file->channelsCount()==0) return false;
    if (m_chain.isEmpty()) return false;

    m_chain.last()->reset();

    initChain(file);

    const int count = file->channelsCount();
    const int refChannel = m_chain.last()->getParameter("?/referenceChannelIndex").toInt()-1;
    const QStringList channels = m_chain.first()->getParameter("?/channels").toStringList();

    for (int i=0; i<count; ++i) {
        //beginning of the chain
        m_chain.first()->setParameter("Channel/channelIndex", i);

        if (!channels.isEmpty() && !channels.contains(QString::number(i+1))) {
            emit tick();
            continue;
        }

        if (!applicableTo(file->channel(i)->type())) {
            emit tick();
            continue;
        }

        if (refChannel == i) {
            emit tick();
            continue;
        }

        const bool wasPopulated = file->channel(i)->populated();

        resetChain();

        //so far end of the chain
        // for each channel
        m_chain.last()->setFile(file);
        m_chain.last()->compute(file); //and collect the result

        if (!wasPopulated) file->channel(i)->clear();
        emit tick();
    }
    m_chain.last()->reset();
    QString fileName = m_chain.last()->getParameter(m_chain.last()->name()+"/name").toString();
//    LOG(DEBUG)<<fileName;

    if (fileName.isEmpty()) return false;
    newFiles << fileName;
    return true;
}

void AbstractAlgorithm::reset()
{DD;
    // no-op
}

void AbstractAlgorithm::start()
{DD;
    auto dt = QDateTime::currentDateTime();
    emit message(QString("Запуск расчета: %1").arg(dt.time().toString()));

//    QDir d;
//    if (!d.exists("C:/DeepSeaBase-temp"))  d.mkdir("C:/DeepSeaBase-temp");

//    QTemporaryDir tempDir("C:\\DeepSeaBase-temp\\temp-XXXXXX");
//    tempDir.setAutoRemove(true);
//    tempFolderName = tempDir.path();



    for (FileDescriptor *file: qAsConst(m_dataBase)) {
        emit message(QString("Расчет для файла\n%1").arg(file->fileName()));
        if (!compute(file)) {
            emit message("Не удалось выполнить расчет");
        }
    }

    finalize();
}

void AbstractAlgorithm::finalize()
{DD;
    emit message(QString("Расчет закончен в %1").arg(QDateTime::currentDateTime().time().toString()));
    emit finished();
}

bool AbstractAlgorithm::applicableTo(Descriptor::DataType channelType)
{DD;
    return channelType == Descriptor::TimeResponse;
}
