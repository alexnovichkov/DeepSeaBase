#ifndef CHANNELTABLEMODEL_H
#define CHANNELTABLEMODEL_H

#include <QObject>
#include <QAbstractTableModel>
#include "filedescriptor.h"
#include <QFont>

class ChannelTableModel : public QAbstractTableModel
{
public:
    ChannelTableModel(QObject *parent);
    Channel *channel(int index);

    void clear();

    // QAbstractItemModel interface
public:
    virtual int rowCount(const QModelIndex &parent) const override;
    virtual int columnCount(const QModelIndex &parent) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
//    virtual bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role) override;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const override;
private:
    FileDescriptor *descriptor;
    int channelsCount;
    QList<int> indexes;

    QFont uFont;
    QFont bFont;
};

#endif // CHANNELTABLEMODEL_H
