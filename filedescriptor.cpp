#include "filedescriptor.h"

FileDescriptor::FileDescriptor(const QString &fileName) :
    _fileName(fileName), _changed(false), _dataChanged(false),
    XBegin(0.0),
    NumInd(0)
{
}

bool FileDescriptor::fileExists() const
{
    return QFileInfo(_fileName).exists();
}

void FileDescriptor::setLegend(const QString &legend)
{
    if (legend == _legend) return;
    _legend = legend;
//    setChanged(true);
}


QString descriptionEntryToString(const DescriptionEntry &entry)
{
    QString result = entry.second;
    if (!entry.first.isEmpty()) return entry.first+"="+result;

    return result;
}

QList<int> filterIndexes(FileDescriptor *dfd, const QList<QPair<FileDescriptor *, int> > &channels)
{
    QList<int> result;
    for(int i=0; i<channels.size(); ++i)
        if (channels.at(i).first == dfd) result << channels.at(i).second;
    return result;
}
