#include "fileio.h"

FileIO::FileIO(FileDescriptor *file, QObject *parent) : QObject(parent), m_file(file)
{

}
