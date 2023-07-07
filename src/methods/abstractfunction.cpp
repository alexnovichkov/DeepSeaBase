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

QString AbstractFunction::parametersDescription() const
{DD;
    const QStringList props = parameters();
    if (props.isEmpty()) return "";

    QStringList result;
    for (const QString &p: props) {
        result.append(m_parameterDescription(p));
    }
    return "["+result.join(',')+"]";
}

bool AbstractFunction::shouldParameterBeVisible(const QString &parameter) const
{DD;
    if (!parameter.startsWith(name()+"/")) return false;
    QString p = parameter.section("/",1);

    return m_parameterShowsFor(p);
}

QVariant AbstractFunction::getParameter(const QString &parameter) const
{DD;
    QVariant p;
    if (paired()) {
        p = m_master->m_getParameter(parameter);
    }
    if (!p.isValid())
        p = m_getParameter(parameter);
    return p;
}

void AbstractFunction::setParameter(const QString &parameter, const QVariant &val)
{DD;
    if (m_slave != nullptr) {
        m_slave->m_setParameter(parameter, val);
    }

    m_setParameter(parameter, val);
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
    output.clear();
//    triggerData.clear();
}

void AbstractFunction::resetData()
{DD;
    if (m_input) m_input->resetData();
    if (m_input2) m_input2->resetData();
}

void AbstractFunction::updateParameter(const QString &parameter, const QVariant &val)
{DD;
    Q_UNUSED(parameter);
    Q_UNUSED(val);
}

AbstractAlgorithm::AbstractAlgorithm(QList<FileDescriptor *> &dataBase, QObject *parent) : QObject(parent),
    m_dataBase(dataBase)
{DD;

}

AbstractAlgorithm::~AbstractAlgorithm()
{DD;
    m_functions.clear();
}

QVariant AbstractAlgorithm::getParameter(AbstractFunction *function, const QString &parameter) const
{DD;
    if (parameter.isEmpty()) return QVariant();

    const auto list = functions();
    for (AbstractFunction *f: list) {
        if (f == function && parameter.startsWith(f->name()+"/")) {
            return f->getParameter(parameter);
        }
    }

    return QVariant();
}

void AbstractAlgorithm::setParameter(AbstractFunction *function, const QString &parameter, const QVariant &val)
{DD;
    if (parameter.isEmpty()) return;

    const auto list = functions();
    for (AbstractFunction *f: list) {
        if (f == function && parameter.startsWith(f->name()+"/")) {
            f->setParameter(parameter, val);
            return;
        }
    }
}

void AbstractAlgorithm::saveSettings()
{DD;
    for (auto f: m_functions) {
        for (const auto &p: f->parameters()) {
            QVariant val = f->getParameter(f->name()+"/"+p);
            se->setSetting(displayName()+"/"+f->name()+"/"+p, val);
        }
    }
}

void AbstractAlgorithm::restoreSettings()
{DD;
    for (auto f: m_functions) {
        for (const auto &p: f->parameters()) {
            QVariant val = se->getSetting(displayName()+"/"+f->name()+"/"+p);
            f->setParameter(f->name()+"/"+p, val);
        }
    }
}

bool AbstractAlgorithm::compute(FileDescriptor *file)
{DD;
    if (QThread::currentThread()->isInterruptionRequested()) {
        LOG(WARNING) << QString("Запрошена отмена расчета");
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
            LOG(WARNING) << QString("Алгоритм %1 не умеет обрабатывать каналы типа %2. "
                                    "Канал %3 будет пропущен")
                            .arg(displayName())
                            .arg(file->channel(i)->type())
                            .arg(i+1);
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

    if (fileName.isEmpty()) {
        LOG(ERROR) << QString("Не удалось создать файл %1").arg(fileName);
        return false;
    }
    LOG(INFO) << QString("Сохранен файл %1").arg(fileName);
    newFiles << fileName;
    return true;
}

void AbstractAlgorithm::start()
{DD;
    auto dt = QDateTime::currentDateTime();
    LOG(INFO) << QString("Запуск расчета %1 в %2").arg(displayName()).arg(dt.time().toString());
    emit message(QString("Запуск расчета: %1").arg(dt.time().toString()));

//    QDir d;
//    if (!d.exists("C:/DeepSeaBase-temp"))  d.mkdir("C:/DeepSeaBase-temp");

//    QTemporaryDir tempDir("C:\\DeepSeaBase-temp\\temp-XXXXXX");
//    tempDir.setAutoRemove(true);
//    tempFolderName = tempDir.path();



    for (FileDescriptor *file: qAsConst(m_dataBase)) {
        LOG(INFO) << QString("Расчет для файла %1").arg(file->fileName());
        emit message(QString("Расчет для файла\n%1").arg(file->fileName()));
        if (!compute(file)) {
            LOG(ERROR) << QString("Не удалось выполнить расчет");
            emit message("Не удалось выполнить расчет");
        }
    }

    finalize();
}

void AbstractAlgorithm::finalize()
{DD;
    emit message(QString("Расчет закончен в %1").arg(QDateTime::currentDateTime().time().toString()));
    LOG(INFO) << QString("Расчет закончен в %1").arg(QDateTime::currentDateTime().time().toString());

    emit finished();
}

bool AbstractAlgorithm::applicableTo(Descriptor::DataType channelType)
{DD;
    return channelType == Descriptor::TimeResponse;
}
