#ifndef CHANNELTABLEMODEL_H
#define CHANNELTABLEMODEL_H

#include <QObject>
#include <QAbstractTableModel>
#include <QFont>

class Channel;
class FileDescriptor;
class QCPPlot;

class ChannelTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    ChannelTableModel(QObject *parent);
    ~ChannelTableModel();
    Channel *channel(int index);
    bool hasSelection() const {return !indexes.isEmpty();}
    QVector<int> selected()  const {return indexes;}
    QVector<Channel*> selectedChannels() const;

    void setCurrentPlot(QCPPlot *currentPlot);



    void clear();

    void setDescriptor(FileDescriptor *dfd);
    void setSelected(const QVector<int> &indexes);

    void onChannelChanged(Channel *ch);

    int channelsCount;
public slots:
    void setYName(const QString &yName);
signals:
    void maybeUpdateChannelProperty(int row, const QString &description, const QString &property, const QString &value);
    void legendsChanged();
    void modelChanged();
    void plotChannel(Channel *channel, bool forAllDescriptors);
    void unplotChannel(Channel *channel, bool forAllDescriptors);
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
    QCPPlot *currentPlot = nullptr;
    QVector<int> indexes;

    // QAbstractItemModel interface
public:
    virtual QStringList mimeTypes() const override;
    virtual QMimeData *mimeData(const QModelIndexList &indexes) const override;
};

#endif // CHANNELTABLEMODEL_H
