#include "fileio.h"

#include "filedescriptor.h"

FileIO::FileIO(QString fileName, QObject *parent) : QObject(parent), fileName(fileName)
{

}

void FileIO::setParameter(const QString &name, const QVariant &value)
{
    m_parameters.insert(name, value);
}

void FileIO::addChannel(Channel *channel)
{
    bool populated = channel->populated();
    if (!populated) channel->populate();
    addChannel(&channel->dataDescription(), channel->data());
    if (!populated) channel->clear();
}
