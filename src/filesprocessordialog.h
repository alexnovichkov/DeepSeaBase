#ifndef FILESPROCESSORDIALOG_H
#define FILESPROCESSORDIALOG_H

#include <QtWidgets>

class FileDescriptor;
class TaskBarProgress;
class QTreeWidget;
class QListWidget;
class QtTreePropertyBrowser;
class AbstractFunction;
class AbstractAlgorithm;

#include <QtVariantPropertyManager>
#include <QtVariantEditorFactory>

class FilesProcessorDialog : public QDialog
{
    Q_OBJECT
public:
    explicit FilesProcessorDialog(QList<FileDescriptor *> &dataBase, QWidget *parent = 0);
    ~FilesProcessorDialog();
    QStringList getNewFiles() const {return newFiles;}

public slots:
    void methodChanged(QTreeWidgetItem*item);
    virtual void accept();
    virtual void reject();
private slots:
    void updateProgressIndicator(const QString &path);
    void updateProgressIndicator();
    void start();
    void stop();
    void onValueChanged(QtProperty *property, const QVariant &val);
    void updateProperty(AbstractFunction *f, const QString &property, const QVariant &val, const QString &attribute);
private:
    struct Property {
        AbstractFunction *f;
        QString name;
    };


    void addProperties(AbstractFunction *f);
    void updateVisibleProperties();

    QList<FileDescriptor *> dataBase;

    QStringList newFiles;

    QPlainTextEdit *infoLabel;
    QDialogButtonBox *buttonBox;
    QProgressBar *progress;
    QTreeWidget *functionsList;
    QTreeWidget *filesTree;

    QtTreePropertyBrowser *propertyTree;
    QtVariantPropertyManager *m_manager;
    QtVariantEditorFactory *m_factory;

    QCheckBox *shutdown;

    QThread *thread_;
    TaskBarProgress *taskBarProgress;
    QWidget *win;
    QList<AbstractAlgorithm*> algorithms;
    AbstractAlgorithm * currentAlgorithm;
    QMap<QtVariantProperty*, Property> map;
};

#endif // FILESPROCESSORDIALOG_H
