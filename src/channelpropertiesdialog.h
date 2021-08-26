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

struct CurrentEdited {
    int index = -1;
    QString value;
};

struct ChannelProperty
{
    QString displayName;
    QString name;
    QLineEdit* edit = nullptr;
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
//    void prev();
//    void next();
//    void applyToCurrent();
//    void applyToAll();
    void updateState();
    void currentChannelChanged(QTreeWidgetItem *cur, QTreeWidgetItem *previous);
    void selectedChannelsChanged();
    void cellChanged(int row, int column);
    void addProperty();
    void removeProperty();
    QList<Channel *> selectedChannels();
    QVector<Channel *> channels;

    QLabel *file;
    QTreeWidget *channelsTable;
    QTableWidget *descriptionsTable;
    QFormLayout *descriptionsLayout;

    QVector<ChannelProperty> channelProperties;
    QVector<DataProperty> dataProperties;

    int current=0;
    //int currentEditedIndex = -1;
//    QString currentValue;
    CurrentEdited currentEdited;
};

#endif // CHANNELPROPERTIESDIALOG_H
