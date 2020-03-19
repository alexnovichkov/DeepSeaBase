#include "sortfiltermodel.h"
#include <QtCore>
#include "logging.h"

SortFilterModel::SortFilterModel(QObject *parent) : QSortFilterProxyModel(parent)
{DD;
    filter.resize(10);
}

void SortFilterModel::setFilter(const QString &text, int column)
{DD;
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
{DD;
    bool accept = true;
    for (int i=0; i<sourceModel()->columnCount(); ++i) {
        if (filter.at(i).isEmpty()) continue;
        QModelIndex index = sourceModel()->index(source_row, i, source_parent);
        QVariant data = sourceModel()->data(index);
        if (data.type()==QVariant::Double)
            accept &= qFuzzyCompare(data.toDouble(),filter.at(i).toDouble());
        else if (data.type()==QVariant::Int)
            accept &= (data.toInt()==filter.at(i).toInt());
        else if (data.type()==QVariant::LongLong)
            accept &= (data.toLongLong()==filter.at(i).toLongLong());
        else if (data.type()==QVariant::UInt)
            accept &= (data.toUInt()==filter.at(i).toUInt());
        else if (data.type()==QVariant::ULongLong)
            accept &= (data.toULongLong()==filter.at(i).toULongLong());
        else accept &= data.toString().contains(filter.at(i), Qt::CaseInsensitive);
    }

    return accept;
}
