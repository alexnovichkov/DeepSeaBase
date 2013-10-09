#include "sortabletreewidgetitem.h"

#include "dfdfiledescriptor.h"

QMap<int, SortableTreeWidgetItem::DataType> SortableTreeWidgetItem::typeMap = QMap<int, SortableTreeWidgetItem::DataType>();

SortableTreeWidgetItem::SortableTreeWidgetItem(QTreeWidget *tree) :
    QTreeWidgetItem(tree), dfd(0)
{}

SortableTreeWidgetItem::SortableTreeWidgetItem(QTreeWidget *parent, const QStringList &strings)
    : QTreeWidgetItem (parent,strings), dfd(0)
{}

SortableTreeWidgetItem::SortableTreeWidgetItem(DfdFileDescriptor *dfd, const QStringList &strings)
    : QTreeWidgetItem (strings) , dfd(dfd)
{}

SortableTreeWidgetItem::~SortableTreeWidgetItem()
{
    delete dfd;
}

void SortableTreeWidgetItem::setTypeMap(const QMap<int, SortableTreeWidgetItem::DataType> &map)
{
    typeMap = map;
}

bool SortableTreeWidgetItem::operator<(const QTreeWidgetItem &other) const
{
    int sortCol = treeWidget()->sortColumn();
    QString s = text(sortCol);
    const DataType dataType = typeMap.value(sortCol);
    switch (dataType) {
        case DataTypeInteger:
            return s.toInt() < other.text(sortCol).toInt();
        case DataTypeDate:
            return QDateTime::fromString(s, dateTimeFormat)
                    < QDateTime::fromString(other.text(sortCol), dateTimeFormat);
        case DataTypeFloat:
            return s.toDouble() < other.text(sortCol).toDouble();
        default:
            break;
    }
    return QTreeWidgetItem::operator <(other);
}
