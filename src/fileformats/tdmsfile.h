#ifndef TDMSFILE_H
#define TDMSFILE_H

#include <QList>
#include <QFlag>
#include <QVariant>
#include <QObject>
#include <QLibrary>

#include "nilibddc.h"

struct DDCMethods
{
    DDCMethods() {
        l.setFileName("nilibddc");
    }
    void load() {
        l.load();
        loaded = true;

        getLibraryErrorDescription = reinterpret_cast<DDCGetLibraryErrorDescription>(l.resolve("DDC_GetLibraryErrorDescription"));
        if (!getLibraryErrorDescription) loaded = false;
        ddcOpenFileEx = reinterpret_cast<DDCOpenFileEx>(l.resolve("DDC_OpenFileEx"));
        if (!ddcOpenFileEx) loaded = false;
        getNumFileProperties = reinterpret_cast<DDCGetNumFileProperties>(l.resolve("DDC_GetNumFileProperties"));
        if (!getNumFileProperties) loaded = false;
        getFilePropertyNameLengthFromIndex = reinterpret_cast<DDCGetFilePropertyNameLengthFromIndex>(l.resolve("DDC_GetFilePropertyNameLengthFromIndex"));
        if (!getFilePropertyNameLengthFromIndex) loaded = false;
        getFilePropertyNameFromIndex = reinterpret_cast<DDCGetFilePropertyNameFromIndex>(l.resolve("DDC_GetFilePropertyNameFromIndex"));
        if (!getFilePropertyNameFromIndex) loaded = false;
        getFilePropertyType = reinterpret_cast<DDCGetFilePropertyType>(l.resolve("DDC_GetFilePropertyType"));
        if (!getFilePropertyType) loaded = false;
        getFileProperty = reinterpret_cast<DDCGetFileProperty>(l.resolve("DDC_GetFileProperty"));
        if (!getFileProperty) loaded = false;
        getFileStringPropertyLength = reinterpret_cast<DDCGetFileStringPropertyLength>(l.resolve("DDC_GetFileStringPropertyLength"));
        if (!getFileStringPropertyLength) loaded = false;
        getFilePropertyTimestampComponents = reinterpret_cast<DDCGetFilePropertyTimestampComponents>(l.resolve("DDC_GetFilePropertyTimestampComponents"));
        if (!getFilePropertyTimestampComponents) loaded = false;
        getNumChannelGroups = reinterpret_cast<DDCGetNumChannelGroups>(l.resolve("DDC_GetNumChannelGroups"));
        if (!getNumChannelGroups) loaded = false;
        getChannelGroups = reinterpret_cast<DDCGetChannelGroups>(l.resolve("DDC_GetChannelGroups"));
        if (!getChannelGroups) loaded = false;
        closeFile = reinterpret_cast<DDCCloseFile>(l.resolve("DDC_CloseFile"));
        if (!closeFile) loaded = false;
        getNumChannelGroupProperties = reinterpret_cast<DDCGetNumChannelGroupProperties>(l.resolve("DDC_GetNumChannelGroupProperties"));
        if (!getNumChannelGroupProperties) loaded = false;
        getChannelGroupPropertyNameLengthFromIndex = reinterpret_cast<DDCGetChannelGroupPropertyNameLengthFromIndex>(l.resolve("DDC_GetChannelGroupPropertyNameLengthFromIndex"));
        if (!getChannelGroupPropertyNameLengthFromIndex) loaded = false;
        getChannelGroupPropertyNameFromIndex = reinterpret_cast<DDCGetChannelGroupPropertyNameFromIndex>(l.resolve("DDC_GetChannelGroupPropertyNameFromIndex"));
        if (!getChannelGroupPropertyNameFromIndex) loaded = false;
        getChannelGroupPropertyType = reinterpret_cast<DDCGetChannelGroupPropertyType>(l.resolve("DDC_GetChannelGroupPropertyType"));
        if (!getChannelGroupPropertyType) loaded = false;
        getChannelGroupProperty = reinterpret_cast<DDCGetChannelGroupProperty>(l.resolve("DDC_GetChannelGroupProperty"));
        if (!getChannelGroupProperty) loaded = false;
        getChannelGroupStringPropertyLength = reinterpret_cast<DDCGetChannelGroupStringPropertyLength>(l.resolve("DDC_GetChannelGroupStringPropertyLength"));
        if (!getChannelGroupStringPropertyLength) loaded = false;
        getChannelGroupPropertyTimestampComponents = reinterpret_cast<DDCGetChannelGroupPropertyTimestampComponents>(l.resolve("DDC_GetChannelGroupPropertyTimestampComponents"));
        if (!getChannelGroupPropertyTimestampComponents) loaded = false;
        getNumChannels = reinterpret_cast<DDCGetNumChannels>(l.resolve("DDC_GetNumChannels"));
        if (!getNumChannels) loaded = false;
        getChannels = reinterpret_cast<DDCGetChannels>(l.resolve("DDC_GetChannels"));
        if (!getChannels) loaded = false;
        closeChannelGroup = reinterpret_cast<DDCCloseChannelGroup>(l.resolve("DDC_CloseChannelGroup"));
        if (!closeChannelGroup) loaded = false;
        getNumChannelProperties = reinterpret_cast<DDCGetNumChannelProperties>(l.resolve("DDC_GetNumChannelProperties"));
        if (!getNumChannelProperties) loaded = false;
        getChannelPropertyNameLengthFromIndex = reinterpret_cast<DDCGetChannelPropertyNameLengthFromIndex>(l.resolve("DDC_GetChannelPropertyNameLengthFromIndex"));
        if (!getChannelPropertyNameLengthFromIndex) loaded = false;
        getChannelPropertyNameFromIndex = reinterpret_cast<DDCGetChannelPropertyNameFromIndex>(l.resolve("DDC_GetChannelPropertyNameFromIndex"));
        if (!getChannelPropertyNameFromIndex) loaded = false;
        getChannelPropertyType = reinterpret_cast<DDCGetChannelPropertyType>(l.resolve("DDC_GetChannelPropertyType"));
        if (!getChannelPropertyType) loaded = false;
        getChannelProperty = reinterpret_cast<DDCGetChannelProperty>(l.resolve("DDC_GetChannelProperty"));
        if (!getChannelProperty) loaded = false;
        getChannelPropertyTimestampComponents = reinterpret_cast<DDCGetChannelPropertyTimestampComponents>(l.resolve("DDC_GetChannelPropertyTimestampComponents"));
        if (!getChannelPropertyTimestampComponents) loaded = false;
        getChannelStringPropertyLength = reinterpret_cast<DDCGetChannelStringPropertyLength>(l.resolve("DDC_GetChannelStringPropertyLength"));
        if (!getChannelStringPropertyLength) loaded = false;
        getNumDataValues = reinterpret_cast<DDCGetNumDataValues>(l.resolve("DDC_GetNumDataValues"));
        if (!getNumDataValues) loaded = false;
        getDataType = reinterpret_cast<DDCGetDataType>(l.resolve("DDC_GetDataType"));
        if (!getDataType) loaded = false;
        closeChannel = reinterpret_cast<DDCCloseChannel>(l.resolve("DDC_CloseChannel"));
        if (!closeChannel) loaded = false;
        getDataValuesUInt8 = reinterpret_cast<DDCGetDataValuesUInt8>(l.resolve("DDC_GetDataValuesUInt8"));
        if (!getDataValuesUInt8) loaded = false;
        getDataValuesInt16 = reinterpret_cast<DDCGetDataValuesInt16>(l.resolve("DDC_GetDataValuesInt16"));
        if (!getDataValuesInt16) loaded = false;
        getDataValuesInt32 = reinterpret_cast<DDCGetDataValuesInt32>(l.resolve("DDC_GetDataValuesInt32"));
        if (!getDataValuesInt32)  loaded = false;
        getDataValuesFloat = reinterpret_cast<DDCGetDataValuesFloat>(l.resolve("DDC_GetDataValuesFloat"));
        if (!getDataValuesFloat) loaded = false;
        getDataValuesDouble = reinterpret_cast<DDCGetDataValuesDouble>(l.resolve("DDC_GetDataValuesDouble"));
        if (!getDataValuesDouble) loaded = false;
    }
    typedef int (*DDCOpenFileEx)(const char *,const char *,int ,DDCFileHandle *);
    typedef const char * (*DDCGetLibraryErrorDescription)(int);
    typedef int (*DDCGetNumFileProperties)(DDCFileHandle , unsigned int *);
    typedef int (*DDCGetFilePropertyNameLengthFromIndex)(DDCFileHandle, size_t, size_t*);
    typedef int (*DDCGetFilePropertyNameFromIndex)(DDCFileHandle, size_t, char*, size_t);
    typedef int (*DDCGetFilePropertyType)(DDCFileHandle, const char *, DDCDataType *);
    typedef int (*DDCGetFileProperty)(DDCFileHandle, const char *, void *, size_t);
    typedef int (*DDCGetFileStringPropertyLength)(DDCFileHandle , const char *, unsigned int *);
    typedef int (*DDCGetFilePropertyTimestampComponents)(DDCFileHandle, const char *, unsigned int *, unsigned int *,
                                                          unsigned int *, unsigned int *, unsigned int *,
                                                          unsigned int *, double *, unsigned int *);
    typedef int (*DDCGetNumChannelGroups)(DDCFileHandle, unsigned int *);
    typedef int (*DDCGetChannelGroups)(DDCFileHandle, DDCChannelGroupHandle*, size_t);
    typedef int (*DDCCloseFile)(DDCFileHandle);
    typedef int (*DDCGetNumChannelGroupProperties)(DDCChannelGroupHandle, unsigned int *);
    typedef int (*DDCGetChannelGroupPropertyNameLengthFromIndex)(DDCChannelGroupHandle, size_t, size_t*);
    typedef int (*DDCGetChannelGroupPropertyNameFromIndex)(DDCChannelGroupHandle, size_t, char *, size_t);
    typedef int (*DDCGetChannelGroupPropertyType)(DDCChannelGroupHandle, const char *, DDCDataType *);
    typedef int (*DDCGetChannelGroupProperty)(DDCChannelGroupHandle, const char *, void *, size_t);
    typedef int (*DDCGetChannelGroupStringPropertyLength)(DDCChannelGroupHandle, const char *, unsigned int *);
    typedef int (*DDCGetChannelGroupPropertyTimestampComponents)(DDCChannelGroupHandle, const char *, unsigned int *,
                                                                         unsigned int *, unsigned int *, unsigned int *,
                                                                         unsigned int *, unsigned int *, double *,
                                                                         unsigned int *);
    typedef int (*DDCGetNumChannels)(DDCChannelGroupHandle, unsigned int *);
    typedef int (*DDCGetChannels)(DDCChannelGroupHandle, DDCChannelHandle *, size_t );
    typedef int (*DDCCloseChannelGroup)(DDCChannelGroupHandle);
    typedef int (*DDCGetNumChannelProperties)(DDCChannelHandle , unsigned int *);
    typedef int (*DDCGetChannelPropertyNameLengthFromIndex)(DDCChannelHandle, size_t, size_t*);
    typedef int (*DDCGetChannelPropertyNameFromIndex)(DDCChannelHandle, size_t, char*, size_t);
    typedef int (*DDCGetChannelPropertyType)(DDCChannelHandle, const char *, DDCDataType *);
    typedef int (*DDCGetChannelProperty)(DDCChannelHandle, const char *, void *, size_t);
    typedef int (*DDCGetChannelPropertyTimestampComponents)(DDCChannelHandle,
                                                                    const char *,
                                                                    unsigned int *,
                                                                    unsigned int *,
                                                                    unsigned int *,
                                                                    unsigned int *,
                                                                    unsigned int *,
                                                                    unsigned int *,
                                                                    double *,
                                                                    unsigned int *);
    typedef int (*DDCGetChannelStringPropertyLength)(DDCChannelHandle, const char *, unsigned int *);
    typedef int (*DDCGetNumDataValues)(DDCChannelHandle, unsigned __int64 *);
    typedef int (*DDCGetDataType)(DDCChannelHandle, DDCDataType *);
    typedef int (*DDCCloseChannel)(DDCChannelHandle);
    typedef int (*DDCGetDataValuesUInt8)(DDCChannelHandle, size_t, size_t, unsigned char*);
    typedef int (*DDCGetDataValuesInt16)(DDCChannelHandle, size_t, size_t, short *);
    typedef int (*DDCGetDataValuesInt32)(DDCChannelHandle, size_t, size_t, long*);
    typedef int (*DDCGetDataValuesFloat)(DDCChannelHandle, size_t, size_t, float*);
    typedef int (*DDCGetDataValuesDouble)(DDCChannelHandle, size_t, size_t, double*);

