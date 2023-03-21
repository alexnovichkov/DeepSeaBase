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
    if (m_parameters.contains("samplesCount")) {
        DataHolder d;
        d.setSegment(*channel->data(), 0, m_parameters.value("samplesCount").toInt()-1);
        addChannel(&channel->dataDescription(), &d);
    }
    else
        addChannel(&channel->dataDescription(), channel->data());
    if (!populated) channel->clear();
}
