#include "channelselector.h"
#include "logging.h"

ChannelSelector::ChannelSelector() = default;


ChannelSelector::ChannelSelector(const QString &filter) : mFilter(filter)
{DD;
    recalculateIndexes();
}

bool ChannelSelector::includes(int index) const
{DD;
    if (mFilter.isEmpty() || mSelection == Selection::All)
        return true; // пустой фильтр - все каналы

    return (mIndexes.contains(index));
}

void ChannelSelector::setFilter(const QString &filter)
{DD;
    if (mFilter == filter) return;

    mFilter = filter;
    recalculateIndexes();
}

void ChannelSelector::setSelection(ChannelSelector::Selection selection)
{
    if (mSelection == selection) return;

    mSelection = selection;
    recalculateIndexes();
}

void ChannelSelector::addIndex(int index)
{DD;
    mIndexes << index;
    if (maxIndex < index) maxIndex = index;
}

QStringList ChannelSelector::indexes() const
{DD;
    QStringList result;
    for (int i: qAsConst(mIndexes)) result << QString::number(i+1);
    return result;
}

void ChannelSelector::recalculateIndexes()
{DD;
    mIndexes.clear();
    maxIndex = -1;
    if (mFilter.isEmpty() || mSelection == Selection::All) return;

    bool ok;
    const QStringList filters = mFilter.split(",",QString::SkipEmptyParts);
    for (const QString &filter: filters) {
        if (filter.contains("-")) {
            QStringList range = filter.split("-");
            int min=-1, max=-1;
            if (range.at(0).trimmed().isEmpty()) min=0;
            else {
                int index = range.at(0).trimmed().toInt(&ok)-1;
                if (ok) min=index;
            }
            if (range.at(1).trimmed().isEmpty()) max = 1000;
            else {
                int index = range.at(1).trimmed().toInt(&ok)-1;
                if (ok) max=index;
            }
            if (min>max) std::swap(min, max);
            for (int i=min; i<=max; ++i) addIndex(i);
        } else {
            int index = filter.toInt(&ok)-1;
            if (ok) addIndex(index);
        }
    }
}
