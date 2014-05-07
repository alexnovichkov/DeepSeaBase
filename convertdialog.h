#ifndef CONVERTDIALOG_H
#define CONVERTDIALOG_H

#include <QDialog>

class AbstractMethod;

class QComboBox;
class QSpinBox;
class QStackedWidget;
class QProcess;
class DfdFileDescriptor;

class ConvertDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ConvertDialog(QList<DfdFileDescriptor *> *dataBase, QWidget *parent = 0);
    ~ConvertDialog();
    QStringList getNewFiles() const {return newFiles;}
    QStringList getSpfFile(const QVector<int> &indexes, QString dir);
signals:
    
public slots:
    void methodChanged(int method);
    virtual void accept();
private:
    bool copyFilesToTempDir(const QVector<int> &, const QString &tempFolderName);
    void moveFilesFromTempDir(const QString &destDir);
    QList<DfdFileDescriptor *> *dataBase;

    QStringList newFiles;

    QVector<int> channels;

    AbstractMethod *currentMethod;

    QComboBox *methodCombo;
    QSpinBox *activeChannelSpin;
    QSpinBox *baseChannelSpin;
    QStackedWidget *methodsStack;
    QComboBox *activeStripCombo;

    QProcess *process;
};

#endif // CONVERTDIALOG_H
