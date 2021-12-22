#ifndef TAB_H
#define TAB_H

#include <QSplitter>
#include <QItemSelection>

class HeaderView;
class QLabel;
class FilesTable;
class QTableView;
class Model;
class SortFilterModel;
class FilteredHeaderView;
class ChannelTableModel;
class QLineEdit;
class FileDescriptor;
class QFileSystemWatcher;
class FileHandler;
class MainWindow;
class ChannelsTable;

class Tab : public QSplitter
{
    Q_OBJECT
    MainWindow *parent;
public:
    Tab(MainWindow *parent);
    ~Tab() {}

    void updateChannelsTable(FileDescriptor *descriptor);

    QLabel *filePathLabel = nullptr;
    FilesTable *filesTable = nullptr;
    ChannelsTable *channelsTable = nullptr;
    Model *model = nullptr;
    SortFilterModel *sortModel = nullptr;
//    FilterHeaderView *filterHeader;
    FilteredHeaderView *filterHeader = nullptr;
    ChannelTableModel *channelModel = nullptr;
    QList<QLineEdit *> filters;

    FileDescriptor *record = nullptr;
    FileHandler *fileHandler = nullptr;
signals:
    void descriptorChanged();
private:
    QAction *openFolderAct;
    QAction *editFileAct;
private slots:
    void filesSelectionChanged(const QItemSelection &newSelection, const QItemSelection &oldSelection);
    void channelsSelectionChanged(const QItemSelection &newSelection, const QItemSelection &oldSelection);
    void updateChannelsTable(const QModelIndex &current, const QModelIndex &previous);
};

#endif // TAB_H
