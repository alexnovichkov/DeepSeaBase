#ifndef MATLABFILE_H
#define MATLABFILE_H

#include "filedescriptor.h"

#include "matio.h"

class MatlabChannel;


#include "matfile.h"

class MatlabFile: public FileDescriptor
{
public:
    MatlabFile(const QString &fileName);
    MatlabFile(const FileDescriptor &other, const QString &fileName,
                      const QVector<int> &indexes = QVector<int>());
    MatlabFile(const QVector<Channel *> &source, const QString &fileName);
    ~MatlabFile();

    static QStringList fileFilters();
    static QStringList suffixes();

    // FileDescriptor interface
public:
    virtual void read() override;
    virtual void write() override;
    virtual void deleteChannels(const QVector<int> &channelsToDelete) override;
    virtual void copyChannelsFrom(const QVector<Channel *> &) override;
    virtual int channelsCount() const override;
    virtual void move(bool up, const QVector<int> &indexes, const QVector<int> &newIndexes) override;
    virtual Channel *channel(int index) const override;
    virtual QString icon() const override;
//    virtual bool rename(const QString &newName, const QString &newPath) override;
    virtual void fillPreliminary(const FileDescriptor *) override;
//    virtual bool copyTo(const QString &name) override;
//    virtual Descriptor::DataType type() const override;
//    virtual QString typeDisplay() const override;
//    virtual bool fileExists() const override;
//    virtual bool isSourceFile() const override;
//    virtual bool operator ==(const FileDescriptor &descriptor) override;
//    virtual bool dataTypeEquals(FileDescriptor *other) const override;
//    virtual bool canTakeChannelsFrom(FileDescriptor *other) const override;
//    virtual bool canTakeAnyChannels() const override;
    virtual void addChannelWithData(DataHolder *data, const DataDescription &description) override;
//    virtual qint64 fileSize() const override;
//    virtual void setChanged(bool changed) override;
private:
    void init(const QVector<Channel *> &);
    mat_t *matfp = NULL;
    QVector<MatlabChannel*> channels;
    QVector<matvar_t *> records;
    Dataset xml;
    friend class MatlabChannel;
};

class MatlabChannel : public Channel
{
public:
    explicit MatlabChannel(MatlabFile *parent);

    MatlabFile *parent;
    matvar_t *values;
    XChannel xml;
    QString _name;
    QString _primaryChannel;
    QString _type;
    int _octaveType = 0;
    bool grouped = false;
    int indexInGroup = 0;
    int groupSize = 1;
    bool complex = false;

    // Channel interface
public:
//    virtual QVariant info(int column, bool edit) const override;
//    virtual int columnsCount() const override;
//    virtual QVariant channelHeader(int column) const override;
    virtual Descriptor::DataType type() const override;
    virtual void populate() override;
//    virtual void setXStep(double xStep) override;
    virtual FileDescriptor *descriptor() const override;
    virtual int index() const override;
};



#endif // MATLABFILE_H
