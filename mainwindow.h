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
    void addFolderWithSubfolders();

    void addFolder(const QString &directory, bool withAllSubfolders, bool silent = false);
    void deleteFiles();
    void addFile();

    void deleteChannels();
    void deleteChannelsBatch();
    void copyChannels();
    void moveChannels();

    void addCorrection();
    void addCorrections();

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
    void treeItemChanged(QTreeWidgetItem *item, int column);

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
    void onCurveDeleted(FileDescriptor *descriptor, int index);

    void calculateMean();
    void calculateThirdOctave();

    void moveChannelsUp();
    void moveChannelsDown();
    void editDescriptions();

    void save();

    void convertMatFiles();
private:
    void moveChannels(bool up);
    void updateFile(FileDescriptor *descriptor);
    void addFiles(QStringList &files);
    void addFiles(const QList<FileDescriptor*> &files);

    void deleteFiles(const QVector<int> &indexes);

    bool deleteChannels(const QList<QPair<FileDescriptor *, int> > &channelsToDelete);
    bool copyChannels(const QList<QPair<FileDescriptor*, int> > &channelsToCopy);

    void exportToExcel(bool fullRange);

//    void updateRecordState(int recordIndex);
    void updateChannelsHeaderState();

    void updateChannelsTable(FileDescriptor *);
    void updateRecordsTable(const QList<FileDescriptor*> &records);

    void createTab(const QString &name, const QStringList &folders);

    QList<QPair<FileDescriptor*, int> > selectedChannels();
    FileDescriptor *findDescriptor(const QString &file);
    bool findDescriptor(FileDescriptor *d);

    void addFile(FileDescriptor *descriptor);
    void setCurrentAndPlot(FileDescriptor *descriptor, int index);


    QStringList tabsNames;

    Tab *tab;



    QAction *addFolderAct;
    QAction *addFolderWithSubfoldersAct;
    QAction *addFileAct;
    QAction *saveAct;


    QAction *delFilesAct;
    QAction *plotAllChannelsAct;
    QAction *removeChannelsPlotsAct;
    QAction *convertAct;
    QAction *calculateThirdOctaveAct;
    QAction *clearPlotAct;
    QAction *savePlotAct;
    QAction *rescanBaseAct;
    QAction *switchCursorAct;
    QAction *trackingCursorAct;
    QAction *copyToClipboardAct;
    QAction *printPlotAct;
    QAction *editColorsAct;

    QAction *meanAct;
    QAction *interactionModeAct;
    QAction *addCorrectionAct;
    QAction *addCorrectionsAct;
    QAction *deleteChannelsAct;
    QAction *deleteChannelsBatchAct;
    QAction *copyChannelsAct;
    QAction *moveChannelsAct;

    QAction *moveChannelsUpAct;
    QAction *moveChannelsDownAct;

    QAction *editDescriptionsAct;

    QToolBar *mainToolBar;

    QAction *exportToExcelAct;
    QAction *switchHarmonicsAct;

    QAction *convertMatFilesAct;

    Plot *plot;

    QSplitter *splitter;



    TabWidget *tabWidget;

};

#endif // MAINWINDOW_H
