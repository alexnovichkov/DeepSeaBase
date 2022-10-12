#ifndef CHECKABLELEGEND_H
#define CHECKABLELEGEND_H

#include "qcustomplot.h"
#include <QTreeView>

class QCPLegendTreeView;
class QCPLegendModel;
class Curve;

#include "enums.h"

struct QCPLegendItem
{
    LegendData data;
    Curve *item;
};

class QCPLegendModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit QCPLegendModel(QObject *parent = 0);
    virtual ~QCPLegendModel();

    bool contains(Curve *item);
    Curve *item(int row);
signals:
    void visibilityChanged(Curve *, bool);
public slots:
    void addItem(Curve *it, const LegendData &data);
    void removeItem(Curve *plotItem);
    void update(Curve *it, const LegendData &data);

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
    void addItem(Curve *item, const LegendData &data);
    void removeItem(Curve *item);
    void updateItem(Curve *item, const LegendData &data);
signals:
    //void checked( QwtPlotItem *plotItem, bool on, int index );
    void clicked(Curve*);
    void markedForDelete(Curve*);
    void markedToMove(Curve*);
    void fixedChanged(Curve*);
    void visibilityChanged(Curve *, bool);
private:
    void handleClick( const QModelIndex &index );
    QCPLegendTreeView *m_treeView;
    QCPLegendModel *m_model;
};

#endif // CHECKABLELEGEND_H