    DDCGetLibraryErrorDescription getLibraryErrorDescription = 0;
    DDCOpenFileEx ddcOpenFileEx = 0;
    DDCGetNumFileProperties getNumFileProperties = 0;
    DDCGetFilePropertyNameLengthFromIndex getFilePropertyNameLengthFromIndex = 0;
    DDCGetFilePropertyNameFromIndex getFilePropertyNameFromIndex = 0;
    DDCGetFilePropertyType getFilePropertyType = 0;
    DDCGetFileProperty getFileProperty = 0;
    DDCGetFileStringPropertyLength getFileStringPropertyLength = 0;
    DDCGetFilePropertyTimestampComponents getFilePropertyTimestampComponents = 0;
    DDCGetNumChannelGroups getNumChannelGroups = 0;
    DDCGetChannelGroups getChannelGroups = 0;
    DDCCloseFile closeFile = 0;
    DDCGetNumChannelGroupProperties getNumChannelGroupProperties = 0;
    DDCGetChannelGroupPropertyNameLengthFromIndex getChannelGroupPropertyNameLengthFromIndex = 0;
    DDCGetChannelGroupPropertyNameFromIndex getChannelGroupPropertyNameFromIndex = 0;
    DDCGetChannelGroupPropertyType getChannelGroupPropertyType = 0;
    DDCGetChannelGroupProperty getChannelGroupProperty = 0;
    DDCGetChannelGroupStringPropertyLength getChannelGroupStringPropertyLength = 0;
    DDCGetChannelGroupPropertyTimestampComponents getChannelGroupPropertyTimestampComponents = 0;
    DDCGetNumChannels getNumChannels = 0;
    DDCGetChannels getChannels = 0;
    DDCCloseChannelGroup closeChannelGroup = 0;
    DDCGetNumChannelProperties getNumChannelProperties = 0;
    DDCGetChannelPropertyNameLengthFromIndex getChannelPropertyNameLengthFromIndex = 0;
    DDCGetChannelPropertyNameFromIndex getChannelPropertyNameFromIndex = 0;
    DDCGetChannelPropertyType getChannelPropertyType = 0;
    DDCGetChannelProperty getChannelProperty = 0;
    DDCGetChannelPropertyTimestampComponents getChannelPropertyTimestampComponents = 0;
    DDCGetChannelStringPropertyLength getChannelStringPropertyLength = 0;
    DDCGetNumDataValues getNumDataValues = 0;
    DDCGetDataType getDataType = 0;
    DDCCloseChannel closeChannel = 0;
    DDCGetDataValuesUInt8 getDataValuesUInt8 = 0;
    DDCGetDataValuesInt16 getDataValuesInt16 = 0;
    DDCGetDataValuesInt32 getDataValuesInt32 = 0;
    DDCGetDataValuesFloat getDataValuesFloat = 0;
    DDCGetDataValuesDouble getDataValuesDouble = 0;

