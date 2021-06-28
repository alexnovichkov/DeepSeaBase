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
class QFormLayout;

struct DescriptorProperty
{
    QCheckBox *checked;
    QComboBox *property;
    QLineEdit *edit;
};

class DescriptorPropertiesDialog : public QDialog
{
    Q_OBJECT
public:
    DescriptorPropertiesDialog(const QList<FileDescriptor *> &records, QWidget *parent);
private:
    void fillFiles();
    void prev();
    void next();
    void applyToCurrent();
    void applyToAll();
    void updateState();
    void currentFileChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);
//    QVector<DescriptorProperty> properties;
    QList<FileDescriptor *> records;
    QLabel *file;
    QTreeWidget *files;
    QFormLayout *propertiesFL;

//    QPushButton * prevButton;
//    QPushButton * nextButton;
//    QPushButton * applyToCurrButton;
//    QPushButton * applyToAllButton;
    int current=0;
};

#endif // DESCRIPTORPROPERTIESDIALOG_H
