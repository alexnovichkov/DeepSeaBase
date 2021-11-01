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
class QTableWidgetItem;
class QToolBar;
class QSplitter;
class Plot;

class QwtLegend;
class QwtPlotGrid;
class QwtPlotItem;
class QwtPlotCurve;

class Curve;
class Channel;

#include "app.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:


    MainWindow(QWidget *parent = 0);
    ~MainWindow();

//    static QVariant getSetting(const QString &key, const QVariant &defValue=QVariant());
//    static void setSetting(const QString &key, const QVariant &value);
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

    void deletePlottedChannels();
    void copyPlottedChannels();
    void movePlottedChannels();

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
     * @brief calculateSpectreRecords
     * Обработка записей - расчет спектров, взаимных характеристик и т.д.
     */
    void calculateSpectreRecords(bool useDeepsea = false);
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
    void onCurveDeleted(Channel *channel);

    void calculateMean();
    void calculateThirdOctave();
    void calculateMovingAvg();

    void moveChannelsUp();
    void moveChannelsDown();
    void editDescriptions();
    void editChannelDescriptions();

    void save();

    void convertMatFiles();
    void convertTDMSFiles();
    void convertEsoFiles();

    void saveTimeSegment(const QList<FileDescriptor*> &files, double from, double to);

    void editYName();
    void updatePlottedChannelsNumbers();

    void previousDescriptor();
    void nextDescriptor();
    void arbitraryDescriptor();

    void cycleChannelsUp();
    void cycleChannelsDown();

    void exportChannelsToWav();
    void updateActions();
    void renameDescriptor();
private:
    QString getFolderToAdd(bool withSubfolders);
    void moveChannels(bool up);
    void addFiles(const QStringList &files, bool silent=false);
    void addDescriptors(const QList<F> &files, bool silent=false);

    bool deleteChannels(FileDescriptor *file, const QVector<int> &channelsToDelete);
    bool deleteChannels(const QVector<Channel *> channels);
    bool copyChannels(FileDescriptor *descriptor, const QVector<int> &channelsToCopy);
    bool copyChannels(const QVector<Channel *> source);

    void exportToExcel(bool fullRange, bool dataOnly=false);

    void updateChannelsTable(FileDescriptor *descriptor);
    void updateRecordsTable(const QList<FileDescriptor *> &records);

    void createTab(const QString &name, const QStringList &folders);

//    FileDescriptor *findDescriptor(const QString &file);

    void addFile(F descriptor);
    void setCurrentAndPlot(FileDescriptor *d, int channelIndex);

    void previousOrNextDescriptor(bool up);
    void cycleChannelsUpOrDown(bool up);


    bool sergeiMode = false;
    QVector<int> plottedChannelsNumbers;
    QVector<int> cycled;

    QStringList tabsNames;

    Tab *tab;



    QAction *addFolderAct;
    QAction *addFolderWithSubfoldersAct;
    QAction *addFileAct;
    QAction *saveAct;
    QAction *renameAct;


    QAction *delFilesAct;
    QAction *plotAllChannelsAct;
    QAction *plotAllChannelsOnRightAct;
    QAction *plotSelectedChannelsAct;
    QAction *plotSelectedChannelsOnRightAct;
    QAction *exportChannelsToWavAct;
    QAction *calculateSpectreAct;
    QAction *calculateSpectreDeepSeaAct;
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

    QAction *deletePlottedChannelsAct;
    //QAction *deleteChannelsBatchAct;
    QAction *copyPlottedChannelsAct;
    QAction *movePlottedChannelsAct;

    QAction *moveChannelsUpAct;
    QAction *moveChannelsDownAct;
    QAction *editDescriptionsAct;
    QAction *editChannelDescriptionsAct;

    QToolBar *mainToolBar;

    QAction *exportToExcelAct;
//    QAction *switchHarmonicsAct;

    QAction *convertMatFilesAct;
    QAction *convertTDMSFilesAct;
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
    QAction *cycleChannelsUpAct;
    QAction *cycleChannelsDownAct;
    QAction *aboutAct;

    Plot *plot;

    QSplitter *splitter;



    TabWidget *tabWidget;

//    QThread *workingThread;


    // QWidget interface

protected:
    void closeEvent(QCloseEvent *event) override;
signals:
    void loading(const QString &file);
private:
    bool closeRequested();
};

#endif // MAINWINDOW_H
