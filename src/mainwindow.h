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
class PlotArea;

class QwtLegend;
class QwtPlotGrid;
class QwtPlotItem;
class QwtPlotCurve;

class Curve;
class Channel;
namespace ads {
    class CDockManager;
    class CDockAreaWidget;
    class CDockWidget;
}

#include "app.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:


    MainWindow(QWidget *parent = 0);
    ~MainWindow();

    //вызывается в Tab
    bool addFiles(const QStringList &files, bool silent=false);
    void updateChannelsTable(FileDescriptor *descriptor);
    void onChannelsDropped(bool plotOnLeft, const QVector<Channel*> &channels);
public slots:
    void createNewTab();
    void addPlotArea();
    void addPlotTabbed();
    void updateActions();
    /**
     * @brief plotChannel строит канал текущей записи
     * @param index номер канала
     */
    void plotChannel(int index);

    /**
     * @brief deleteCurve удаляет график
     * @param index номер канала
     */
    void deleteCurve(int index);
private slots:

    void addFolder();
    void addFolderWithSubfolders();

    bool addFolder(const QString &directory, bool withAllSubfolders, bool silent = false);
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

    /**
     * @brief plotAllChannels
     * Строит все каналы выделенной записи
     */
    void plotAllChannels();
    void plotAllChannelsAtRight();



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
     * @brief rescanBase
     * обновляет список файлов в таблице, удаляет мертвые записи
     */
    void rescanBase();

    void closeTab(ads::CDockWidget *t);
    void closeOtherTabs(int);

    void closePlot(ads::CDockWidget *t);
    void closeOtherPlots(int index);

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

    void renameDescriptor();

//Удалить
//    void showFileHandler(int index);
private:
    void createActions();
    QString getFolderToAdd(bool withSubfolders);
    void moveChannels(bool up);

    void addDescriptors(const QList<F> &files, bool silent=false);

    bool deleteChannels(FileDescriptor *file, const QVector<int> &channelsToDelete);
    bool deleteChannels(const QVector<Channel *> channels);
    bool copyChannels(FileDescriptor *descriptor, const QVector<int> &channelsToCopy);
    bool copyChannels(const QVector<Channel *> source);

    void exportToExcel(bool fullRange, bool dataOnly=false);
    void updateRecordsTable(const QList<FileDescriptor *> &records);

    void createTab(const QString &name, const QStringList &folders);

//    FileDescriptor *findDescriptor(const QString &file);

    void addFile(F descriptor);
    void setCurrentAndPlot(FileDescriptor *d, int channelIndex);

    void previousOrNextDescriptor(bool up);
    void cycleChannelsUpOrDown(bool up);

    ads::CDockManager* m_DockManager;
    ads::CDockAreaWidget *topArea = nullptr;
    ads::CDockAreaWidget *bottomArea = nullptr;


    bool sergeiMode = false;
    QVector<int> plottedChannelsNumbers;
    QVector<int> cycled;

    QStringList tabsNames;

    Tab *currentTab = nullptr;

    QMenu *plotsMenu;


    QAction *addFolderAct;
    QAction *addFolderWithSubfoldersAct;
    QAction *addFileAct;
    QAction *saveAct;
    QAction *renameAct;
    QAction *plotHelpAct;

    QAction *delFilesAct;
    QAction *plotAllChannelsAct;
    QAction *plotAllChannelsOnRightAct;
    QAction *plotSelectedChannelsAct;
    QAction *plotSelectedChannelsOnRightAct;
    QAction *exportChannelsToWavAct;
    QAction *calculateSpectreAct;
    QAction *calculateSpectreDeepSeaAct;
    QAction *calculateThirdOctaveAct;

    QAction *rescanBaseAct;
    QAction *editColorsAct;

    QAction *meanAct;
    QAction *movingAvgAct;
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

    QAction *convertMatFilesAct;
    QAction *convertTDMSFilesAct;
    QAction *convertEsoFilesAct;
    QAction *convertAct;
    QAction *copyToLegendAct;
    QAction *editYNameAct;
    QAction *aboutAct;
    QAction *addPlotAreaAct;

    PlotArea *currentPlot = nullptr;

protected:
    void closeEvent(QCloseEvent *event) override;
signals:
    void loading(const QString &file);
    void updateLegends();
private:
    bool closeRequested();
    PlotArea *createPlotArea();
};

#endif // MAINWINDOW_H
