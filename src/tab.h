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

class Tab : public QSplitter
{
    Q_OBJECT
public:
    Tab(QWidget * parent);
    ~Tab() {}

    HeaderView *tableHeader = nullptr;
    QLabel *filePathLabel = nullptr;
    FilesTable *filesTable = nullptr;
    QTableView *channelsTable = nullptr;
    Model *model = nullptr;
    SortFilterModel *sortModel = nullptr;
//    FilterHeaderView *filterHeader;
    FilteredHeaderView *filterHeader = nullptr;
    ChannelTableModel *channelModel = nullptr;
    QList<QLineEdit *> filters;

    FileDescriptor *record = nullptr;
    FileHandler *fileHandler = nullptr;
private slots:
    void filesSelectionChanged(const QItemSelection &newSelection, const QItemSelection &oldSelection);
    void channelsSelectionChanged(const QItemSelection &newSelection, const QItemSelection &oldSelection);
};

#endif // TAB_H
