#ifndef CHANNELPROPERTIESDIALOG_H
#define CHANNELPROPERTIESDIALOG_H

#include <QObject>
#include <QDialog>

class QCheckBox;
class QComboBox;
class QLineEdit;
class Channel;
class QLabel;
class QTreeWidget;
class QTabWidget;
class QTreeWidgetItem;
class QTableWidget;
class QFormLayout;
class QToolButton;
class QtTreePropertyBrowser;
class QtVariantPropertyManager;
class QtVariantEditorFactory;

struct CurrentEdited {
    int index = -1;
    QString value;
};

struct ChannelProperty
{
    QString displayName;
    QString name;
    QLineEdit* edit = nullptr;
//    QToolButton *writeAllBtn = nullptr;
    QCheckBox *check = nullptr;
};
struct DataProperty
{
    QString displayName;
    QLineEdit* label = nullptr;
};

class ChannelPropertiesDialog : public QDialog
{
    Q_OBJECT
public:
    ChannelPropertiesDialog(const QVector<Channel *> &channels, QWidget *parent=nullptr);
private:
    void fillFiles();
    void prev();
    void next();
    void applyToCurrent();
    void applyToAll();
    void currentChannelChanged(QTreeWidgetItem *cur, QTreeWidgetItem *previous);
    void addProperty(QFormLayout *l, ChannelProperty &p);
    void removeProperty();
    QVector<Channel *> channels;

    QLabel *file;
    QTreeWidget *channelsTable;
    QFormLayout *descriptionsLayout;
    QFormLayout *functionLayout;

    QPushButton *prevButton;
    QPushButton *nextButton;
    QPushButton *applyToCurrButton;
    QPushButton *applyToAllButton;

    QVector<ChannelProperty> channelProperties;
    QVector<ChannelProperty> functionProperties;
    QVector<ChannelProperty> descriptionProperties;
    QVector<DataProperty> dataProperties;


//    QtTreePropertyBrowser *propertyTree;
//    QtVariantPropertyManager *m_manager;
//    QtVariantEditorFactory *m_factory;


    int current=0;
    //int currentEditedIndex = -1;
//    QString currentValue;
    CurrentEdited currentEdited;
};

#endif // CHANNELPROPERTIESDIALOG_H
