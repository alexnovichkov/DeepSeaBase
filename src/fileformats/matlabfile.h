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

    void setXml(Dataset xml) {this->xml = xml;}

    static QStringList fileFilters();
    static QStringList suffixes();

    // FileDescriptor interface
public:
    virtual void read() override;
    virtual void write() override;
    virtual int channelsCount() const override;
    virtual Channel *channel(int index) const override;
    virtual QString icon() const override;
private:
    void init(const QVector<Channel *> &source);
    matvar_t* createFileDescription() const;
    matvar_t* createDescription() const;
    matvar_t* createSourceDescription() const;
    void readFileDescription(matvar_t *matvar);
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
    explicit MatlabChannel(Channel &other, MatlabFile *parent);

    MatlabFile *parent;
    matvar_t *values = nullptr;
    XChannel xml;
//    QString _name;
//    QString _primaryChannel;
    QString _type;
//    int _octaveType = 0;
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
public:
    void write(mat_t *matfp);
private:
    matvar_t * createMatVar();
    matvar_t *createXValuesVar();
    matvar_t *createYValuesVar();
    matvar_t *createZValuesVar();
    matvar_t *createQuantity(QString label);
    matvar_t *createQuantityTerms(QString label);
    matvar_t *createUnitTransformation(QString label);
    matvar_t *createFunctionRecord();
};

matvar_t* createUtf8Field(const QString &val);
matvar_t* createDoubleField(double val);
matvar_t* createDoubleField(const QVector<double> &val);
matvar_t* createSingleField(const QVector<double> &val, const size_t N, const size_t n);
matvar_t* createSingleField(const QVector<cx_double> &val, const size_t N, const size_t n);
//QVector<cx_double> getDataFromMatVarUngroupedComplex(matvar_t *v);
QVector<cx_double> getDataFromMatVarComplex(matvar_t *v, int index);
//QVector<double> getDataFromMatVarUngrouped(matvar_t *v);
//QVector<double> getDataFromMatVarGrouped(matvar_t *v, int index);
QVector<double> getDataFromMatVar(matvar_t *v, int index);

#endif // MATLABFILE_H
