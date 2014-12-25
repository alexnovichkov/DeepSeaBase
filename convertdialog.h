#ifndef CONVERTDIALOG_H
#define CONVERTDIALOG_H

#include <QDialog>
#include <QtCore>

class AbstractMethod;

class QComboBox;
class QSpinBox;
class QStackedWidget;
class QProcess;
class QLabel;
class FileDescriptor;
class DfdFileDescriptor;
class QProgressBar;
class DfdChannel;
class Parameters;
class QCheckBox;

class ConvertDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ConvertDialog(QList<FileDescriptor *> *dataBase, QWidget *parent = 0);
    ~ConvertDialog();
    QStringList getNewFiles() const {return newFiles;}
    QStringList getSpfFile(QString dir);
signals:
    
public slots:
    void methodChanged(int method);
    virtual void accept();
    virtual void reject();
private slots:
    void updateProgressIndicator(const QString &path);
private:
    void moveFilesFromTempDir(const QString &tempFolderName, QString destDir);
    int stripByBandwidth(double bandwidth);

    QString createUniqueFileName(const QString &tempFolderName, const QString &fileName);

    void convert(DfdFileDescriptor *dfd, const QString &tempFolderName);
    void applyWindow(QVector<float> &values, const Parameters &p);
    QVector<double> computeSpectre(const QVector<float> &values, const Parameters &p);

    QList<DfdFileDescriptor *> dataBase;

    QString tempFolderName;
    QDateTime dt;

    QStringList newFiles;
    QStringList newFiles_;

    QVector<int> channels;

    AbstractMethod *currentMethod;

    QComboBox *methodCombo;
    QSpinBox *activeChannelSpin;
    QSpinBox *baseChannelSpin;
    QStackedWidget *methodsStack;
    QComboBox *activeStripCombo;
    QLabel *infoLabel;
    QSpinBox *overlap;
    QCheckBox *useDeepsea;

    QProcess *process;
    QProgressBar *progress;

    double bandWidth;
};

#endif // CONVERTDIALOG_H
