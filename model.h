#ifndef MODEL_H
#define MODEL_H

#include <QObject>
#include <QAbstractTableModel>
#include "filedescriptor.h"
#include <QFont>

class Model : public QAbstractTableModel
{
    Q_OBJECT
public:
    Model(QObject *parent = 0);
    ~Model();

    bool contains(const QString &fileName, int *index = 0) const;
    bool contains(FileDescriptor *file, int *index = 0) const;
    FileDescriptor *file(int i);
    FileDescriptor *find(const QString &fileName);
    int rowOfFile(FileDescriptor *file) const;
    void addFiles(const QList<FileDescriptor*> &files);
    void deleteFiles();
    int size() const {return descriptors.size();}
    QList<int> selected() const {return indexes;}
    void setSelected(const QList<int> &indexes) {this->indexes = indexes;}
    QList<FileDescriptor*> selectedFiles() const;

    void setDataDescriptor(FileDescriptor *file, const DescriptionList &data);
    void setChannelDescription(int channel, const QString &description);
    void setChannelName(int channel, const QString &name);
    void updateFile(FileDescriptor *file, int column);
    void updateFile(FileDescriptor *file);
    void clear();
    void invalidateGraphs();
    void invalidateGraph(FileDescriptor *file, int channel);

    void save();
    QModelIndex modelIndexOfFile(FileDescriptor *f, int column);

    // QAbstractItemModel interface
public:
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role);
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;


signals:
    void legendsChanged();
    void plotNeedsUpdate();
private:
    QList<FileDescriptor *> descriptors;
    QFont uFont;
    QFont bFont;

    QList<int> indexes;
};

#endif // MODEL_H
