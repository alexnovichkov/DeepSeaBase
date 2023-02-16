#include "fileio.h"

FileIO::FileIO(QString fileName, QObject *parent) : QObject(parent), fileName(fileName)
{

}

void FileIO::setParameter(const QString &name, const QVariant &value)
{
    m_parameters.insert(name, value);
}
