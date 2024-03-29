#ifndef MATLABCONVERTERDIALOG_H
#define MATLABCONVERTERDIALOG_H

#include <QDialog>
#include <QStyledItemDelegate>
#include <QFileInfoList>

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
class QTreeWidgetItem;

class ComboBoxItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit ComboBoxItemDelegate(MatlabConvertor *convertor, QObject *parent = 0)
        : QStyledItemDelegate(parent), convertor(convertor) {}

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const;

    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const;
    virtual void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
private:
    MatlabConvertor *convertor;
};



class MatlabConverterDialog : public QDialog
{
    Q_OBJECT
public:
    MatlabConverterDialog(QWidget *parent = 0);
    ~MatlabConverterDialog();
    QStringList getConvertedFiles() const {return convertedFiles;}
    bool addFiles() const {return m_addFiles;}
signals:
    void filesConverted(const QStringList &files);
public slots:
    virtual void accept();
    virtual void reject();
    void updateProgressIndicator();
private slots:
    void start();
    void stop();
    void finalize();
    void chooseMatFiles();
    void updateFormat();
    void converted(const QString &file);
//    void editItem(QTreeWidgetItem*item, int column);
private:
    QString findXmlFile(bool silent) const; // silent means it won't ask for a file via dialog
    QFileInfoList findMatFiles(const QString &folder);
    QStringList convertedFiles;
    QString folder;
    QProgressBar *progress;
    QDialogButtonBox *buttonBox;
    QLineEdit *edit;
    QTreeWidget *tree;
    QPlainTextEdit *textEdit;
    QPushButton *button;

    QComboBox *formatBox;

    QThread *thread;
    QCheckBox *openFolderButton;
    QCheckBox *addFilesButton;
    QComboBox *channelTypeCombo;
    bool m_addFiles;
public:
    MatlabConvertor *convertor;
};

#endif // MATLABCONVERTERDIALOG_H
