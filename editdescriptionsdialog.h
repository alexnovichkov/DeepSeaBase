#ifndef EDITDESCRIPTIONSDIALOG_H
#define EDITDESCRIPTIONSDIALOG_H

#include <QDialog>

#include "filedescriptor.h"

class QTabWidget;

class EditDescriptionsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit EditDescriptionsDialog(QList<FileDescriptor *> &records, QWidget *parent = 0);
    QHash<FileDescriptor*, DescriptionList> descriptions();
signals:

public slots:

private:
    QList<FileDescriptor *> records;

    QTabWidget *tab;
};

#endif // EDITDESCRIPTIONSDIALOG_H
