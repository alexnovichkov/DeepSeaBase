#ifndef FILESTABLE_H
#define FILESTABLE_H

#include <QObject>
#include <QTreeView>

class FilesTable : public QTreeView
{
    Q_OBJECT
public:
    FilesTable(QWidget *parent = nullptr);
};

#endif // FILESTABLE_H
