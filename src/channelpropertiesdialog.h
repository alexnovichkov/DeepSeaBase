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
    void cellChanged(int row, int column);
    void addProperty();
    void removeProperty();
    QVector<Channel *> channels;
    QLabel *file;
    QTreeWidget *channelsTable;
    QTableWidget *descriptionsTable;

    int current=0;
};

#endif // CHANNELPROPERTIESDIALOG_H
