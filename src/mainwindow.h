#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QList>
#include <QHash>
#include <QVariant>
#include <QPair>

class FileDescriptor;

class QAction;
class Tab;
class QMenu;
class QToolBar;
class PlotArea;
class CorrectionDialog;
class Channel;
class QCPPlot;

namespace ads {
    class CDockManager;
    class CDockAreaWidget;
    class CDockWidget;
}
class IConvertPlugin;

#include "app.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

    bool addFiles(const QStringList &files, bool silent=false);

    void onChannelsDropped(bool plotOnLeft, const QVector<Channel*> &channels);
    void onChannelsDropped(bool plotOnLeft, const QVector<Channel*> &channels, bool plotAll);
public slots:
    void createNewTab();
    void addPlotArea();
    void addPlotTabbed();
    void updateActions();

private slots:
    void onFocusedDockWidgetChanged(ads::CDockWidget* old, ads::CDockWidget* now);
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
    void exportToClipboard();

    void onChannelChanged(Channel *ch);

    void calculateMean();
    void calculateThirdOctave();
    void calculateMovingAvg();
    void saveHorizontalSlice(const QVector<double> &zValues);
    void saveVerticalSlice(const QVector<double>& frequencies);
    void saveTimeSegment(const QVector<FileDescriptor*> &files, double from, double to);

    void moveChannelsUp();
    void moveChannelsDown();
    void editDescriptions();
    void editChannelDescriptions();

    void save();

    void convertMatFiles();
    void convertEsoFiles();
    void convertAnaFiles();

    void exportChannelsToWav();
    void renameDescriptor();

    void onPluginTriggered(const QString &pluginKey);
    void setCurrentPlot(QCPPlot *plot);
private:
    void saveTabsState();
    void createActions();
    void createConvertPluginsMenu(QMenu *menu);
    QString getFolderToAdd(bool withSubfolders);
    void moveChannels(bool up);

    void ctrlUpTriggered(bool b);
    void ctrlDownTriggered(bool b);

    void addDescriptors(const QList<F> &files, bool silent=false);

    bool deleteChannels(FileDescriptor *file, const QVector<int> &channelsToDelete);

    /**
     * @brief deletePlotted удаляет каналы, построенные на текущем графике
     * @return true если удаление успешно
     */
    bool deletePlotted();
    bool copyChannels(FileDescriptor *descriptor, const QVector<int> &channelsToCopy);
    bool copyChannels(const QVector<Channel *> source);

    void updateRecordsTable(const QList<FileDescriptor *> &records);

    void createTab(const QString &name, const QStringList &folders);

    void addFile(F descriptor);
    void setCurrentAndPlot(FileDescriptor *d, int channelIndex);

    void setDescriptor(int direction, bool checked);

    void dockWidgetAboutToBeRemoved(ads::CDockWidget* dockWidget);
    void unplotChannel(Channel *channel, bool allChannels);

    ads::CDockAreaWidget *bottomArea = nullptr;
    ads::CDockAreaWidget *topArea = nullptr;
    ads::CDockManager* m_DockManager;

    QStringList tabsNames;

    Tab* currentTab = nullptr;
    PlotArea *currentPlot = nullptr;

    QMenu *plotsMenu;
    QMenu *tabsMenu;

    QAction *settingsAct;
    QAction *addTabAct;
    QAction *addFolderAct;
    QAction *addFolderWithSubfoldersAct;
    QAction *addFileAct;
    QAction *saveAct;
    QAction *renameAct;
    QAction *plotHelpAct;

    QAction *delFilesAct;
    QAction *exportChannelsToWavAct;
    QAction *calculateSpectreAct;
    QAction *calculateSpectreAct1;
    QAction *calculateSpectreDeepSeaAct;
    QAction *calculateThirdOctaveAct;

    QAction *rescanBaseAct;
    QAction *editColorsAct;

    QAction *meanAct;
    QAction *movingAvgAct;
    QAction *addCorrectionAct;
    QAction *addCorrectionsAct;
    QAction *deleteChannelsAct;
    QAction *deleteChannelsOneAct;
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

    QAction *ctrlUpAct;
    QAction *ctrlDownAct;
    bool ctrlUpTargetsChannels = true;

    QToolBar *mainToolBar;

    QAction *exportToExcelAct;
    QAction *exportToExcelFullAct;
    QAction *exportToExcelOnlyDataAct;
    QAction *exportToClipboardAct;

    QAction *convertMatFilesAct;
    QAction *convertEsoFilesAct;
    QAction *convertAnaFilesAct;
    QAction *convertAct;
    QAction *aboutAct;
    QAction *addPlotAreaAct;

    QHash<QString, IConvertPlugin *> loadedPlugins;
protected:
    void closeEvent(QCloseEvent *event) override;
signals:
    void loading(const QString &file);
    void updateLegends();
    void updatePlotAreas();
    void descriptorChanged(int index, FileDescriptor *d);
private:
    bool closeRequested();
    PlotArea *createPlotArea();

    // QObject interface
public:
//    virtual bool event(QEvent *event) override;
};

#endif // MAINWINDOW_H
