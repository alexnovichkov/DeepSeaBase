#include "sortabletreewidgetitem.h"

#include "fileformats/dfdfiledescriptor.h"
#include "logging.h"

QMap<int, SortableTreeWidgetItem::DataType> SortableTreeWidgetItem::typeMap = QMap<int, SortableTreeWidgetItem::DataType>();

SortableTreeWidgetItem::SortableTreeWidgetItem(QTreeWidget *tree) :
    QTreeWidgetItem(tree), fileDescriptor(0)
{DD;}

SortableTreeWidgetItem::SortableTreeWidgetItem(QTreeWidget *parent, const QStringList &strings)
    : QTreeWidgetItem (parent,strings), fileDescriptor(0)
{DD;}

SortableTreeWidgetItem::SortableTreeWidgetItem(FileDescriptor *dfd, const QStringList &strings)
    : QTreeWidgetItem (strings) , fileDescriptor(dfd)
{DD;}

SortableTreeWidgetItem::~SortableTreeWidgetItem()
{DD;
//    delete fileDescriptor;
}

void SortableTreeWidgetItem::setTypeMap(const QMap<int, SortableTreeWidgetItem::DataType> &map)
{DD;
    typeMap = map;
}

bool SortableTreeWidgetItem::operator<(const QTreeWidgetItem &other) const
{DD;
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
