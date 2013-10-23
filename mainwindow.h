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
class QScrollBar;
class Channel;
class QTableWidgetItem;

class CheckableHeaderView;

class Plot;

class QwtLegend;
class QwtPlotGrid;
class QwtPlotItem;
class QwtPlotCurve;

class Curve;

typedef QStringList FileRecord;

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
    void addExistingFiles();
    void deleteFiles();

    void updateChannelsTable(QTreeWidgetItem *, QTreeWidgetItem *);
//    void updateCheckState(QTreeWidgetItem*item, int col);

    void maybePlotChannel(int,int,int,int);
    void maybePlotChannel(QTableWidgetItem *item);

//    void xRangeChanged(const QCPRange &range);
//    void yRangeChanged(const QCPRange &range);

//    void graphClicked(QCPAbstractPlottable*);
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

private:
    void addFiles(QList<QStringList> &files, bool addToDatabase);
    void addFiles(const QStringList &files, bool addToDatabase);

    void deleteFiles(const QVector<int> &indexes);

    void updateRecordState(int recordIndex);
    void updateChannelsHeaderState();

    void rescanDeadRecords();
    void removeDeadRecords();

    QTreeWidget *tree;
    QTableWidget *channelsTable;

    QList<FileRecord> alreadyAddedFiles;
    DfdFileDescriptor *record;


    QAction *addFolderAct;
    QAction *delFilesAct;
    QAction *plotAllChannelsAct;
    QAction *convertAct;
    QAction *clearPlotAct;
    QAction *savePlotAct;
    QAction *rescanBaseAct;
    QAction *switchCursorAct;

    Plot *plot;

    QSplitter *splitter;
    QSplitter *upperSplitter;
    CheckableHeaderView *tableHeader;
    QLabel *filePathLabel;


};

#endif // MAINWINDOW_H
