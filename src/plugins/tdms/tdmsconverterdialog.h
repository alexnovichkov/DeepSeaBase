#ifndef TDMSCONVERTERDIALOG_H
#define TDMSCONVERTERDIALOG_H

#include <QDialog>

class QProgressBar;
class QDialogButtonBox;
class QLineEdit;
class QTreeWidget;
class QPlainTextEdit;
class QPushButton;
class TDMSFileConverter;
class QThread;
class QCheckBox;
class QComboBox;
class QTreeWidgetItem;

class TDMSConverterDialog : public QDialog
{
    Q_OBJECT
public:
    TDMSConverterDialog(QWidget *parent = 0);
    ~TDMSConverterDialog();
    QStringList getConvertedFiles() const {return convertedFiles;}
    bool addFiles() const {return m_addFiles;}
public slots:
    virtual void accept();
    virtual void reject();
    void updateProgressIndicator();
private slots:
    void start();
    void stop();
    void finalize();
    void chooseFiles();
    void updateFormat();
private:
    QStringList convertedFiles;
    QString folder;
    QProgressBar *progress;
    QDialogButtonBox *buttonBox;
    QLineEdit *edit;
    QTreeWidget *tree;
    QPlainTextEdit *textEdit;
    QPushButton *button;

    QThread *thread;
    QCheckBox *openFolderButton;
    QCheckBox *addFilesButton;
    QComboBox *fileFormat;
    bool m_addFiles;
public:
    TDMSFileConverter *converter;
};


#endif // TDMSCONVERTERDIALOG_H
