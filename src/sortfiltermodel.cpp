#include "sortfiltermodel.h"
#include <QtCore>

SortFilterModel::SortFilterModel(QObject *parent) : QSortFilterProxyModel(parent)
{
    filter.resize(10);
}

void SortFilterModel::setFilter(const QString &text, int column)
{
    if (filter.at(column) == text) return;
    filter[column] = text;
    invalidateFilter();
}


bool SortFilterModel::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const
{
    QVariant leftData = sourceModel()->data(source_left);
    QVariant rightData = sourceModel()->data(source_right);

    if (leftData.type() == QVariant::DateTime)
        return leftData.toDateTime() < rightData.toDateTime();
    else if (leftData.type() == QVariant::Int)
        return leftData.toInt() < rightData.toInt();
    else if (leftData.type() == QVariant::Double)
        return leftData.toDouble() < rightData.toDouble();

    return QString::localeAwareCompare(leftData.toString(), rightData.toString()) < 0;
}


bool SortFilterModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    bool accept = true;
    for (int i=0; i<sourceModel()->columnCount(); ++i) {
        if (filter.at(i).isEmpty()) continue;
        QModelIndex index = sourceModel()->index(source_row, i, source_parent);
        accept &= sourceModel()->data(index).toString().contains(filter.at(i), Qt::CaseInsensitive);
    }

    return accept;
}
