#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QList>
#include <QHash>
#include <QVariant>
#include <QPair>

class FileDescriptor;

class QAction;
class QLabel;
class TabWidget;
class Tab;
class QMenu;
class QTreeWidget;
class QStatusBar;
class QFileSystemModel;
class QTreeView;
class QTableWidget;
class QTreeWidgetItem;
class QScrollBar;
class DfdChannel;
class QTableWidgetItem;
class QToolBar;

class TabWidget;
class CheckableHeaderView;

class Plot;

class QwtLegend;
class QwtPlotGrid;
class QwtPlotItem;
class QwtPlotCurve;

class Curve;
class Channel;

#include <QSplitter>

class Tab : public QSplitter
{
    Q_OBJECT
public:
    Tab(QWidget * parent) : QSplitter(parent), record(0) {}

    CheckableHeaderView *tableHeader;
    QLabel *filePathLabel;
    QTreeWidget *tree;
    QTableWidget *channelsTable;
    QStringList folders;

    FileDescriptor *record;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:


    MainWindow(QWidget *parent = 0);
    ~MainWindow();

    static QVariant getSetting(const QString &key, const QVariant &defValue=QVariant());
    static void setSetting(const QString &key, const QVariant &value);
private slots:
    void addFolder();
    void addFolder(const QString &directory, bool silent = false);
    void deleteFiles();
    void addFile();

    void deleteChannels();
    void copyChannels();
    void moveChannels();

    void updateChannelsTable(QTreeWidgetItem *, QTreeWidgetItem *);

//    void maybePlotChannel(int,int,int,int);
    void maybePlotChannel(QTableWidgetItem *item);

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
    /**
     * @brief headerToggled
     * Вызывается, когда отмечена галочка в заголовке списка каналов
     */
    void headerToggled(int column,Qt::CheckState state);

    /**
     * @brief clearPlot
     * Удаляет все графики с графика
     */
    void clearPlot();

    /**
     * @brief recordLegendChanged
     * Обновляет легенду графика
     * @param item
     * @param column
     */
    void recordLegendChanged(QTreeWidgetItem *item, int column);

    /**
     * @brief rescanBase
     * обновляет список файлов в таблице, удаляет мертвые записи
     */
    void rescanBase();

    void createNewTab();

    void closeTab(int);
    void closeOtherTabs(int);
    void renameTab(int i);
    void changeCurrentTab(int currentIndex);
    void onTabTextChanged();

    void editColors();



    void exportToExcel();
    void exportToExcelFull();

    void onCurveColorChanged(Curve *curve);
    void onCurveDeleted(Curve *curve);

    void calculateMean();

    void moveChannelsUp();
    void moveChannelsDown();
    void editDescriptions();
private:
    void moveChannels(bool up);
    void updateFile(FileDescriptor *descriptor);
    void addFiles(const QStringList &files);
    void addFiles(const QList<FileDescriptor*> &files);

    void deleteFiles(const QVector<int> &indexes);

    bool deleteChannels(const QMultiHash<FileDescriptor *, int> &channelsToDelete);
    bool copyChannels(const QMultiHash<FileDescriptor*, int> &channelsToCopy);

    void exportToExcel(bool fullRange);

//    void updateRecordState(int recordIndex);
    void updateChannelsHeaderState();

    void updateChannelsTable(FileDescriptor *);
    void updateRecordsTable(const QList<FileDescriptor*> &records);

    void createTab(const QString &name, const QStringList &folders);

    QMultiHash<FileDescriptor *, int> selectedChannels();
    FileDescriptor *findDescriptor(const QString &file);

    void addFile(FileDescriptor *descriptor);
    void setCurrentAndPlot(FileDescriptor *descriptor, int index);


    QStringList tabsNames;

    Tab *tab;



    QAction *addFolderAct;
    QAction *addFileAct;


    QAction *delFilesAct;
    QAction *plotAllChannelsAct;
    QAction *removeChannelsPlotsAct;
    QAction *convertAct;
    QAction *clearPlotAct;
    QAction *savePlotAct;
    QAction *rescanBaseAct;
    QAction *switchCursorAct;
    QAction *copyToClipboardAct;
    QAction *printPlotAct;
    QAction *editColorsAct;

    QAction *meanAct;
    QAction *interactionModeAct;
    QAction *addCorrectionAct;
    QAction *deleteChannelsAct;
    QAction *copyChannelsAct;
    QAction *moveChannelsAct;

    QAction *moveChannelsUpAct;
    QAction *moveChannelsDownAct;

    QAction *editDescriptionsAct;

    QToolBar *mainToolBar;

    QAction *exportToExcelAct;

    Plot *plot;

    QSplitter *splitter;



    TabWidget *tabWidget;

};

#endif // MAINWINDOW_H
