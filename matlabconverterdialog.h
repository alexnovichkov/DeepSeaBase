#ifndef MATLABCONVERTERDIALOG_H
#define MATLABCONVERTERDIALOG_H

#include <QDialog>

class QProgressBar;
class QDialogButtonBox;
class QLineEdit;
class QTreeWidget;
class QPlainTextEdit;
class QPushButton;
class MatlabConvertor;
class QThread;

class MatlabConverterDialog : public QDialog
{
    Q_OBJECT
public:
    MatlabConverterDialog(QWidget *parent = 0);
    ~MatlabConverterDialog();
    QStringList getConvertedFiles() const {return convertedFiles;}
public slots:
    virtual void accept();
    virtual void reject();
    void updateProgressIndicator();
private slots:
    void start();
    void stop();
    void finalize();
private:
    QStringList convertedFiles;
    QString folder;
    QProgressBar *progress;
    QDialogButtonBox *buttonBox;
    QLineEdit *edit;
    QTreeWidget *tree;
    QPlainTextEdit *textEdit;
    QPushButton *button;
    MatlabConvertor *convertor;
    QThread *thread;
};

#endif // MATLABCONVERTERDIALOG_H