#include "channelselector.h"

ChannelSelector::ChannelSelector() = default;


ChannelSelector::ChannelSelector(const QString &filter) : m_filter(filter)
{
    recalculateIndexes();
}

bool ChannelSelector::includes(int index) const
{
    if (m_filter.isEmpty() || m_filter == "все") return true; // пустой фильтр - все каналы
    return (m_indexes.contains(index));
}

void ChannelSelector::setFilter(const QString &filter)
{
    if (m_filter == filter) return;

    m_filter = filter;
    recalculateIndexes();
}

void ChannelSelector::addIndex(int index)
{
    m_indexes << index;
    if (max_index < index) max_index = index;
}

QString ChannelSelector::indexesAsString() const
{
    QStringList result;
    for (int index: m_indexes) result << QString::number(index+1);
    return result.join(",");
}

void ChannelSelector::recalculateIndexes()
{
    m_indexes.clear();
    max_index = -1;
    if (m_filter.isEmpty() || m_filter == "все") return;

    bool ok;
    const QStringList filters = m_filter.split(",",QString::SkipEmptyParts);
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
