#ifndef MODEL_H
#define MODEL_H

#include <QObject>
#include <QAbstractTableModel>
#include "app.h"
#include "fileformats/filedescriptor.h"
#include <QFont>

#define MODEL_COLUMNS_COUNT 11
#define MODEL_COLUMN_SAVE 0
#define MODEL_COLUMN_INDEX 1
#define MODEL_COLUMN_FILENAME 2
#define MODEL_COLUMN_DATETIME 3
#define MODEL_COLUMN_TYPE 4
#define MODEL_COLUMN_SIZE 5
#define MODEL_COLUMN_XNAME 6
#define MODEL_COLUMN_XSTEP 7
#define MODEL_COLUMN_CHANNELSCOUNT 8
#define MODEL_COLUMN_DESCRIPTION 9
#define MODEL_COLUMN_LEGEND 10


class Model : public QAbstractTableModel
{
    Q_OBJECT
public:
    Model(QObject *parent = 0);
    ~Model();

    bool contains(const QString &fileName, int *index = 0) const;
    bool contains(const F &file, int *index = 0) const;
    bool contains(FileDescriptor* file, int *index = 0) const;
    //const F &file(int i) const;
    F file(int i);
//    FileDescriptor *find(const QString &fileName) const;

    void addFiles(const QList<F> &files);
    void deleteFiles(); //удаление выделенных файлов
    int size() const {return descriptors.size();}

    QList<int> selected() const {return indexes;}
    void setSelected(const QList<int> &indexes);
    QList<FileDescriptor*> selectedFiles(Descriptor::DataType type) const;
    QList<FileDescriptor*> selectedFiles() const;
    QList<FileDescriptor*> selectedFiles(const QVector<Descriptor::DataType> &types) const;

    void setDataDescription(FileDescriptor *file, const DataDescription &data);
    void setChannelDescription(int channel, const QString &description);
    void setChannelName(int channel, const QString &name);
    void updateFile(FileDescriptor *file, int column = -1);
    void updateFile(int idx, int column = -1);
//    void clear(const QStringList &filesToSkip);
    void clear(); //удаление всех файлов

    void invalidateCurve(Channel* channel);
    void save();
    void discardChanges();

    bool changed() const;

    // QAbstractItemModel interface
public:
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const override;
    virtual Qt::DropActions supportedDropActions() const override;
    virtual bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override;
    virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;


signals:
    void legendsChanged();
    void plotNeedsUpdate();
    void modelChanged();
    void needAddFiles(const QStringList &files);
private:
    QModelIndex modelIndexOfFile(FileDescriptor* f, int column) const;
    QList<F> descriptors;
    QFont uFont;
    QFont bFont;

    QList<int> indexes;

};

#endif // MODEL_H
