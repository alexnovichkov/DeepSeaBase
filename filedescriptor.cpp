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
