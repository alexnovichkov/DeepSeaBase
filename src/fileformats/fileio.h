#ifndef FILEIO_H
#define FILEIO_H

class FileDescriptor;
class Channel;

class FileIO
{
public:
    FileIO();
    virtual ~FileIO() {}

    virtual void read(FileDescriptor *file) = 0;
    virtual bool write(FileDescriptor *file) = 0;
    virtual void populate(Channel *channel) = 0;
};

#endif // FILEIO_H
