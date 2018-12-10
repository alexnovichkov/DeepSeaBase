#ifndef SORTFILTERMODEL_H
#define SORTFILTERMODEL_H

#include <QSortFilterProxyModel>

class SortFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    SortFilterModel(QObject *parent = 0);
public slots:
    void setFilter(const QString &text, int column);

    // QSortFilterProxyModel interface
protected:
    virtual bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const override;
private:
    QVector<QString> filter;

    // QSortFilterProxyModel interface
protected:
    virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
};

#endif // SORTFILTERMODEL_H
