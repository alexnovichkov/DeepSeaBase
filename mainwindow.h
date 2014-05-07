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
class QTableWidget;
class QTreeWidgetItem;
class QScrollBar;
class Channel;
class QTableWidgetItem;

class TabWidget;
class CheckableHeaderView;

class Plot;

class QwtLegend;
class QwtPlotGrid;
class QwtPlotItem;
class QwtPlotCurve;

class Curve;

#include <QSplitter>

class Tab : public QSplitter
{
    Q_OBJECT
public:
    Tab(QWidget * parent) : QSplitter(parent) {}

    CheckableHeaderView *tableHeader;
    QLabel *filePathLabel;
    QTreeWidget *tree;
    QTableWidget *channelsTable;
    QStringList folders;
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
    void addFolder(const QString &directory);
    void deleteFiles();

    void updateChannelsTable(QTreeWidgetItem *, QTreeWidgetItem *);

    void maybePlotChannel(int,int,int,int);
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

    void addFile(const QString &fileName, bool plot);
    void updateFile(const QString &fileName, bool plot);

    void exportToExcel();

    void onCurveColorChanged(Curve *curve);
private:
    void addFiles(const QStringList &files);

    void deleteFiles(const QVector<int> &indexes);

//    void updateRecordState(int recordIndex);
    void updateChannelsHeaderState();

    void updateChannelsTable(DfdFileDescriptor *);

    void createTab(const QString &name, const QStringList &folders);

    QString getNewSheetName();

    QStringList tabsNames;

    Tab *tab;

    DfdFileDescriptor *record;

    QAction *addFolderAct;
    QAction *delFilesAct;
    QAction *plotAllChannelsAct;
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

    QAction *exportToExcelAct;

    Plot *plot;

    QSplitter *splitter;



    TabWidget *tabWidget;
};

#endif // MAINWINDOW_H
