#include "sortfiltermodel.h"
#include <QtCore>
#include "logging.h"
#include "model.h"
#include "algorithms.h"

SortFilterModel::SortFilterModel(QObject *parent) : QSortFilterProxyModel(parent)
{DD;
    filters.resize(MODEL_COLUMNS_COUNT);
    collator.setNumericMode(true);
}

void SortFilterModel::setFilter(const QString &text, int column)
{DD;
    if (filters.at(column) == text) return;
    filters[column] = text;
    invalidateFilter();
}

QString SortFilterModel::filter(int column) const
{DD;
    return filters.value(column);
}


bool SortFilterModel::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const
{DD;
    QVariant leftData = sourceModel()->data(source_left);
    QVariant rightData = sourceModel()->data(source_right);

    if (leftData.type() == QVariant::DateTime)
        return leftData.toDateTime() < rightData.toDateTime();
    else if (leftData.type() == QVariant::Int)
        return leftData.toInt() < rightData.toInt();
    else if (leftData.type() == QVariant::Double)
        return leftData.toDouble() < rightData.toDouble();

    return collator.compare(leftData.toString(), rightData.toString()) < 0;
}


bool SortFilterModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{DD;
    bool accept = true;
    for (int i=0; i<sourceModel()->columnCount(); ++i) {
        if (filters.at(i).isEmpty()) continue;
        QModelIndex index = sourceModel()->index(source_row, i, source_parent);
        QVariant data = sourceModel()->data(index);
        if (data.type()==QVariant::Double) {
            QString s = filters.at(i);
            s.replace(',', '.');
            if (!s.contains('.')) {
                accept &= (int(std::floor(data.toDouble())) == filters.at(i).toInt());
            }
            else {
                int precision = s.section('.', 1).length();
                double d = data.toDouble() * std::pow(10, precision);
                accept &= qFuzzyCompare(std::floor(d), std::floor(s.toDouble() * std::pow(10, precision)));
            }
        }
        else if (data.type()==QVariant::Int)
            accept &= (data.toInt()==filters.at(i).toInt());
        else if (data.type()==QVariant::LongLong)
            accept &= (data.toLongLong()==filters.at(i).toLongLong());
        else if (data.type()==QVariant::UInt)
            accept &= (data.toUInt()==filters.at(i).toUInt());
        else if (data.type()==QVariant::ULongLong)
            accept &= (data.toULongLong()==filters.at(i).toULongLong());
        else accept &= data.toString().contains(filters.at(i), Qt::CaseInsensitive);
    }

    return accept;
}
