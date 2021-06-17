#ifndef UFFFILE_H
#define UFFFILE_H

#include <QtCore>

#include "fileformats/filedescriptor.h"
#include "fields.h"
#include "algorithms.h"

class DataHolder;

int uffWindowTypeFromDescription(const QString &description);
QString windowDescriptionFromUffType(int type);
int scalingTypeFromDescription(const QString &description);
QString scalingDescriptionFromUffType(int type);
int normalizationTypeFromDescription(const QString &description);
QString normalizationDescriptionFromUffType(int type);
int unitTypeFromName(const QString &name);
QString unitNameFromUffType(int type);
QString unitDescriptionFromUffType(int type);
double logrefFromUffUnit(int type);


class FunctionHeader
{
public:
    FunctionHeader();
    void read(QTextStream &stream);
    void read(char *data, qint64 &offset);
    void write(QTextStream &stream);
    void toDataDescription(DataDescription &d);
    void sanitize();
    static FunctionHeader fromDescription(const DataDescription &d);

    QVector<FieldDescription> type1858;
    bool valid;
};

class FunctionDescription
{
public:
    FunctionDescription();
    void read(QTextStream &stream);
    void read(char *data, qint64 &offset);
    void write(QTextStream &stream);
    void toDataDescription(DataDescription &d);
    void sanitize();
    static FunctionDescription fromDescription(const DataDescription &d);

    QVector<FieldDescription> type58;
    bool valid;
};

QDataStream & operator>> (QDataStream& stream, FunctionHeader& header);
QDataStream & operator<< (QDataStream& stream, const FunctionHeader& header);

class UffFileDescriptor;

class Function : public Channel
{
public:
    Function(UffFileDescriptor *parent);
    Function(Channel &other, UffFileDescriptor *parent);
    virtual ~Function();

    void read(QTextStream &stream, qint64 pos = -1);
    void read(char *data, qint64 &offset, int size);
    void read(QDataStream &stream);
    void write(QTextStream &stream, int &id);

    virtual FileDescriptor *descriptor() const override;
    QVector<qint64> dataPositions;
    QVector<qint64> dataEnds;
    QVector<double> zValues;

    // Channel interface
public:
    virtual int index() const override;

    virtual Descriptor::DataType type() const override;
    virtual void populate() override;
private:
    friend class UffFileDescriptor;
    UffFileDescriptor *parent;
    void readRest();
    bool populateWithMmap();
    bool populateWithStream();
};

class UffHeader
{
public:
    /* Header */
    UffHeader();
    UffHeader(const DataDescription &data);

    void read(QTextStream &stream);
    void read(char *pos, qint64 &offset);
    void write(QTextStream &stream);
    QString info() const;
    DataDescription toDataDescription() const;

    QVector<FieldDescription> type151;
};

QDataStream &operator>>(QDataStream& stream, UffHeader& header);
QDataStream &operator<<(QDataStream& stream, const UffHeader& header);

class UffUnits
{
public:
    UffUnits();

    void read(QTextStream &stream);
    void read(char *pos, qint64 &offset);
    void write(QTextStream &stream);

    QVector<FieldDescription> type164;
};

QDataStream &operator>>(QDataStream& stream, UffUnits& header);
QDataStream &operator<<(QDataStream& stream, const UffUnits& header);

class UffFileDescriptor : public FileDescriptor
{
public:
    UffFileDescriptor(const QString &fileName);
    UffFileDescriptor(const FileDescriptor &other, const QString &fileName,
                      QVector<int> indexes = QVector<int>());
    ~UffFileDescriptor();

    QList<Function *> channels;

    // FileDescriptor interface
public:
    virtual void read() override;
    virtual void write() override;

    virtual void deleteChannels(const QVector<int> &channelsToDelete) override;
    virtual void copyChannelsFrom(FileDescriptor *sourceFile, const QVector<int> &indexes) override;

    virtual void addChannelWithData(DataHolder *data, const DataDescription &description) override;

    virtual void move(bool up, const QVector<int> &indexes, const QVector<int> &newIndexes) override;

    virtual int channelsCount() const override;

    virtual Channel *channel(int index) const override;
    virtual bool operator ==(const FileDescriptor &descriptor) override;

    virtual void setChanged(bool changed) override;

    static QStringList fileFilters();
    static QStringList suffixes();
private:
    void removeTempFile();
    void readWithStreams();
    bool readWithMmap();
};

#endif // UFFFILE_H
