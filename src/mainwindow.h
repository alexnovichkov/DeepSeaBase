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
class QItemSelection;
class QTableView;

class TabWidget;
class HeaderView;

class Plot;

class QwtLegend;
class QwtPlotGrid;
class QwtPlotItem;
class QwtPlotCurve;

class Curve;
class Channel;
class Model;
class ChannelTableModel;
class SortFilterModel;
class QLineEdit;
class FilterHeaderView;

#include <QSplitter>


class Tab : public QSplitter
{
    Q_OBJECT
public:
    Tab(QWidget * parent) : QSplitter(parent), record(0) {}
    ~Tab();

    HeaderView *tableHeader;
    QLabel *filePathLabel;
    QTreeView *filesTable;
    QTableView *channelsTable;
    Model *model;
    SortFilterModel *sortModel;
    FilterHeaderView *filterHeader;
    ChannelTableModel *channelModel;
    QList<QLineEdit *> filters;

    QStringList folders;

    FileDescriptor *record;
private slots:
    void filesSelectionChanged(const QItemSelection &newSelection, const QItemSelection &oldSelection);
    void channelsSelectionChanged(const QItemSelection &newSelection, const QItemSelection &oldSelection);
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

    void updateChannelsTable(const QModelIndex &current, const QModelIndex &previous);

    /**
     * @brief plotAllChannels
     * Строит все каналы выделенной записи
     */
    void plotAllChannels();
    void plotAllChannelsAtRight();

    /**
     * @brief plotChannel строит канал текущей записи
     * @param index номер канала
     */
    void plotChannel(int index);

    /**
     * @brief plotSelectedChannels строит выделенные каналы
     */
    void plotSelectedChannels();

    /**
     * @brief calculateSpectreRecords
     * Обработка записей - расчет спектров, взаимных характеристик и т.д.
     */
    void calculateSpectreRecords();
    /**
     * @brief convertFiles
     * Конвертирует выделенные записи в другой формат и добавляет их в базу
     */
    void convertFiles();

    /**
     * @brief copyToLegend
     * Копирует названия выделенных файлов в столбец легенды
     */
    void copyToLegend();

    /**
     * @brief deleteCurve удаляет график
     * @param index номер канала
     */
    void deleteCurve(int index);

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

    void editColors();



    void exportToExcel();
    void exportToExcelFull();
    void exportToExcelData();

    void onCurveColorChanged(Curve *curve);
    void onCurveDeleted(FileDescriptor *descriptor, int index);

    void calculateMean();
    void calculateThirdOctave();
    void calculateMovingAvg();

    void moveChannelsUp();
    void moveChannelsDown();
    void editDescriptions();

    void save();

    void convertMatFiles();
    void convertEsoFiles();

    void saveTimeSegment(const QList<FileDescriptor*> &files, double from, double to);

    void editYName();
    void onChannelDescriptionChanged(int index, const QString& value);
    void onChannelNameChanged(int index, const QString& value);
    void updatePlottedChannelsNumbers();

    void previousDescriptor();
    void nextDescriptor();
    void arbitraryDescriptor();
private:
    void moveChannels(bool up);
    void addFiles(QStringList &files);
    void addDescriptors(const QList<FileDescriptor*> &files);

    bool deleteChannels(FileDescriptor *file, const QVector<int> &channelsToDelete);
    bool copyChannels(FileDescriptor *descriptor, const QVector<int> &channelsToCopy);

    void exportToExcel(bool fullRange, bool dataOnly=false);

    void updateChannelsTable(FileDescriptor *descriptor);
    void updateRecordsTable(const QList<FileDescriptor *> &records);

    void createTab(const QString &name, const QStringList &folders);

    FileDescriptor *findDescriptor(const QString &file);
    bool duplicated(FileDescriptor *file) const;

    void addFile(FileDescriptor *descriptor);
    void setCurrentAndPlot(FileDescriptor *d, int channelIndex);


    bool sergeiMode = false;
    QVector<int> plottedChannelsNumbers;

    QStringList tabsNames;

    Tab *tab;



    QAction *addFolderAct;
    QAction *addFolderWithSubfoldersAct;
    QAction *addFileAct;
    QAction *saveAct;


    QAction *delFilesAct;
    QAction *plotAllChannelsAct;
    QAction *plotAllChannelsAtRightAct;
    QAction *plotSelectedChannelsAct;
    QAction *removeChannelsPlotsAct;
    QAction *calculateSpectreAct;
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
    QAction *movingAvgAct;
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
//    QAction *switchHarmonicsAct;

    QAction *convertMatFilesAct;
    QAction *convertEsoFilesAct;
    QAction *convertAct;
    QAction *copyToLegendAct;

    QAction *autoscaleXAct;
    QAction *autoscaleYAct;
    QAction *autoscaleYSlaveAct;
    QAction *autoscaleAllAct;

    QAction *removeLabelsAct;
    QAction *playAct;
    QAction *editYNameAct;

    QAction *previousDescriptorAct;
    QAction *nextDescriptorAct;
    QAction *arbitraryDescriptorAct;

    Plot *plot;

    QSplitter *splitter;



    TabWidget *tabWidget;

//    QThread *workingThread;


    // QWidget interface
protected:
    virtual void closeEvent(QCloseEvent *event) override;
private:
    bool closeRequested();
signals:
    void allClosed();
};

#endif // MAINWINDOW_H
