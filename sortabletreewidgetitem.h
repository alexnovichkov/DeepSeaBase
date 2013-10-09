#ifndef SORTABLETREEWIDGETITEM_H
#define SORTABLETREEWIDGETITEM_H

#include <QTreeWidgetItem>
class DfdFileDescriptor;

const QString dateTimeFormat = "dd.MM.yy hh:mm:ss";

/**
 * @brief The SortableTreeWidgetItem class
 * allows sorting in the QTreeWidget according to some rules.
 * Just set type map (column, DataType)
 */

class SortableTreeWidgetItem : public QTreeWidgetItem
{
public:
    enum DataType {
        DataTypeString = 0,
        DataTypeInteger,
        DataTypeDate,
        DataTypeFloat
    };

    SortableTreeWidgetItem(QTreeWidget *tree);
    SortableTreeWidgetItem(QTreeWidget *parent, const QStringList &strings);
    SortableTreeWidgetItem(DfdFileDescriptor *dfd, const QStringList &strings);
    ~SortableTreeWidgetItem();

    static void setTypeMap(const QMap<int,DataType> &map);

    bool operator< (const QTreeWidgetItem &other) const;

    DfdFileDescriptor *dfd;
private:
    static QMap<int,DataType> typeMap;
};

#endif // SORTABLETREEWIDGETITEM_H
