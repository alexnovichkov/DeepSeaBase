#ifndef CONVERTDIALOG_H
#define CONVERTDIALOG_H

#include <QDialog>
#include <QtCore>

class AbstractMethod;

class QComboBox;
class QSpinBox;
class QStackedWidget;
class QLabel;
class FileDescriptor;
class DfdFileDescriptor;
class QProgressBar;
class QCheckBox;
class QThread;
class Converter;
class QDialogButtonBox;
class QPlainTextEdit;
class TaskBarProgress;
class QLineEdit;

class CalculateSpectreDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CalculateSpectreDialog(QList<FileDescriptor *> &dataBase, QWidget *parent = 0);
    ~CalculateSpectreDialog();
    QStringList getNewFiles() const {return newFiles;}

public slots:
    void methodChanged(int method);
    virtual void accept();
    virtual void reject();
private slots:
    void updateProgressIndicator(const QString &path);
    void updateProgressIndicator();
    void start();
    void stop();
private:
    QList<FileDescriptor *> dataBase;

    QStringList newFiles;
    AbstractMethod *currentMethod = 0;

    QComboBox *methodCombo;
    QStackedWidget *methodsStack;
    QPlainTextEdit *infoLabel;
//    QCheckBox *useDeepsea;
    QDialogButtonBox *buttonBox;
    QProgressBar *progress;
    QCheckBox *shutdown;

    QLineEdit *channelsFilter;

    Converter *converter = nullptr;
    QThread *thread = nullptr;
    TaskBarProgress *taskBarProgress = nullptr;
    QWidget *win;
};

#endif // CONVERTDIALOG_H
