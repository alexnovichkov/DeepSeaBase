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

    //список индексов из фильтра, нумерация с 1
    QStringList indexes() const;

private:
    void addIndex(int index);
    void recalculateIndexes();
    QString m_filter = QString("все");
    QSet<int> m_indexes;
    int max_index = -1;
};

#endif // CHANNELSELECTOR_H
