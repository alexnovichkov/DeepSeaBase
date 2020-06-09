#ifndef CHANNELTABLEMODEL_H
#define CHANNELTABLEMODEL_H

#include <QObject>
#include <QAbstractTableModel>
#include <QFont>

class Channel;
class FileDescriptor;

class ChannelTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    ChannelTableModel(QObject *parent);
    Channel *channel(int index);
    QVector<int> selected()  const {return indexes;}
    QVector<int> plotted() const;

    void setYName(const QString &yName);

    void clear();
    void deleteCurves();
    void plotChannels(const QVector<int> &toPlot, bool plotOnRight = false);

    void setDescriptor(FileDescriptor *dfd);
    void setSelected(const QVector<int> &indexes);

    void onCurveChanged(Channel *ch);

    int channelsCount;
signals:
    void maybeUpdateChannelDescription(int row, const QString &value);
    void maybeUpdateChannelName(int row, const QString &value);
    void maybePlot(int row);
    void deleteCurve(int row);
    void updateLegends();
    void modelChanged();
    // QAbstractItemModel interface
public:
    virtual int rowCount(const QModelIndex &parent) const override;
    virtual int columnCount(const QModelIndex &parent) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    virtual bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role) override;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const override;
private:
    FileDescriptor *descriptor;
    QVector<int> indexes;

    QFont uFont;
    QFont bFont;
};

#endif // CHANNELTABLEMODEL_H
