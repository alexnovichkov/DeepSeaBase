#ifndef LEGEND_H
#define LEGEND_H

#include "qwt_legend.h"
#include <QSet>
#include <QAbstractTableModel>
#include <QTreeView>

// Этот класс используется при импорте графиков в рисунок или при печати графика
class Legend : public QwtLegend
{
    Q_OBJECT
public:
    explicit Legend(QWidget *parent = 0);
signals:
    void markedForDelete( const QVariant &itemInfo, int index );
    void markedToMoveToRight( const QVariant &itemInfo, int index );

private slots:
    void itemMarkedForDelete();
    void itemMarkedToMoveToRight();
protected:
    virtual QWidget *createWidget( const QwtLegendData & ) const;
    virtual void updateWidget( QWidget *widget, const QwtLegendData &data );
};



class QStandardItem;
class QModelIndex;
class QwtPlotItem;


struct LegendItem
{
    QString text;
    QColor color;
    bool checked = true;
    bool selected = false;
    QwtPlotItem *item;
};

class LegendModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit LegendModel(QObject *parent = 0);
    virtual ~LegendModel();

    bool contains(QwtPlotItem *item);
    QwtPlotItem *item(int row);
public slots:
    void addItem(QwtPlotItem *it);
    void removeItem(QwtPlotItem *plotItem);
    void update(QwtPlotItem *it, const QString &text, const QColor& color, bool checked, bool selected);

    // QAbstractItemModel interface
public:
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    virtual Qt::ItemFlags flags(const QModelIndex &index) const override;

private:
    QVector<LegendItem> items;

};

class LegendTreeView : public QTreeView
{
public:
    explicit LegendTreeView(QWidget *parent);
    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;
};


// Этот класс используется для отображения графиков на экране

class CheckableLegend : public QwtAbstractLegend
{
    Q_OBJECT

public:
    explicit CheckableLegend( QWidget *parent = NULL );
    virtual ~CheckableLegend();

    virtual void renderLegend(QPainter *painter, const QRectF &rect, bool fillBackground) const;

    virtual bool isEmpty() const;

    virtual int scrollExtent( Qt::Orientation ) const;

signals:
    //void checked( QwtPlotItem *plotItem, bool on, int index );
    void clicked(QwtPlotItem *);
    void markedForDelete(QwtPlotItem*);
    void markedToMove(QwtPlotItem*);

public slots:
    virtual void updateLegend(const QVariant &itemInfo, const QList<QwtLegendData> & data);

private slots:
    void handleClick( const QModelIndex & );

private:
    void updateItem(QwtPlotItem *item, const QwtLegendData &data);

    LegendTreeView *d_treeView;
    LegendModel *d_model;
};

#endif // LEGEND_H
