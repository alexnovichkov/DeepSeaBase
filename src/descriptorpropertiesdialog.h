#ifndef DESCRIPTORPROPERTIESDIALOG_H
#define DESCRIPTORPROPERTIESDIALOG_H

#include <QObject>
#include <QDialog>

class QCheckBox;
class QComboBox;
class QLineEdit;
class FileDescriptor;
class QLabel;
class QTreeWidget;
class QTabWidget;
class QTreeWidgetItem;
class QTableWidget;

class DescriptorPropertiesDialog : public QDialog
{
    Q_OBJECT
public:
    DescriptorPropertiesDialog(const QList<FileDescriptor *> &records, QWidget *parent=nullptr);
private:
    void fillFiles();
    void prev();
    void next();
    void applyToCurrent();
    void applyToAll();
    void updateState();
    void currentFileChanged(QTreeWidgetItem *cur, QTreeWidgetItem *previous);
    void cellChanged(int row, int column);
    void addProperty();
    void removeProperty();
    QList<FileDescriptor *> records;
    QTreeWidget *files;
    QTableWidget *descriptionsTable;

    int current=0;
};

#endif // DESCRIPTORPROPERTIESDIALOG_H
