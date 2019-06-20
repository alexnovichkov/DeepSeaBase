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
class QCheckBox;
class QComboBox;

class MatlabConverterDialog : public QDialog
{
    Q_OBJECT
public:
    MatlabConverterDialog(QWidget *parent = 0);
    ~MatlabConverterDialog();
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
    void chooseMatFiles();
private:
    QString findXmlFile() const;
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
    QCheckBox *openFolderButton;
    QCheckBox *addFilesButton;
    QComboBox *rawFileFormat;
    bool m_addFiles;
};

#endif // MATLABCONVERTERDIALOG_H
