#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QList>
#include <QHash>
#include <QVariant>
#include <QPair>

#include "dfdfiledescriptor.h"

class QAction;
class QLabel;
class TabWidget;
class Tab;
class QMenu;
class QTreeWidget;

class QStatusBar;

class QFileSystemModel;
class QTreeView;
class QSplitter;
class QTableWidget;
class QTreeWidgetItem;
class QCustomPlot;
class QScrollBar;
class QCPGraph;
class Channel;
class QTableWidgetItem;
class QCPRange;
class QCPAbstractPlottable;

typedef QPair<QString, int> GraphIndex;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();
private slots:
    void addFolder();
    void addExistingFiles();
    void deleteFiles();

    void currentRecordChanged(QTreeWidgetItem *, QTreeWidgetItem *);
//    void updateCheckState(QTreeWidgetItem*item, int col);

    void plotChannel(Channel *channel, const GraphIndex &index, bool addToFixed);
    void maybePlotChannel(int,int,int,int);
    void maybePlotChannel(QTableWidgetItem *item);

    void xRangeChanged(const QCPRange &range);
    void yRangeChanged(const QCPRange &range);

    void graphClicked(QCPAbstractPlottable*);
    void plotSelectionChanged();

    /**
     * @brief plotAllChannels
     * Строит все каналы выделенной записи
     */
    void plotAllChannels();

    /**
     * @brief convertRecords
     * Конвертирует выделенные записи в другой формат и добавляет их в базу
     */
    void convertRecords();
private:
    QVariant getSetting(const QString &key, const QVariant &defValue=QVariant());
    void setSetting(const QString &key, const QVariant &value);

    void addFiles(const QStringList &files, bool addToDatabase);
    QTreeWidget *tree;
    QTableWidget *channelsTable;

    //QList<DfdFileDescriptor *> records;
    QStringList alreadyAddedFiles;
    DfdFileDescriptor *record;
    QHash<GraphIndex, QCPGraph *> graphs;
    QHash<DfdDataType, QCustomPlot *> plots;
    QCPGraph *freeGraph;

    QAction *addFolderAct;
    QAction *delFilesAct;
    QAction *plotAllChannelsAct;
    QAction *convertAct;

    QCustomPlot *plot;
    QScrollBar *hScrollBar;
    QScrollBar *vScrollBar;

    QSplitter *splitter;

    QLabel *filePathLabel;
};

#endif // MAINWINDOW_H