    bool loaded = false;
    QLibrary l;
};

class TDMSChannel
{
public:
    TDMSChannel(DDCChannelHandle channel, DDCMethods *m);
    ~TDMSChannel();
    QVector<double> getDouble();
    QVector<float> getFloat();
    QVariantMap properties;
    quint64 numberOfValues = 0;
    DDCDataType dataType;
private:
    DDCChannelHandle channel;
    DDCMethods *m;
};

class TDMSGroup
{
public:
    TDMSGroup(DDCChannelGroupHandle group, DDCMethods *m);
    ~TDMSGroup();
    QList<TDMSChannel*> channels;
    QVariantMap properties;
private:
    DDCChannelHandle *_channels;
    DDCChannelGroupHandle group;
    DDCMethods *m;
};

class TDMSFile
{
public:
    TDMSFile(const QString &fileName);
    ~TDMSFile();
    bool isValid() const {return _isValid;}
    QList<TDMSGroup*> groups;
    QVariantMap properties;
private:
    QString fileName;
    DDCFileHandle file;
    bool _isValid = true;
    DDCChannelGroupHandle *_groups;
    DDCMethods m;
};

#include <QObject>
#include <QFileInfoList>

class TDMSFileConvertor : public QObject
{
    Q_OBJECT
public:
    TDMSFileConvertor(QObject *parent = 0);
    void setFilesToConvert(const QStringList &toConvert) {filesToConvert = toConvert;}
    void setRawFileFormat(int format) {rawFileFormat = format;} // 0 = float, 1 = quint16
    void setDestinationFormat(const QString &format)  {destinationFormat = format;} //dfd, uff, d94

    QStringList getNewFiles() const {return newFiles;}
public slots:
    bool convert();
signals:
    void tick();
    void finished();
    void message(const QString &s);
private:
    QString destinationFormat;
    QString folderName;
    QStringList newFiles;
    QStringList filesToConvert;
    int rawFileFormat = 0;
};

#endif // TDMSFILE_H
