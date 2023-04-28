#ifndef UFFCONVERTERDIALOG_H
#define UFFCONVERTERDIALOG_H

#include <QDialog>
#include <QFileInfoList>

class QProgressBar;
class QDialogButtonBox;
class QLineEdit;
class QTreeWidget;
class QPlainTextEdit;
class QPushButton;
class QThread;
class FileDescriptor;
class QComboBox;
class QCheckBox;

class FileConverter : public QObject
{
    Q_OBJECT
public:
    FileConverter(QObject *parent = 0);
    void setFilesToConvert(const QStringList &toConvert) {filesToConvert = toConvert;}
    void setDestinationFormat(const QString &format) {destinationFormat = format;}

    QStringList getUffFiles() const;
    QStringList getNewFiles() const {return newFiles;}
public slots:
    bool convert();
signals:
    void tick();
    void finished();
    void message(const QString &s);
private:
    QFileInfoList uffFiles;
    QStringList newFiles;
    QStringList filesToConvert;
    QString destinationFormat;
};

class ConverterDialog : public QDialog
{
    Q_OBJECT
public:
    ConverterDialog(QList<FileDescriptor *> dataBase, QWidget *parent = 0);
    ~ConverterDialog();
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
private:
    void addFile(const QString &fileName);
    QStringList convertedFiles;
    QString folder;
    bool m_addFiles = false;
    QProgressBar *progress;
    QDialogButtonBox *buttonBox;
    QTreeWidget *tree;
    QPlainTextEdit *textEdit;
    QPushButton *button;
    FileConverter *convertor;
    QComboBox *formatBox;
    QThread *thread;
    QCheckBox *addFilesButton;
};

#endif // UFFCONVERTERDIALOG_H
