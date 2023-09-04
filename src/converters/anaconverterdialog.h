#ifndef ANACONVERTERDIALOG_H
#define ANACONVERTERDIALOG_H

#include <QDialog>

class QProgressBar;
class QDialogButtonBox;
class QLineEdit;
class QTreeWidget;
class QPlainTextEdit;
class AnaConverter;
class QCheckBox;
class QComboBox;

class AnaConverterDialog : public QDialog
{
    Q_OBJECT
public:
    AnaConverterDialog(QWidget *parent = 0);
    ~AnaConverterDialog();
    QStringList getConvertedFiles() const {return convertedFiles;}
    bool addFiles() const;
signals:
    void filesConverted(const QStringList &files);
public slots:
    virtual void accept();
    virtual void reject();
    void updateProgressIndicator();
private slots:
    void chooseFiles();
    void start();
    void stop();
    void finalize();
    void updateFormat();
    void setTargetFolder();
private:
    QStringList convertedFiles;
    QString folder;
    QProgressBar *progress;
    QDialogButtonBox *buttonBox;
    QComboBox *fileFormat;
    QLineEdit *edit;
    QLineEdit *targetFolderEdit;
    QPushButton *targetButton;
    QTreeWidget *tree;
    QPlainTextEdit *textEdit;
    QPushButton *button;
    AnaConverter *converter = nullptr;
    QThread *thread = nullptr;
    QCheckBox *addFilesButton;
    QCheckBox *openFolderButton;
    QCheckBox *trimFilesButton;
    QComboBox *dataFormat;
};

#endif // ANACONVERTERDIALOG_H
