#ifndef CHANNELSELECTOR_H
#define CHANNELSELECTOR_H

#include <QtCore>

class ChannelSelector
{
public:
    enum class Selection {
        All,
        Some
    };
    ChannelSelector();
    ChannelSelector(const QString &filter);
    bool includes(int index) const;

    QString filter() const {return mFilter;}
    Selection selection() const {return mSelection;}
    void setFilter(const QString &filter);
    void setSelection(Selection selection);

    //список индексов из фильтра, нумерация с 1
    QStringList indexes() const;

private:
    void addIndex(int index);
    void recalculateIndexes();
    QString mFilter;
    QSet<int> mIndexes;
    Selection mSelection = Selection::All;
    int maxIndex = -1;
};

#endif // CHANNELSELECTOR_H
