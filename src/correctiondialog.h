#ifndef CORRECTIONDIALOG_H
#define CORRECTIONDIALOG_H

#include <QDialog>

class Plot;
class QTableView;
class HeaderView;
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
    ~CorrectionDialog();
    void setPlot(Plot *plot);
    void setFiles(const QList<FileDescriptor *> &descriptors);
private slots:
    void correct();
//protected:
//    void closeEvent(QCloseEvent *event) override;
private:
//    void
    void makeCorrectionConstant(Channel *channel);
    QLineEdit *edit;
    Plot *plot;
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
