#ifndef CHANNELSELECTOR_H
#define CHANNELSELECTOR_H

#include <QtCore>

class ChannelSelector
{
public:
    ChannelSelector();
    ChannelSelector(const QString &filter);
    bool includes(int index) const;

    QString filter() const {return m_filter;}
    void setFilter(const QString &filter);
    void addIndex(int index);

private:
    void recalculateIndexes();
    QString m_filter;
    QSet<int> m_indexes;
    int max_index = -1;
};

#endif // CHANNELSELECTOR_H
