#ifndef CORRECTIONDIALOG_H
#define CORRECTIONDIALOG_H

#include <QDialog>

class Plot;
class QTableWidget;
class CheckableHeaderView;
class QPushButton;
class QCheckBox;
class FileDescriptor;
class QLineEdit;
class QComboBox;
class QToolButton;

class CorrectionDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CorrectionDialog(Plot *plot, QList<FileDescriptor*> &files, QWidget *parent = 0);
    
private slots:
    void correct();
private:
    QLineEdit *edit;
    Plot *plot;
    QTableWidget *table;
    CheckableHeaderView *tableHeader;
    QToolButton *correctButton;
    QCheckBox *allFilesCheckBox;
    QComboBox *correctionType;
    QList<FileDescriptor*> files;

    // QDialog interface
public slots:
    virtual void accept() override;
    virtual void reject() override;
};

#endif // CORRECTIONDIALOG_H
