#ifndef ANAFILE_H
#define ANAFILE_H

#include "filedescriptor.h"

class AnaChannel;

/*
 * dateTime
 * fileCreationTime
 *
 *
 */

class AnaFile : public FileDescriptor
{
public:
    AnaFile(const QString &fileName);
    virtual ~AnaFile();

    static QStringList fileFilters();
    static QStringList suffixes();

    // FileDescriptor interface
public:
    virtual void read() override;
    virtual int channelsCount() const override;
    virtual Channel *channel(int index) const override;
    virtual QString fileType() const override;
    virtual QString icon() const override;
    virtual bool rename(const QString &newName, const QString &newPath) override;
    virtual bool rename(const QString &newName) override;
    virtual bool remove() override;
    virtual bool copyTo(const QString &name) override;
    virtual Descriptor::DataType type() const override;
    virtual QString typeDisplay() const override;
    virtual bool fileExists() const override;
    virtual bool isSourceFile() const override;
    virtual bool dataTypeEquals(FileDescriptor *other) const override;
    virtual bool canTakeChannelsFrom(FileDescriptor *other) const override;
    virtual bool canTakeAnyChannels() const override;
    virtual qint64 fileSize() const override;

private:
    friend class AnaChannel;
    int getFormat() const;
    QString rawFileName; // путь к RAW файлу
    AnaChannel *_channel = nullptr;
};


/*
 * function.precision int16/int32 <- "FORMAT"
 * function.name "time"
 * function.type 1
 * function.logref
 * function.logscale <- "linear"
 * function.format "real"
 * function.octaveFormat 0
 *
 * samplerate <- FRQ
 * name <- ;
 * description <- CHANNEL
 * yname <- "В"
 * xname <- "с"
 * zname <- ""
 * samples <- высчитывается по FORMAT и по размеру файла ana
 * blocks <- 1
 *
 * description.gain
 * description.absvolt
 * description.base
 * description.dboff
 * description.frccorr
 * description.velocity
 * description.distance
 * description.time
 * description.voltage
 *
 */

class AnaChannel: public Channel
{
public:
    AnaChannel(AnaFile *parent);

    // Channel interface
public:
    virtual QVariant info(int column, bool edit) const override;
    virtual int columnsCount() const override;
    virtual QVariant channelHeader(int column) const override;
    virtual Descriptor::DataType type() const override;
    virtual void populate() override;
    virtual FileDescriptor *descriptor() const override;
    virtual int index() const override;
private:
    void postprocess(QVector<double> &v);
    friend class AnaFile;

    AnaFile *parent = nullptr;
    double coef1, coef2;
    int format = 2; //2 - int16, 4 - int32
};

#endif // ANAFILE_H
