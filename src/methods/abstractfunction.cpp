#include "abstractfunction.h"

#include <QtCore>
#include "fileformats/filedescriptor.h"
#include "logging.h"

AbstractFunction::AbstractFunction(QObject *parent) : QObject(parent),
    m_input(0)
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
{
    Q_UNUSED(property);

    return true;
}

void AbstractFunction::setFile(FileDescriptor *file)
{
    m_file = file;
    if (m_input) m_input->setFile(file);
}

void AbstractFunction::reset()
{
    // no-op
}

void AbstractFunction::updateProperty(const QString &property, const QVariant &val)
{
    Q_UNUSED(property);
    Q_UNUSED(val);
    //no-op
}




AbstractAlgorithm::AbstractAlgorithm(QList<FileDescriptor *> &dataBase, QObject *parent) : QObject(parent),
    m_dataBase(dataBase)
{

}

bool AbstractAlgorithm::propertyShowsFor(const QString &property) const
{
    if (property.isEmpty()) return true;

    for (AbstractFunction *f: functions()) {
        if (property.startsWith(f->name()+"/")) {
            return f->propertyShowsFor(property);
        }
    }

    return true;
}

QVariant AbstractAlgorithm::getProperty(const QString &property) const
{
    if (property.isEmpty()) return QVariant();

    for (AbstractFunction *f: functions()) {
        if (property.startsWith(f->name()+"/")) {
            return f->getProperty(property);
        }
    }

    return QVariant();
}

void AbstractAlgorithm::setProperty(const QString &property, const QVariant &val)
{
    if (property.isEmpty()) return;

    for (AbstractFunction *f: functions()) {
        if (property.startsWith(f->name()+"/")) {
            f->setProperty(property, val);
            return;
        }
    }
}

void AbstractAlgorithm::reset()
{
    // no-op
}

void AbstractAlgorithm::start()
{
    dt = QDateTime::currentDateTime();
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
{
    qDebug()<<"End converting"<<QDateTime::currentDateTime().time();
    emit message(QString("Расчет закончен в %1").arg(QDateTime::currentDateTime().time().toString()));
    emit finished();
}
