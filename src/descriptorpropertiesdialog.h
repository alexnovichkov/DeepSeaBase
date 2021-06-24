#ifndef DESCRIPTORPROPERTIESDIALOG_H
#define DESCRIPTORPROPERTIESDIALOG_H

#include <QObject>
#include <QDialog>

class QCheckBox;
class QComboBox;
class QLineEdit;
class FileDescriptor;
class QLabel;

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
    DescriptorPropertiesDialog(QList<FileDescriptor *> &records, QWidget *parent);
private:
    void prev();
    void next();
    void applyToCurrent();
    void applyToAll();
    void updateState();
    QVector<DescriptorProperty> properties;
    QList<FileDescriptor *> records;
    QLabel *file;

    QPushButton * prevButton;
    QPushButton * nextButton;
    QPushButton * applyToCurrButton;
    QPushButton * applyToAllButton;
    int current=0;
};

#endif // DESCRIPTORPROPERTIESDIALOG_H
