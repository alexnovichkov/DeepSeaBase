#ifndef DFDFILTERPROXY_H
#define DFDFILTERPROXY_H

#include <QSortFilterProxyModel>
#include <QFileSystemModel>
#include "logging.h"
#include "fileformats/filedescriptor.h"
#include "fileformats/formatfactory.h"


class DfdFilterProxy : public QSortFilterProxyModel
{
public:
    DfdFilterProxy(FileDescriptor *filter, QObject *parent)
        : QSortFilterProxyModel(parent), filter(filter)
    {DD;
        if (filter) {
            filterByContent = true;
        }
    }
protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
    {DD;
        QFileSystemModel *model = qobject_cast<QFileSystemModel *>(sourceModel());
        if (!model) return false;

        QModelIndex index0 = model->index(source_row, 0, source_parent);

        QFileInfo fi = model->fileInfo(index0);

        if (fi.isFile()) {
            if (suffixes.contains(fi.suffix().toLower())) {
                //принимаем все файлы, если не сравниваем с конкретным
                if (!filterByContent)
                    return true;

                QScopedPointer<FileDescriptor> descriptor(FormatFactory::createDescriptor(fi.canonicalFilePath()));

                if (descriptor->canTakeAnyChannels())
                    return true;

                descriptor->read();
                //частный случай: мы можем записать данные из SourceData в CuttedData, преобразовав их в floats,
                //но не наоборот
                return descriptor->canTakeChannelsFrom(filter);
            }
            else //не файлы dfd, uff, d94
                return false;
        }
        else //папки
            return true;
    }
private:
    FileDescriptor *filter;
    bool filterByContent = false;
    QStringList suffixes = FormatFactory::allSuffixes(true);
};

#endif // DFDFILTERPROXY_H
