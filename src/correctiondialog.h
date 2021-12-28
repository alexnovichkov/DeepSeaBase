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
class Channel;

class CorrectionDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CorrectionDialog(Plot *plot, QWidget *parent = 0);
    void setFiles(const QList<FileDescriptor *> &descriptors);
private slots:
    void correct();
private:
//    void
    void makeCorrectionConstant(Channel *channel);
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
