#ifndef CORRECTIONDIALOG_H
#define CORRECTIONDIALOG_H

#include <QDialog>

class QTableView;
class HeaderView;
class QPushButton;
class QCheckBox;
class FileDescriptor;
class QLineEdit;
class QComboBox;
class QToolButton;
class Channel;
class QCPPlot;

class CorrectionDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CorrectionDialog(QCPPlot *plot, QWidget *parent = 0);
    ~CorrectionDialog();
    void setFiles(const QList<FileDescriptor *> &descriptors);
private slots:
    void correct();
//protected:
//    void closeEvent(QCloseEvent *event) override;
private:
//    void
    void makeCorrectionConstant(Channel *channel);
    QLineEdit *edit;
    QCPPlot *plot;
    QTableView *table;
    HeaderView *tableHeader;
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
