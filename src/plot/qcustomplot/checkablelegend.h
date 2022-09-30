#ifndef CHECKABLELEGEND_H
#define CHECKABLELEGEND_H

#include "qcustomplot.h"
#include <QTreeView>

class QCPLegendTreeView;
class QCPLegendModel;

#include "enums.h"

struct QCPLegendItem
{
    LegendData data;
    QCPAbstractPlottable *item;
};

class QCPLegendModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit QCPLegendModel(QObject *parent = 0);
    virtual ~QCPLegendModel();

    bool contains(QCPAbstractPlottable *item);
    QCPAbstractPlottable *item(int row);
public slots:
    void addItem(QCPAbstractPlottable *it, const LegendData &data);
    void removeItem(QCPAbstractPlottable *plotItem);
    void update(QCPAbstractPlottable *it, const LegendData &data);

    // QAbstractItemModel interface
public:
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const override;

private:
    QVector<QCPLegendItem> items;
};

class QCPLegendTreeView : public QTreeView
{
public:
    explicit QCPLegendTreeView(QWidget *parent=nullptr);
    virtual QSize sizeHint() const;
//    virtual QSize minimumSizeHint() const;
};

class QCPCheckableLegend : public QObject
{
    Q_OBJECT
public:
    QCPCheckableLegend(QWidget *parent);
    inline QWidget *widget() const {return m_treeView;}
    void addItem(QCPAbstractPlottable *item, const LegendData &data);
    void removeItem(QCPAbstractPlottable *item);
    void updateItem(QCPAbstractPlottable *item, const LegendData &data);
signals:
    //void checked( QwtPlotItem *plotItem, bool on, int index );
    void clicked(QCPAbstractPlottable*);
    void markedForDelete(QCPAbstractPlottable*);
    void markedToMove(QCPAbstractPlottable*);
    void fixedChanged(QCPAbstractPlottable*);
private:
    void handleClick( const QModelIndex &index );
    QCPLegendTreeView *m_treeView;
    QCPLegendModel *m_model;
};

#endif // CHECKABLELEGEND_H
