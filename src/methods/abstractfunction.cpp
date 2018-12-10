#include "abstractfunction.h"

#include <QtCore>
#include "filedescriptor.h"

AbstractFunction::AbstractFunction(QList<FileDescriptor *> &dataBase, QObject *parent) : QObject(parent),
    m_dataBase(dataBase), m_input(0)
{

}

QString AbstractFunction::propertiesDescription() const
{
    QString result="[";
    foreach (const QString &p, properties()) {
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

bool AbstractFunction::compute()
{
    return true;
}

bool AbstractFunction::compute(FileDescriptor *file, const QString &tempFolderName)
{
    Q_UNUSED(file);
    Q_UNUSED(tempFolderName);
    return true;
}

QVector<double> AbstractFunction::get(FileDescriptor *file, const QVector<double> &data)
{
    Q_UNUSED(file);
    Q_UNUSED(data);

    return QVector<double>();
}

void AbstractFunction::reset()
{
    // no-op
}

void AbstractFunction::start()
{
    dt = QDateTime::currentDateTime();
    qDebug()<<"Start converting"<<dt.time();
    emit message(QString("Запуск расчета: %1").arg(dt.time().toString()));

    QDir d;
    if (!d.exists("C:/DeepSeaBase-temp"))  d.mkdir("C:/DeepSeaBase-temp");

    QTemporaryDir tempDir("C:\\DeepSeaBase-temp\\temp-XXXXXX");
    tempDir.setAutoRemove(true);
    tempFolderName = tempDir.path();



    foreach (FileDescriptor *file, m_dataBase) {
        emit message(QString("Расчет для файла\n%1").arg(file->fileName()));
        if (!compute(file, tempFolderName)) {
            emit message("Не удалось сконвертировать файл " + file->fileName());
        }
    }

    finalize();
}

void AbstractFunction::finalize()
{

}

bool AbstractFunction::event(QEvent *event)
{
//    qDebug()<<name()<<"moved to thread"<<thread();
//    if (event->type() ==  QEvent::ThreadChange) {
//        foreach (AbstractFunction *f, m_functions) {
//            qDebug()<<f->name()<<"is currently in"<<f->thread();
//            f->moveToThread(this->thread());
//        }
//    }
    return QObject::event(event);
}
