#ifndef ESOCONVERTERDIALOG_H
#define ESOCONVERTERDIALOG_H

#include <QDialog>

class QProgressBar;
class QDialogButtonBox;
class QLineEdit;
class QTreeWidget;
class QPlainTextEdit;
class QPushButton;
class QThread;

#include <QObject>
#include <QFileInfoList>

class EsoConvertor : public QObject
{
    Q_OBJECT
public:
    EsoConvertor(QObject *parent = 0);
    //void setFolder(const QString &folder);
    void setFilesToConvert(const QStringList &toConvert) {filesToConvert = toConvert;}
    void setChannelNames(const QStringList &names) {channelNames = names;}
    void setResultFile(const QString &file) {dfdFileName = file;}

    //QStringList getMatFiles() const;
    QString getNewFile() const {return dfdFileName;}
    void setColumn(int column, bool use) {columns[column]=use;}
public slots:
    bool convert();
signals:
    void tick();
    void finished();
    void message(const QString &s);
private:
    QString dfdFileName;
    QStringList filesToConvert;
    QStringList channelNames;
    QVector<bool> columns;
};

class EsoConverterDialog : public QDialog
{
    Q_OBJECT
public:
    EsoConverterDialog(QWidget *parent = 0);
    ~EsoConverterDialog();
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
    EsoConvertor *convertor;
    QThread *thread;
};

#endif // ESOCONVERTERDIALOG_H
