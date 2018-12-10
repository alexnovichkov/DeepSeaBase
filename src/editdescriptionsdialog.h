#ifndef EDITDESCRIPTIONSDIALOG_H
#define EDITDESCRIPTIONSDIALOG_H

#include <QDialog>

#include "filedescriptor.h"

class QTabWidget;
class QStackedWidget;

class EditDescriptionsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit EditDescriptionsDialog(QList<FileDescriptor *> &records, QWidget *parent = 0);
    QHash<FileDescriptor*, DescriptionList> descriptions();
signals:

public slots:

private:
    QTabWidget *tab;
    QStackedWidget *stack;
};

#endif // EDITDESCRIPTIONSDIALOG_H
