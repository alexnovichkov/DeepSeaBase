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

class UffConvertor : public QObject
{
    Q_OBJECT
public:
    UffConvertor(QObject *parent = 0);
    void setFilesToConvert(const QStringList &toConvert) {filesToConvert = toConvert;}

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
};

class ConverterDialog : public QDialog
{
    Q_OBJECT
public:
    ConverterDialog(QList<FileDescriptor *> dataBase, QWidget *parent = 0);
    ~ConverterDialog();
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
    void addFile(const QString &fileName);
    QStringList convertedFiles;
    QString folder;
    QProgressBar *progress;
    QDialogButtonBox *buttonBox;
    QTreeWidget *tree;
    QPlainTextEdit *textEdit;
    QPushButton *button;
    UffConvertor *convertor;
    QThread *thread;
};

#endif // UFFCONVERTERDIALOG_H
