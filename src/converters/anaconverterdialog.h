#ifndef ANACONVERTERDIALOG_H
#define ANACONVERTERDIALOG_H

#include <QDialog>

class QProgressBar;
class QDialogButtonBox;
class QLineEdit;
class QTreeWidget;
class QPlainTextEdit;
class AnaConverter;

class AnaConverterDialog : public QDialog
{
    Q_OBJECT
public:
    AnaConverterDialog(QWidget *parent = 0);
    ~AnaConverterDialog();
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
    AnaConverter *converter;
    QThread *thread;
};

#endif // ANACONVERTERDIALOG_H
