#include "mainwindow.h"

#include <QtWidgets>

#include "convertdialog.h"
#include "sortabletreewidgetitem.h"
#include "checkableheaderview.h"
#include "plot.h"
#include "tabwidget.h"
#include "colorselector.h"
#include "coloreditdialog.h"
#include "correctiondialog.h"
#include "curve.h"
#include "pointlabel.h"

#include <ActiveQt/ActiveQt>
#include "logging.h"

#include "dfdfiledescriptor.h"
#include "ufffile.h"
#include "editdescriptionsdialog.h"
#include "matlabfiledescriptor.h"
#include "matlabconverterdialog.h"
#include    "esoconverterdialog.h"
#include    "uffconverterdialog.h"
#include "filedescriptor.h"

#define DSB_VERSION "1.6.7"

class DrivesDialog : public QDialog
{
//
public:
    DrivesDialog(QWidget * parent) : QDialog(parent)
    {

        QFileInfoList drives = QDir::drives();
        QVBoxLayout *l = new QVBoxLayout;
        l->addWidget(new QLabel("Выберите диски для сканирования", this));
        foreach(const QFileInfo &fi, drives) {
            QCheckBox *drive = new QCheckBox(fi.absoluteFilePath(), this);
            drive->setChecked(false);
            drivesList << drive;
            l->addWidget(drive);
        }
        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                                           QDialogButtonBox::Cancel);
        connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
        connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

        l->addWidget(buttonBox);
        setLayout(l);
    }
    QStringList drives;
private:
    QList<QCheckBox *> drivesList;

    // QDialog interface
public slots:
    virtual void accept()
    {
        foreach(QCheckBox *box, drivesList) {
            if (box->isChecked()) drives << box->text();
        }
        QDialog::accept();
    }
};

class DfdFilterProxy : public QSortFilterProxyModel
{
public:
    DfdFilterProxy(FileDescriptor *dfd, QObject *parent)
        : QSortFilterProxyModel(parent)
    {
        DfdFileDescriptor *d = dynamic_cast<DfdFileDescriptor *>(dfd);
        this->dataType = d->DataType;
        this->xStep = d->xStep();
    }
protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
    {
        QFileSystemModel *model = qobject_cast<QFileSystemModel *>(sourceModel());
        if (!model) return false;

        QModelIndex index0 = model->index(source_row, 0, source_parent);

        QFileInfo fi = model->fileInfo(index0);

        if (fi.isFile()) {
            if (fi.suffix().toLower()=="dfd") {
                DfdFileDescriptor dfd(fi.canonicalFilePath());
                dfd.read();
                if (dfd.DataType == dataType && dfd.xStep() == xStep) {
                    return true;
                }
                else {
                    return false;
                }
            }
            else {
                return false;
            }
        }
        else {
            return true;
        }
    }
private:
    DfdDataType dataType;
    double xStep;
};

class DfdUffFilterProxy : public QSortFilterProxyModel
{
public:
    DfdUffFilterProxy(QObject *parent)
        : QSortFilterProxyModel(parent)
    {
    }
protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
    {
        QFileSystemModel *model = qobject_cast<QFileSystemModel *>(sourceModel());
        if (!model) return false;

        QModelIndex index0 = model->index(source_row, 0, source_parent);

        QFileInfo fi = model->fileInfo(index0);

        if (fi.isFile()) {
            if (fi.suffix().toLower()=="dfd" || fi.suffix().toLower()=="uff" || fi.suffix().toLower()=="unv") {
                return true;
            }
            else {
                return false;
            }
        }
        else {
            return true;
        }
    }
};


FileDescriptor *createDescriptor(const QString &fileName)
{DD;
    const QString suffix = QFileInfo(fileName).suffix().toLower();
    if (suffix=="dfd") return new DfdFileDescriptor(fileName);
    if (suffix=="uff" || suffix=="unv") return new UffFileDescriptor(fileName);
    return 0;
}

void maybeAppend(const QString &s, QStringList &list)
{DD;
    if (!list.contains(s)) list.append(s);
}

void processDir(const QString &file, QStringList &files, bool includeSubfolders)
{DD;
    if (QFileInfo(file).isDir()) {
        QFileInfoList dirLst = QDir(file).entryInfoList(QStringList()<<"*.dfd"<<"*.DFD"<<"*.uff"<<"*.unv",
                                                        QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot,
                                                        QDir::DirsFirst);
        for (int i=0; i<dirLst.count(); ++i) {
            if (dirLst.at(i).isDir()) {
                if (includeSubfolders)
                    processDir(dirLst.at(i).absoluteFilePath(),files,includeSubfolders);
            }
            else
                maybeAppend(dirLst.at(i).absoluteFilePath(), files);
        }
    }
    else {
        maybeAppend(file, files);
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), tab(0)
{DD;
    setWindowTitle(tr("DeepSea Database ")+DSB_VERSION);
    setAcceptDrops(true);

    mainToolBar = new QToolBar(this);
    addToolBar(mainToolBar);
    mainToolBar->setIconSize(QSize(24,24));

    plot = new Plot(this);
//    connect(plot, SIGNAL(fileCreated(QString,bool)), SLOT(addFile(QString,bool)));
//    connect(plot, SIGNAL(fileChanged(QString, bool)), SLOT(updateFile(QString,bool)));
    connect(plot, SIGNAL(curveChanged(Curve*)), SLOT(onCurveColorChanged(Curve*)));
    connect(plot, SIGNAL(curveDeleted(FileDescriptor*,int)), SLOT(onCurveDeleted(FileDescriptor*,int)));

    addFolderAct = new QAction(qApp->style()->standardIcon(QStyle::SP_DialogOpenButton),
                               tr("Добавить папку"),this);
    addFolderAct->setShortcut(tr("Ctrl+O"));
    connect(addFolderAct, SIGNAL(triggered()), SLOT(addFolder()));

    addFolderWithSubfoldersAct = new QAction(tr("Добавить папку со всеми вложенными папками"),this);
    connect(addFolderWithSubfoldersAct, SIGNAL(triggered()), SLOT(addFolderWithSubfolders()));

    addFileAct  = new QAction(tr("Добавить файл"),this);
    connect(addFileAct, SIGNAL(triggered()), SLOT(addFile()));

    moveChannelsUpAct = new QAction("Сдвинуть каналы вверх", this);
    moveChannelsUpAct->setShortcutContext(Qt::WidgetShortcut);
    moveChannelsUpAct->setShortcut(tr("Ctrl+Up"));
    connect(moveChannelsUpAct, SIGNAL(triggered()), SLOT(moveChannelsUp()));

    moveChannelsDownAct = new QAction("Сдвинуть каналы вниз", this);
    moveChannelsDownAct->setShortcutContext(Qt::WidgetShortcut);
    moveChannelsDownAct->setShortcut(tr("Ctrl+Down"));
    connect(moveChannelsDownAct, SIGNAL(triggered()), SLOT(moveChannelsDown()));

    saveAct = new QAction("Сохранить файлы", this);
    saveAct->setIcon(qApp->style()->standardIcon(QStyle::SP_DialogSaveButton));
    saveAct->setShortcut(tr("Ctrl+S"));
    connect(saveAct, SIGNAL(triggered()), SLOT(save()));


    delFilesAct = new QAction(QString("Удалить записи"), this);
    delFilesAct->setShortcut(Qt::Key_Delete);
    delFilesAct->setShortcutContext(Qt::WidgetShortcut);
    connect(delFilesAct, SIGNAL(triggered()), SLOT(deleteFiles()));

    plotAllChannelsAct = new QAction(QString("Построить все каналы"), this);
    connect(plotAllChannelsAct, SIGNAL(triggered()), SLOT(plotAllChannels()));

    plotAllChannelsAtRightAct = new QAction(QString("...на правой оси"), this);
    connect(plotAllChannelsAtRightAct, SIGNAL(triggered()), SLOT(plotAllChannelsAtRight()));


    QAction *plotHelpAct = new QAction("Справка", this);
    connect(plotHelpAct, &QAction::triggered, [=](){

        QDesktopServices::openUrl(QUrl("help.html"));


    });

    calculateSpectreAct = new QAction(QString("Обработать записи..."), this);
    connect(calculateSpectreAct, SIGNAL(triggered()), SLOT(calculateSpectreRecords()));

    convertAct = new QAction("Конвертировать файлы...", this);
    connect(convertAct, SIGNAL(triggered()), SLOT(convertFiles()));

    clearPlotAct  = new QAction(QString("Очистить график"), this);
    clearPlotAct->setIcon(QIcon(":/icons/cross.png"));
    connect(clearPlotAct, SIGNAL(triggered()), SLOT(clearPlot()));

    savePlotAct = new QAction(QString("Сохранить график..."), this);
    savePlotAct->setIcon(qApp->style()->standardIcon(QStyle::SP_DialogSaveButton));
    connect(savePlotAct, SIGNAL(triggered()), plot, SLOT(savePlot()));

    rescanBaseAct = new QAction(QString("Пересканировать базу"), this);
    rescanBaseAct->setIcon(qApp->style()->standardIcon(QStyle::SP_BrowserReload));
    rescanBaseAct->setShortcut(QKeySequence::Refresh);
    connect(rescanBaseAct, SIGNAL(triggered()), this, SLOT(rescanBase()));

    switchCursorAct = new QAction(QString("Показать/скрыть курсор"), this);
    switchCursorAct->setIcon(QIcon(":/icons/cursor.png"));
    switchCursorAct->setCheckable(true);
    switchCursorAct->setObjectName("simpleCursor");
    bool pickerEnabled = MainWindow::getSetting("pickerEnabled", true).toBool();
    switchCursorAct->setChecked(pickerEnabled);
    connect(switchCursorAct, SIGNAL(triggered()), plot, SLOT(switchCursor()));

    trackingCursorAct = new QAction(QString("Включить курсор дискрет"), this);
    trackingCursorAct->setIcon(QIcon(":/icons/tracking.png"));
    trackingCursorAct->setCheckable(true);
    trackingCursorAct->setObjectName("trackingCursor");
    connect(trackingCursorAct, &QAction::triggered, [=](){
        plot->switchTrackingCursor();
    });
    connect(plot,SIGNAL(trackingPanelCloseRequested()),trackingCursorAct,SLOT(toggle()));


    copyToClipboardAct = new QAction(QString("Копировать в буфер обмена"), this);
    copyToClipboardAct->setIcon(QIcon(":/icons/clipboard.png"));
    connect(copyToClipboardAct, SIGNAL(triggered()), plot, SLOT(copyToClipboard()));

    printPlotAct = new QAction(QString("Распечатать рисунок"), this);
    printPlotAct->setIcon(QIcon(":/icons/print.png"));
    connect(printPlotAct, SIGNAL(triggered()), plot, SLOT(print()));

    exportToExcelAct = new QAction(QString("Экспортировать в Excel"), this);
    exportToExcelAct->setIcon(QIcon(":/icons/excel.png"));
    connect(exportToExcelAct, SIGNAL(triggered()), this, SLOT(exportToExcel()));
    QMenu *excelMenu = new QMenu(this);
    QAction *exportToExcelFullAct = new QAction("Экспортировать в Excel весь диапазон", this);
    connect(exportToExcelFullAct, SIGNAL(triggered()), this, SLOT(exportToExcelFull()));
    excelMenu->addAction(exportToExcelFullAct);
    QAction *exportToExcelOnlyDataAct = new QAction("Экспортировать в Excel только данные", this);
    connect(exportToExcelOnlyDataAct, SIGNAL(triggered()), this, SLOT(exportToExcelData()));
    excelMenu->addAction(exportToExcelOnlyDataAct);
    exportToExcelAct->setMenu(excelMenu);

    meanAct = new QAction(QString("Вывести среднее (энерг.)"), this);
    meanAct->setIcon(QIcon(":/icons/mean.png"));
    connect(meanAct, SIGNAL(triggered()), this, SLOT(calculateMean()));

    movingAvgAct = new QAction(QString("Рассчитать скользящее среднее"), this);
    connect(movingAvgAct, SIGNAL(triggered()), this, SLOT(calculateMovingAvg()));

    calculateThirdOctaveAct = new QAction(QString("Рассчитать третьоктаву"), this);
    calculateThirdOctaveAct->setIcon(QIcon(":/icons/third.png"));
    connect(calculateThirdOctaveAct, SIGNAL(triggered()), this, SLOT(calculateThirdOctave()));

    editColorsAct = new QAction(QString("Изменить цвета графиков"), this);
    editColorsAct->setIcon(QIcon(":/icons/colors.png"));
    connect(editColorsAct, SIGNAL(triggered()), this, SLOT(editColors()));

    interactionModeAct = new QAction(QString("Включить режим изменения данных"), this);
    interactionModeAct->setIcon(QIcon(":/icons/data.png"));
    interactionModeAct->setCheckable(true);
    connect(interactionModeAct, &QAction::triggered, [=](){
        plot->switchInteractionMode();
    });

//    switchHarmonicsAct = new QAction(QString("Включить показ гармоник"), this);
//    switchHarmonicsAct->setIcon(QIcon(":/icons/harmonics.png"));
//    switchHarmonicsAct->setCheckable(true);
//    connect(switchHarmonicsAct, &QAction::triggered, [=](){
//        plot->switchHarmonicsMode();
//    });

    addCorrectionAct = new QAction("Добавить поправку...", this);
    addCorrectionAct->setIcon(QIcon(":/icons/correction.png"));
    connect(addCorrectionAct, SIGNAL(triggered()), SLOT(addCorrection()));

    addCorrectionsAct = new QAction("Добавить поправки из файла...", this);
    connect(addCorrectionsAct, SIGNAL(triggered()), SLOT(addCorrections()));
    QMenu *addCorrectionMenu = new QMenu(this);
    addCorrectionMenu->addAction(addCorrectionsAct);
    addCorrectionAct->setMenu(addCorrectionMenu);

    deleteChannelsAct = new QAction("Удалить выделенные каналы...",this);
    deleteChannelsAct->setIcon(qApp->style()->standardIcon(QStyle::SP_TrashIcon));
    connect(deleteChannelsAct, SIGNAL(triggered()), this, SLOT(deleteChannels()));

    deleteChannelsBatchAct = new QAction("Удалить каналы в нескольких файлах...",this);
    connect(deleteChannelsBatchAct, SIGNAL(triggered()), this, SLOT(deleteChannelsBatch()));

    QMenu *deleteChannelsMenu = new QMenu(this);
    deleteChannelsMenu->addAction(deleteChannelsBatchAct);

    deleteChannelsAct->setMenu(deleteChannelsMenu);

    copyChannelsAct = new QAction("Копировать выделенные каналы в файл...", this);
    connect(copyChannelsAct, SIGNAL(triggered()), SLOT(copyChannels()));

    moveChannelsAct = new QAction("Переместить выделенные каналы в файл...", this);
    connect(moveChannelsAct, SIGNAL(triggered()), SLOT(moveChannels()));

    editDescriptionsAct = new QAction("Редактировать описание...", this);
    editDescriptionsAct->setIcon(QIcon(":/icons/descriptor.png"));
    connect(editDescriptionsAct, SIGNAL(triggered()), SLOT(editDescriptions()));

    QMenu *addFolderMenu = new QMenu(this);
    addFolderMenu->addAction(addFolderWithSubfoldersAct);
    addFolderAct->setMenu(addFolderMenu);

    convertMatFilesAct = new QAction("Конвертировать Matlab файлы...", this);
    connect(convertMatFilesAct,SIGNAL(triggered()),SLOT(convertMatFiles()));

    convertEsoFilesAct = new QAction("Конвертировать файлы ESO...", this);
    connect(convertEsoFilesAct,SIGNAL(triggered()),SLOT(convertEsoFiles()));

    mainToolBar->addWidget(new QLabel("Записи:"));
    mainToolBar->addAction(addFolderAct);
    mainToolBar->addAction(addFileAct);
    mainToolBar->addAction(calculateSpectreAct);
    mainToolBar->addAction(calculateThirdOctaveAct);
    mainToolBar->addAction(editDescriptionsAct);

    mainToolBar->addSeparator();
    mainToolBar->addWidget(new QLabel("  Каналы:"));
    QMenu *channelsMenu = new QMenu("Операции", this);
    channelsMenu->addAction(copyChannelsAct);
    channelsMenu->addAction(moveChannelsAct);
    channelsMenu->addAction(movingAvgAct);

    QToolButton *channelsTB = new QToolButton(this);
    channelsTB->setMenu(channelsMenu);
    channelsTB->setText("Операции...");
    channelsTB->setFixedHeight(32);
    channelsTB->setPopupMode(QToolButton::InstantPopup);



    mainToolBar->addWidget(channelsTB);
    mainToolBar->addAction(deleteChannelsAct);
    mainToolBar->addAction(meanAct);
    mainToolBar->addAction(addCorrectionAct);

    mainToolBar->addSeparator();
    mainToolBar->addWidget(new QLabel("  График:"));
    mainToolBar->addAction(clearPlotAct);
    mainToolBar->addAction(savePlotAct);
    mainToolBar->addAction(copyToClipboardAct);
    mainToolBar->addAction(printPlotAct);
    mainToolBar->addWidget(new QLabel("         ",this));

    mainToolBar->addAction(exportToExcelAct);
    mainToolBar->addAction(switchCursorAct);
//    mainToolBar->addSeparator();

    mainToolBar->addAction(interactionModeAct);
//    mainToolBar->addAction(switchHarmonicsAct);
    mainToolBar->addAction(trackingCursorAct);
    mainToolBar->addAction(plotHelpAct);


    QMenu *fileMenu = menuBar()->addMenu(tr("Файл"));
    fileMenu->addAction(addFolderAct);
    fileMenu->addAction(addFileAct);
    fileMenu->addAction(convertAct);
    fileMenu->addAction(convertMatFilesAct);
    fileMenu->addAction(convertEsoFilesAct);

    QMenu *recordsMenu = menuBar()->addMenu(QString("Записи"));
    recordsMenu->addAction(delFilesAct);
    recordsMenu->addAction(rescanBaseAct);
    recordsMenu->addAction(saveAct);

    QMenu *settingsMenu = menuBar()->addMenu(QString("Настройки"));
    settingsMenu->addAction(editColorsAct);

    tabWidget = new TabWidget(this);
    connect(tabWidget,SIGNAL(newTab()),this, SLOT(createNewTab()));
    connect(tabWidget,SIGNAL(closeTab(int)),this, SLOT(closeTab(int)));
    connect(tabWidget,SIGNAL(closeOtherTabs(int)), this, SLOT(closeOtherTabs(int)));
    connect(tabWidget,SIGNAL(renameTab(int)),this, SLOT(renameTab(int)));
    connect(tabWidget,SIGNAL(currentChanged(int)),SLOT(changeCurrentTab(int)));
    connect(tabWidget,SIGNAL(tabTextChanged(QString)),SLOT(onTabTextChanged()));

    QMap<int, SortableTreeWidgetItem::DataType> typeMap;
    typeMap.insert(0, SortableTreeWidgetItem::DataTypeInteger);
    typeMap.insert(7, SortableTreeWidgetItem::DataTypeInteger);
    typeMap.insert(2, SortableTreeWidgetItem::DataTypeDate);
    typeMap.insert(4, SortableTreeWidgetItem::DataTypeFloat);
    typeMap.insert(6, SortableTreeWidgetItem::DataTypeFloat);
    SortableTreeWidgetItem::setTypeMap(typeMap);


    autoscaleXAct = new QAction("Автомасштабирование по оси X", this);
    autoscaleXAct->setIcon(QIcon(":/icons/autoscale-x.png"));
    autoscaleXAct->setCheckable(true);
    bool autoscale = getSetting("autoscale-x", true).toBool();
    connect(autoscaleXAct, &QAction::toggled, [=](bool toggled){
        plot->toggleAutoscale(0 /* x axis */,toggled);
        setSetting("autoscale-x", toggled);
    });
    autoscaleXAct->setChecked(autoscale);
    plot->toggleAutoscale(0 /* x axis */, autoscale);

    autoscaleYAct = new QAction("Автомасштабирование по оси Y", this);
    autoscaleYAct->setIcon(QIcon(":/icons/autoscale-y-main.png"));
    autoscaleYAct->setCheckable(true);
    autoscale = getSetting("autoscale-y", true).toBool();
    connect(autoscaleYAct, &QAction::toggled, [=](bool toggled){
        plot->toggleAutoscale(1 /* y axis */,toggled);
        setSetting("autoscale-y", toggled);
    });
    autoscaleYAct->setChecked(autoscale);
    plot->toggleAutoscale(1 /* x axis */, autoscale);

    autoscaleYSlaveAct = new QAction("Автомасштабирование по правой оси Y", this);
    autoscaleYSlaveAct->setIcon(QIcon(":/icons/autoscale-y-slave.png"));
    autoscaleYSlaveAct->setCheckable(true);
    autoscale = getSetting("autoscale-y-slave", true).toBool();
    connect(autoscaleYSlaveAct, &QAction::toggled, [=](bool toggled){
        plot->toggleAutoscale(2 /* y slave axis */,toggled);
        setSetting("autoscale-y-slave", toggled);
    });
    autoscaleYSlaveAct->setChecked(autoscale);
    plot->toggleAutoscale(2 /* x axis */, autoscale);

    QToolBar *scaleToolBar = new QToolBar(this);
    scaleToolBar->setOrientation(Qt::Vertical);
    scaleToolBar->addAction(autoscaleXAct);
    scaleToolBar->addAction(autoscaleYAct);
    scaleToolBar->addAction(autoscaleYSlaveAct);

    QWidget *plotsWidget = new QWidget(this);
    QGridLayout *plotsLayout = new QGridLayout;
    plotsLayout->addWidget(scaleToolBar,0,0);
    plotsLayout->addWidget(plot,0,1);
    plotsWidget->setLayout(plotsLayout);

    splitter = new QSplitter(Qt::Vertical, this);
    splitter->addWidget(tabWidget);
    splitter->addWidget(plotsWidget);

    QByteArray mainSplitterState = getSetting("mainSplitterState").toByteArray();
    if (!mainSplitterState.isEmpty())
        splitter->restoreState(mainSplitterState);

    setCentralWidget(splitter);



    QVariantMap v = getSetting("folders1").toMap();
    //qDebug()<<v;
    if (v.isEmpty())
        createNewTab();
    else {
        QMapIterator<QString, QVariant> it(v);
        while (it.hasNext()) {
            it.next();
            createTab(it.key(), it.value().toStringList());
            tabsNames << it.key();
        }
    }
}

void MainWindow::createTab(const QString &name, const QStringList &folders)
{DD;
    tab = new Tab(this);
    tab->setOrientation(Qt::Horizontal);

    tab->tree = new QTreeWidget(this);
    tab->tree->setRootIsDecorated(false);
    tab->tree->setContextMenuPolicy(Qt::ActionsContextMenu);
    tab->tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    tab->tree->setDragEnabled(true);
    tab->tree->setDragDropMode(QAbstractItemView::InternalMove);
   // tab->tree->setDefaultDropAction(Qt::MoveAction);
    tab->tree->addAction(addFolderAct);
    tab->tree->addAction(addFileAct);
    tab->tree->addAction(delFilesAct);
    tab->tree->addAction(plotAllChannelsAct);
    tab->tree->addAction(plotAllChannelsAtRightAct);
//    tab->tree->addAction(removeChannelsPlotsAct);
    tab->tree->addAction(calculateSpectreAct);
    tab->tree->addAction(convertAct);

    tab->tree->setHeaderLabels(QStringList()
                          << QString("№")
                          << QString("Файл")
                          << QString("Дата")
                          << QString("Тип")
                          << QString("Размер")
                          << QString("Ось Х")
                          << QString("Шаг")
                          << QString("Каналы")
                          << QString("Описание")
                          << QString("Легенда")
            );
    tab->tree->header()->setStretchLastSection(false);
    tab->tree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    tab->tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    tab->tree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    tab->tree->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    tab->tree->header()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    tab->tree->header()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    tab->tree->header()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    tab->tree->header()->setSectionResizeMode(7, QHeaderView::ResizeToContents);

    QByteArray treeHeaderState = getSetting("treeHeaderState").toByteArray();
    if (!treeHeaderState.isEmpty())
        tab->tree->header()->restoreState(treeHeaderState);


    connect(tab->tree, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),SLOT(updateChannelsTable(QTreeWidgetItem*,QTreeWidgetItem*)));
    connect(tab->tree,SIGNAL(itemChanged(QTreeWidgetItem*,int)), SLOT(treeItemChanged(QTreeWidgetItem*, int)));
    tab->tree->sortByColumn(0, Qt::AscendingOrder);
//    tab->tree->setSortingEnabled(true);

    tab->channelsTable = new QTableWidget(0,6,this);

    tab->tableHeader = new CheckableHeaderView(Qt::Horizontal, tab->channelsTable);

    tab->channelsTable->setHorizontalHeader(tab->tableHeader);
    tab->tableHeader->setCheckState(0,Qt::Checked);
    tab->tableHeader->setCheckable(0,true);
    tab->tableHeader->setCheckState(0,Qt::Unchecked);
    connect(tab->tableHeader,SIGNAL(toggled(int,Qt::CheckState)),this,SLOT(headerToggled(int,Qt::CheckState)));

    tab->channelsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    tab->channelsTable->horizontalHeader()->setStretchLastSection(false);
    //connect(tab->channelsTable, SIGNAL(currentCellChanged(int,int,int,int)),this,SLOT(maybePlotChannel(int,int,int,int)));
    connect(tab->channelsTable, SIGNAL(itemChanged(QTableWidgetItem*)), SLOT(maybePlotChannel(QTableWidgetItem*)));

    tab->channelsTable->addAction(moveChannelsUpAct);
    tab->channelsTable->addAction(moveChannelsDownAct);

    tab->filePathLabel = new QLabel(this);
    tab->filePathLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);




    QWidget *treeWidget = new QWidget(this);
    QGridLayout *treeLayout = new QGridLayout;
    treeLayout->addWidget(tab->tree,0,0);
    treeWidget->setLayout(treeLayout);

    QToolButton *openFolderButton = new QToolButton(this);
    QAction *openFolderAct = new QAction("Открыть папку с этой записью", this);
    openFolderAct->setIcon(qApp->style()->standardIcon(QStyle::SP_DirOpenIcon));
    openFolderButton->setDefaultAction(openFolderAct);
    connect(openFolderAct, &QAction::triggered, [=](){
        if (!tab->filePathLabel->text().isEmpty()) {
            QDir dir(tab->filePathLabel->text());
            dir.cdUp();
            QProcess::startDetached("explorer.exe", QStringList(dir.toNativeSeparators(dir.absolutePath())));
        }
    });

    QWidget *channelsWidget = new QWidget(this);
    QGridLayout *channelsLayout = new QGridLayout;
    channelsLayout->addWidget(tab->filePathLabel,0,0);
    channelsLayout->addWidget(tab->channelsTable,1,0,1,2);
    channelsLayout->addWidget(openFolderButton, 0,1);
    channelsWidget->setLayout(channelsLayout);


    tab->addWidget(treeWidget);
    tab->addWidget(channelsWidget);



    QByteArray upperSplitterState = getSetting("upperSplitterState").toByteArray();
    if (!upperSplitterState.isEmpty())
        tab->restoreState(upperSplitterState);

    int i = tabWidget->addTab(tab, name);
    tabWidget->setCurrentIndex(i);

    foreach (QString folder, folders) {
        bool subfolders = folder.endsWith(":1");
        if (subfolders || folder.endsWith(":0"))
            folder.chop(2);
        QFileInfo fi(folder);
        if (fi.exists()) {
            addFolder(folder, subfolders, true);
        }
    }
}

void MainWindow::createNewTab()
{DD;
    static int sequenceNumber = 1;
    QString name = tr("Вкладка %1").arg(sequenceNumber);
    while (tabsNames.contains(name)) {
        sequenceNumber++;
        name = tr("Вкладка %1").arg(sequenceNumber);
    }
    createTab(name, QStringList());
    sequenceNumber++;
    tabsNames << name;
}

void MainWindow::closeTab(int i)
{DD;
    if (tabWidget->count()==1) return;
    int index=i;
    if (i<0) index = tabWidget->currentIndex();

    tabWidget->setCurrentIndex(index);
    //tab now points to current tab

    QVector<int> indexes;
    for (int row = 0; row < tab->tree->topLevelItemCount(); ++row)
        indexes << row;
    deleteFiles(indexes);

    QWidget *w = tabWidget->widget(index);
    tabWidget->removeTab(index);
    w->deleteLater();
}

void MainWindow::closeOtherTabs(int index)
{DD;
    int count=tabWidget->count();
    if (count<=1) return;

    for (int i = count - 1; i > index; --i)
        closeTab(i);
    for (int i = index - 1; i >= 0; --i)
        closeTab(i);
}

void MainWindow::renameTab(int i)
{DD;
    QString oldTabName = tabWidget->tabText(i);

    QString newTabName=QInputDialog::getText(this,
                                             tr("Переименование вкладки"),
                                             tr("Задайте новое имя"),
                                             QLineEdit::Normal,
                                             oldTabName);

    if (!newTabName.isEmpty()) {
        tabWidget->setTabText(i,newTabName);
    }
}

void MainWindow::changeCurrentTab(int currentIndex)
{DD;
    if (currentIndex<0) return;

    Tab *sp = qobject_cast<Tab *>(tabWidget->currentWidget());
    if (sp) {
        tab = sp;
//        tab->tree->setFocus();

    }
    else
        tab = 0;
}

void MainWindow::onTabTextChanged()
{DD;
    QString s = tabWidget->tabText(tabWidget->currentIndex());
    tabWidget->setTabText(tabWidget->currentIndex(),s);
}

void MainWindow::editColors()
{DD;
    ColorEditDialog dialog(this);
    dialog.exec();
}

MainWindow::~MainWindow()
{DD;
    setSetting("mainSplitterState",splitter->saveState());

    if (tab) {
        setSetting("upperSplitterState",tab->saveState());
        QByteArray treeHeaderState = tab->tree->header()->saveState();
        setSetting("treeHeaderState", treeHeaderState);
    }

    QVariantMap map;
    for (int i=0; i<tabWidget->count(); ++i) {
        Tab *t = qobject_cast<Tab *>(tabWidget->widget(i));
        if (t) {
            if (!t->folders.isEmpty())
                map.insert(tabWidget->tabText(i), t->folders);
        }
    }
    //qDebug()<<map;

    setSetting("folders1", map);

    for (int i= tabWidget->count()-1; i>=0; --i) {
        closeTab(i);
    }

    ColorSelector::instance()->drop();
}

bool checkForContains(Tab *tab, const QString &file, int *index = 0)
{DD;
    for (int i=0; i<tab->tree->topLevelItemCount(); ++i) {
        SortableTreeWidgetItem *item = dynamic_cast<SortableTreeWidgetItem *>(tab->tree->topLevelItem(i));
        if (item) {
            if (item->fileDescriptor->fileName() == file) {
                if (index) *index = i;
                return true;
            }
        }
    }

    if (index) *index = -1;
    return false;
}

void MainWindow::addFolder() /*SLOT*/
{DD;
    QString directory = getSetting("lastDirectory").toString();

    directory = QFileDialog::getExistingDirectory(this,
                                                  tr("Добавление папки"),
                                                  directory,
                                                  QFileDialog::ShowDirsOnly | QFileDialog::ReadOnly);

    if (directory.isEmpty()) return;
    setSetting("lastDirectory", directory);
    addFolder(directory, false, false);
}

void MainWindow::addFolderWithSubfolders() /*SLOT*/
{
    QString directory = getSetting("lastDirectory").toString();

    directory = QFileDialog::getExistingDirectory(this,
                                                  tr("Добавление папки со всеми вложенными папками"),
                                                  directory,
                                                  QFileDialog::ShowDirsOnly | QFileDialog::ReadOnly);

    if (directory.isEmpty()) return;
    setSetting("lastDirectory", directory);
    addFolder(directory, true, false);
}

void MainWindow::addFile()
{
    QString directory = getSetting("lastDirectory").toString();

    QFileDialog dialog(this, "Добавить файлы", directory/*, "Файлы dfd (*.dfd);;Файлы uff (*.uff)"*/);
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);
    QString defaultSuffix = QFileInfo(directory).suffix();
    if (defaultSuffix.isEmpty()) defaultSuffix = "dfd";
    dialog.setDefaultSuffix(defaultSuffix);
    QSortFilterProxyModel *proxy = new DfdUffFilterProxy(this);
    dialog.setProxyModel(proxy);
    dialog.setFileMode(QFileDialog::ExistingFiles);


    QStringList fileNames;
    if (dialog.exec()) {
        fileNames = dialog.selectedFiles();
    }
    if (fileNames.isEmpty()) return;

    setSetting("lastDirectory", fileNames.first());
    addFiles(fileNames);
    foreach (const QString &file, fileNames)
        if (!tab->folders.contains(file)) tab->folders << file;
}

void MainWindow::addFolder(const QString &directory, bool withAllSubfolders, bool silent)
{DD;
    if (directory.isEmpty() || !tab) return;

    QStringList filesToAdd;
    processDir(directory, filesToAdd, withAllSubfolders);

    QStringList toAdd;
    foreach (const QString &file, filesToAdd) {
        if (!checkForContains(tab, file))
            toAdd << file;
    }

    addFiles(toAdd);
    if (toAdd.size() < filesToAdd.size() && !silent) {
        QMessageBox::information(this,QString("База данных"),
                                 toAdd.isEmpty()?QString("Эти файлы уже есть в базе"):
                                                 QString("Только %1 файлов из %2 было добавлено")
                                                 .arg(toAdd.size())
                                                 .arg(filesToAdd.size()));
    }
    //успешное добавление нескольких файлов -> добавляем название папки в базу
    if (!toAdd.isEmpty()) {
        QString suffix = withAllSubfolders?":1":":0";
        if (!tab->folders.contains(directory+suffix))
            tab->folders.append(directory+suffix);
    }
}

void MainWindow::deleteFiles()
{DD;
    if (!tab) return;

    QVector<int> list;
    for (int i=0; i<tab->tree->topLevelItemCount(); ++i) {
        if (tab->tree->topLevelItem(i)->isSelected()) {
            list << i;
        }
    }
    if (!list.isEmpty())
        deleteFiles(list);

    if (tab->tree->topLevelItemCount() == 0)
        tab->folders.clear();
}

QList<QPair<FileDescriptor*, int> > MainWindow::selectedChannels()
{DD;
    QList<QPair<FileDescriptor*, int> > channels;

    foreach(Curve *c, plot->graphs) {
        channels << (QPair<FileDescriptor*, int>(c->descriptor, c->channelIndex));
    }

    return channels;
}

void MainWindow::deleteChannels() /** SLOT */
{DD;
    QList<QPair<FileDescriptor*, int> > channelsToDelete = selectedChannels();
    if (channelsToDelete.isEmpty()) return;

    if (QMessageBox::question(this,"DeepSea Base","Выделенные каналы будут \nудалены из файла. Продолжить?")==QMessageBox::Yes) {
        deleteChannels(channelsToDelete);

        QList<FileDescriptor*> list;
        for (int i=0; i<channelsToDelete.size(); ++i)
            if (!list.contains(channelsToDelete.at(i).first)) list << channelsToDelete.at(i).first;

        updateRecordsTable(list);
        updateChannelsTable(tab->record);
    }
}

void MainWindow::deleteChannelsBatch()
{
    QList<QPair<FileDescriptor*, int> > channels = selectedChannels();
    QList<FileDescriptor*> descriptorsList;
    for (int i=0; i<channels.size(); ++i)
        if (!descriptorsList.contains(channels.at(i).first)) descriptorsList << channels.at(i).first;
    bool allChannelsFromOneFile = descriptorsList.size()==1;

    QList<FileDescriptor*> filesToDelete;
    for (int i=0; i<tab->tree->topLevelItemCount(); ++i) {
        SortableTreeWidgetItem *item = dynamic_cast<SortableTreeWidgetItem *>(tab->tree->topLevelItem(i));
        if (item && item->isSelected()) {
            filesToDelete << item->fileDescriptor;
        }
    }

    // нет построенных каналов или выделен только один файл
    if (filesToDelete.size()<2 || channels.isEmpty()) return;

    if (QMessageBox::question(this,"DeepSea Base","Выделенные каналы будут \n"
                              "удалены из всех выделенных файлов. Продолжить?")!=QMessageBox::Yes)
        return;

    // проверка на выделенные каналы
    if (!allChannelsFromOneFile) {
        if (QMessageBox::question(this,"DeepSea Base","Выделены каналы из разных файлов.\n"
                                  "В качестве шаблона будет использован только первый файл. Продолжить?")==QMessageBox::Yes)
        {
            // удаляем из списка каналов все каналы других файлов
            for (int i=channels.size()-1; i>=0; --i) {
                if (channels.at(i).first != descriptorsList.first()) channels.removeAt(i);
            }
            if (channels.isEmpty()) return;
        }
        else {
            return;
        }
    }

    QVector<int> channelsInds;
    for (int i=0; i<channels.size(); ++i) channelsInds << channels.at(i).second;
    qSort(channelsInds);

    // проверка на количество каналов в файлах
    foreach (FileDescriptor *d, filesToDelete) {
        if (d->channelsCount()<=channelsInds.last()) {
            if (QMessageBox::question(this,"DeepSea Base","В некоторых файлах меньше каналов, чем заявлено\n"
                                      "к удалению. Продолжить?")==QMessageBox::Yes)
                break;
            else
                return;
        }
    }

    foreach (FileDescriptor *d, filesToDelete) {
        foreach (int index, channelsInds) {
            QPair<FileDescriptor*, int> pair = {d, index};
            if (!channels.contains(pair)) channels << pair;

        }
    }
    deleteChannels(channels);

    updateRecordsTable(filesToDelete);
    updateChannelsTable(tab->record);
}

void MainWindow::copyChannels() /** SLOT */
{DD;
    QList<QPair<FileDescriptor*, int> > channelsToCopy = selectedChannels();
    if (channelsToCopy.isEmpty()) return;

    if (copyChannels(channelsToCopy)) {
        QList<FileDescriptor*> list;
        for (int i=0; i<channelsToCopy.size(); ++i)
            if (!list.contains(channelsToCopy.at(i).first)) list << channelsToCopy.at(i).first;

        updateRecordsTable(list);
        updateChannelsTable(tab->record);
    }
}

void MainWindow::moveChannels() /** SLOT */
{DD;
    // сначала копируем каналы, затем удаляем
    QList<QPair<FileDescriptor*, int> > channelsToMove = selectedChannels();
    if (channelsToMove.isEmpty()) return;

    if (QMessageBox::question(this,"DeepSea Base","Выделенные каналы будут \nудалены из файла. Продолжить?")==QMessageBox::Yes) {
        if (copyChannels(channelsToMove)) {
            deleteChannels(channelsToMove);
            QList<FileDescriptor*> list;
            for (int i=0; i<channelsToMove.size(); ++i)
                if (!list.contains(channelsToMove.at(i).first)) list << channelsToMove.at(i).first;

            updateRecordsTable(list);
            updateChannelsTable(tab->record);
        }
    }
}

void MainWindow::addCorrection()
{DD;
    if (plot->hasGraphs()) {
        QList<FileDescriptor*> filesToDelete;
        for (int i=0; i<tab->tree->topLevelItemCount(); ++i) {
            SortableTreeWidgetItem *item = dynamic_cast<SortableTreeWidgetItem *>(tab->tree->topLevelItem(i));
            if (item && item->isSelected()) {
                filesToDelete << item->fileDescriptor;
            }
        }

        CorrectionDialog dialog(plot, filesToDelete);
        dialog.exec();

        updateChannelsTable(tab->record);
    }
}

void MainWindow::addCorrections()
{
    static QString pathToExcelFile;
    pathToExcelFile = QFileDialog::getOpenFileName(this, "Выбрать файл xls с поправками", pathToExcelFile, "*.xls;;*.xlsx");

    if (pathToExcelFile.isEmpty()) return;

    QAxObject *excel = 0;

    if (!excel) excel = new QAxObject("Excel.Application", this);
    //excel->setProperty("Visible", true);


    //получаем рабочую книгу
    QAxObject * workbooks = excel->querySubObject("WorkBooks");
    workbooks->dynamicCall("Open(QString)", pathToExcelFile);

//            QFile file1("Excel.Workbooks.html");
//            file1.open(QIODevice::WriteOnly | QIODevice::Text);
//            QTextStream out(&file1);
//            out << workbooks->generateDocumentation();
//            file1.close();
//    return;


    QAxObject * workbook = excel->querySubObject("ActiveWorkBook");
    if (!workbook) {
        return;
    }
    QAxObject * worksheet = workbook->querySubObject("ActiveSheet");
    if (!worksheet) {
        return;
    }

    int column = 1;
    while (1) {
        QAxObject *cells = worksheet->querySubObject("Cells(Int,Int)", 1, column);
        QString fileName = cells->property("Value").toString();
        if (fileName.isEmpty()) break;

       // qDebug()<<fileName;
        if (QFile(fileName).exists()) {
            fileName.replace("\\","/");
            FileDescriptor *dfd = findDescriptor(fileName);
            bool deleteAfter=false;
            if (!dfd) {
                dfd = createDescriptor(fileName);
                dfd->read();
                deleteAfter = true;
            }
            dfd->populate();

            int row = 2;
            bool corrected = false;
            while (1) {
                QAxObject *cells1 = worksheet->querySubObject("Cells(Int,Int)", row, column);
                QString value = cells1->property("Value").toString();
               // qDebug()<<value;
                if (value.isEmpty()) break;
                if (row-2>=dfd->channelsCount()) break;

                corrected = true;
                double corr = value.toDouble();
                if (corr != 0.0)
                    dfd->channel(row-2)->addCorrection(corr, true);
                row++;
            }
            if (corrected) {
                dfd->setChanged(true);
                dfd->setDataChanged(true);
                dfd->write();
                dfd->writeRawFile();
            }
            if (deleteAfter) delete dfd;
        }

        column++;
    }

    updateChannelsTable(tab->record);
    plot->updateAxes();
    plot->updateLegends();
    plot->replot();


    delete worksheet;
    delete workbook;
    delete workbooks;

    excel->dynamicCall("Quit()");
    delete excel;

    QMessageBox::information(this,"","Готово!");
}

bool MainWindow::deleteChannels(const QList<QPair<FileDescriptor*, int> > &channelsToDelete)
{DD;
    for (int i=0; i<channelsToDelete.size(); ++i)
        plot->deleteGraph(channelsToDelete.at(i).first, channelsToDelete.at(i).second, true);

    QList<FileDescriptor*> allDescriptors;
    for (int i=0; i<channelsToDelete.size(); ++i)
        if (!allDescriptors.contains(channelsToDelete.at(i).first))
            allDescriptors << channelsToDelete.at(i).first;

    while (!allDescriptors.isEmpty()) {
        FileDescriptor *dfd = allDescriptors.first();
        dfd->populate();
        QVector<int> indexes;
        for (int i=0; i<channelsToDelete.size(); ++i) {
            if (channelsToDelete.at(i).first == dfd) indexes << channelsToDelete.at(i).second;
        }
        dfd->deleteChannels(indexes);
        allDescriptors.removeFirst();
    }

    return true;
}

bool MainWindow::copyChannels(const QList<QPair<FileDescriptor *, int> > &channelsToCopy)
{DD;
    bool allFilesDfd = true;
    for (int i=0; i<channelsToCopy.size(); ++i) {
        if (!channelsToCopy.at(i).first->fileName().toLower().endsWith(".dfd")) {
            allFilesDfd = false;
            break;
        }
    }

    bool oneFolder = true;
    QString startFolder;

    for (int i=0; i<channelsToCopy.size(); ++i) {
        if (startFolder.isEmpty()) startFolder = QFileInfo(channelsToCopy.at(i).first->fileName()).canonicalPath();
        else if (QFileInfo(channelsToCopy.at(i).first->fileName()).canonicalPath() != startFolder) {
            oneFolder = false;
            break;
        }
    }

    /* Сначала определяем тип файла и шаг по оси x */
    QString startFile = MainWindow::getSetting("startDir").toString();
    if (oneFolder) startFile = startFolder;

    QFileDialog dialog(this, "Выбор файла для записи каналов", startFile,
                       "Файлы dfd (*.dfd);;Файлы uff (*.uff);;Файлы unv (*.unv)");
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);
    QString defaultSuffix = QFileInfo(startFile).suffix();
    if (defaultSuffix.isEmpty()) defaultSuffix = "dfd";
    dialog.setDefaultSuffix(defaultSuffix);

    QStringList suffixes = QStringList()<<".dfd"<<".uff"<<".unv";
    QStringList filters = QStringList()<<"Файлы dfd (*.dfd)"<<"Файлы uff (*.uff)"<<"Файлы unv (*.unv)";

    bool forceUff = false;



    // если все файлы - dfd, то мы можем записать их только в dfd файл с таким же типом данных и шагом по x
    // поэтому если они различаются, то насильно записываем каналы в файл uff
    if (allFilesDfd) {
        //определяем тип данных и шаг по x
        FileDescriptor *dfd = channelsToCopy.first().first;
        double xStep = dfd->channel(channelsToCopy.first().second)->xStep();

        // мы не можем записывать каналы с разным типом/шагом в один файл,
        //поэтому добавляем фильтр
        bool filesSame = true;

        for (int i=1; i<channelsToCopy.size(); ++i) {
            if (!channelsToCopy.at(i).first->dataTypeEquals(dfd)
                || channelsToCopy.at(i).first->channel(channelsToCopy.at(i).second)->xStep()!=xStep) {
                filesSame = false;
                break;
            }
        }
        if (filesSame) {
            QSortFilterProxyModel *proxy = new DfdFilterProxy(dfd, this);
            dialog.setProxyModel(proxy);
        }
        else {
            if (QMessageBox::question(this,"DeepSea Base",
                                      "Выделенные каналы имеют разный тип данных и/или разрешение по частоте.\n"
                                      "Изменить тип файла на uff?","Да, записать каналы в файл uff с тем же именем",
                                      "Нет, прервать операцию")
                ==1) {
                return false;
            }
            else {
                forceUff = true;
            }
        }
    }
    dialog.setFileMode(QFileDialog::AnyFile);


    QStringList selectedFiles;
    QString selectedFilter;
    if (dialog.exec()) {
        selectedFiles = dialog.selectedFiles();
        selectedFilter = dialog.selectedNameFilter();
    }
    if (selectedFiles.isEmpty()) return false;

    QString file = selectedFiles.first();

    if (!file.endsWith(suffixes.at(filters.indexOf(selectedFilter))))
        file.append(suffixes.at(filters.indexOf(selectedFilter)));
    if (forceUff) {
        file.chop(4);
        file.append(".uff");
    }

    MainWindow::setSetting("startDir", file);

    // ИЩЕМ ЭТОТ ФАЙЛ СРЕДИ ДОБАВЛЕННЫХ В БАЗУ
    FileDescriptor *dfd = findDescriptor(file);

    if (!dfd) {//не нашли файл в базе, нужно создать новый объект
        if (QFileInfo(file).exists()) {
            // добавляем каналы в существующий файл
            dfd = createDescriptor(file);
            dfd->read();
        }
        else {
            FileDescriptor *firstdfd = channelsToCopy.first().first;
            // такого файла не существует, создаем новый файл и записываем в него каналы
            dfd = createDescriptor(file);
            dfd->fillPreliminary(firstdfd->type());
        }
        dfd->copyChannelsFrom(channelsToCopy);
        dfd->fillRest();

        dfd->setChanged(true);
        dfd->setDataChanged(true);
        dfd->write();
        dfd->writeRawFile();
        addFile(dfd);
    }
    else {
        dfd->copyChannelsFrom(channelsToCopy);
        updateFile(dfd);
    }
    return true;
}

void MainWindow::calculateMean()
{DD;
    const int graphsSize = plot->graphsCount();
    if (graphsSize<2) return;

    QList<QPair<FileDescriptor *, int> > channels;

    bool dataTypeEqual = true;
    bool stepsEqual = true; // одинаковый шаг по оси Х
    bool namesEqual = true; // одинаковые названия осей Y
    bool oneFile = true; // каналы из одного файла
    bool writeToSeparateFile = true;
    bool writeToUff = false;

    Curve *firstCurve = plot->graphs.first();
    channels.append({firstCurve->descriptor, firstCurve->channelIndex});

    bool allFilesDfd = firstCurve->descriptor->fileName().toLower().endsWith("dfd");

    for (int i = 1; i<graphsSize; ++i) {
        Curve *curve = plot->graphs.at(i);
        channels.append({curve->descriptor, curve->channelIndex});

        allFilesDfd &= firstCurve->descriptor->fileName().toLower().endsWith("dfd");
        if (firstCurve->channel->xStep() != curve->channel->xStep())
            stepsEqual = false;
        if (firstCurve->channel->yName() != curve->channel->yName())
            namesEqual = false;
        if (firstCurve->descriptor->fileName() != curve->descriptor->fileName())
            oneFile = false;
        if (firstCurve->channel->type() != curve->channel->type())
            dataTypeEqual = false;
    }
    if (!dataTypeEqual) {
        int result = QMessageBox::warning(0, "Среднее графиков",
                                          "Графики имеют разный тип. Среднее будет записано в файл uff. Продолжить?",
                                          "Да","Нет");
        if (result == 1) return;
        else writeToUff = true;
    }
    if (!namesEqual) {
        int result = QMessageBox::warning(0, "Среднее графиков",
                                          "Графики по-видимому имеют разный тип. Среднее будет записано в файл uff. Продолжить?",
                                          "Да", "Нет");
        if (result == 1) return;
        else writeToUff = true;
    }
    if (!stepsEqual) {
        int result = QMessageBox::warning(0, "Среднее графиков",
                                          "Графики имеют разный шаг по оси X. Среднее будет записано в файл uff. Продолжить?",
                                          "Да", "Нет");
        if (result == 1) return;
        else writeToUff = true;
    }
    if (oneFile) {
        QMessageBox box("Среднее графиков", QString("Графики взяты из одной записи %1.\n").arg(firstCurve->descriptor->fileName())
                        +
                        QString("Сохранить среднее в эту запись дополнительным каналом?"),
                        QMessageBox::Question,
                        QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel);
        box.setButtonText(QMessageBox::Yes, "Да, записать в этот файл");
        box.setButtonText(QMessageBox::No, "Нет, экспортировать в отдельный файл");
        box.setEscapeButton(QMessageBox::Cancel);

        int result = box.exec();
        if (result == QMessageBox::Cancel) return;
        writeToSeparateFile = (result == QMessageBox::No);
    }

    QString meanDfdFile;
    FileDescriptor *meanDfd = 0;
    bool descriptorFound = false;

    if (writeToSeparateFile) {
        QString meanD = firstCurve->descriptor->fileName();
        meanD.chop(4);
        if (writeToUff) meanD.append(".uff");

        meanD = MainWindow::getSetting(writeToUff?"lastMeanUffFile":"lastMeanFile", meanD).toString();

        QFileDialog dialog(this, "Выбор файла для записи каналов", meanD,
                           "Поддерживаемые форматы (*.dfd *.uff *.unv)");
        dialog.setOption(QFileDialog::DontUseNativeDialog, true);
        dialog.setFileMode(QFileDialog::AnyFile);
        dialog.setDefaultSuffix(writeToUff?"uff":"dfd");

        if (!writeToUff && allFilesDfd) {
            QSortFilterProxyModel *proxy = new DfdFilterProxy(firstCurve->descriptor, this);
            dialog.setProxyModel(proxy);
        }

        QStringList selectedFiles;
        if (dialog.exec()) {
            selectedFiles = dialog.selectedFiles();
        }
        if (selectedFiles.isEmpty()) return;

        meanDfdFile = selectedFiles.first();
        if (meanDfdFile.isEmpty()) return;

        meanDfd = findDescriptor(meanDfdFile);
        if (meanDfd)
            descriptorFound = true;
        else
            meanDfd = createDescriptor(meanDfdFile);

        if (!meanDfd) return;

        if (QFileInfo(meanDfdFile).exists() && !descriptorFound)
            meanDfd->read();
        else
            meanDfd->fillPreliminary(firstCurve->channel->type());

        MainWindow::setSetting(writeToUff?"lastMeanUffFile":"lastMeanFile", meanDfdFile);
    }
    else {
        meanDfd = firstCurve->descriptor;
        meanDfdFile = meanDfd->fileName();
        descriptorFound = true;
    }

    meanDfd->calculateMean(channels);

    if (writeToSeparateFile)
        meanDfd->fillRest();

    meanDfd->setChanged(true);
    meanDfd->setDataChanged(true);
    meanDfd->write();
    meanDfd->writeRawFile();

    if (descriptorFound) {
        updateFile(meanDfd);
    }
    else {
        addFile(meanDfd);
    }
    setCurrentAndPlot(meanDfd, meanDfd->channelsCount()-1);
}

void MainWindow::moveChannelsUp()
{DD;
    moveChannels(true);
}

void MainWindow::moveChannelsDown()
{
    moveChannels(false);
}

void MainWindow::editDescriptions()
{
    QList<FileDescriptor *> records;
    for (int i=0; i<tab->tree->topLevelItemCount(); ++i) {
        if (tab->tree->topLevelItem(i)->isSelected()) {
            SortableTreeWidgetItem *item = dynamic_cast<SortableTreeWidgetItem *>(tab->tree->topLevelItem(i));
            records << item->fileDescriptor;
        }
    }

    EditDescriptionsDialog dialog(records, this);
    if (dialog.exec()) {
        QHash<FileDescriptor*,DescriptionList> descriptions = dialog.descriptions();
        QHashIterator<FileDescriptor*,DescriptionList> it(descriptions);
        while (it.hasNext()) {
            it.next();
            it.key()->setDataDescriptor(it.value());
            it.key()->setChanged(true);
            it.key()->write();

            for (int j=0; j<tab->tree->topLevelItemCount(); ++j) {
                SortableTreeWidgetItem *item = dynamic_cast<SortableTreeWidgetItem *>(tab->tree->topLevelItem(j));
                if (item->fileDescriptor == it.key()) {
                    item->setText(8, it.key()->dataDescriptorAsString());
                }
            }
        }
    }
}

void MainWindow::save()
{
    for (int i=0; i<tabWidget->count(); ++i) {
        Tab *t = qobject_cast<Tab *>(tabWidget->widget(i));
        if (t) {
            for (int i=0; i<t->tree->topLevelItemCount(); ++i) {
                SortableTreeWidgetItem *item = dynamic_cast<SortableTreeWidgetItem *>(tab->tree->topLevelItem(i));
                FileDescriptor *d = item->fileDescriptor;
                d->write();
                d->writeRawFile();
            }
        }
    }
}

void MainWindow::convertMatFiles()
{
    MatlabConverterDialog dialog(this);
    if (dialog.exec()) {
        if (dialog.addFiles()) {
            QStringList files = dialog.getConvertedFiles();
            this->addFiles(files);
//            foreach (const QString &file, files)
//                if (!tab->folders.contains(file)) tab->folders << file;
        }
    }
}

void MainWindow::convertEsoFiles()
{
    EsoConverterDialog dialog(this);
    if (dialog.exec()) {
        QStringList files = dialog.getConvertedFiles();
        if (files.isEmpty()) return;
        this->addFiles(files);
        foreach (const QString &file, files)
            if (!tab->folders.contains(file)) tab->folders << file;
    }
}

QVector<int> computeIndexes(QVector<int> notYetMoved, bool up, int totalSize)
{DD;
    QVector<int> moved;
    if (up) {
        while (notYetMoved.size()>0) {
            int j=notYetMoved.first();
            notYetMoved.pop_front();
            if (j==0 || moved.contains(j-1)) moved << j;
            else moved << j-1;
        }
    }
    else {
        int lastIndex = totalSize-1;
        while (notYetMoved.size()>0) {
            int j=notYetMoved.last();
            notYetMoved.pop_back();
            if (j==lastIndex || moved.contains(j+1)) moved.prepend(j);
            else moved.prepend(j+1);
        }
    }
    return moved;
}

void MainWindow::moveChannels(bool up)
{DD;
    QVector<int> indexes;
    for (int i=0; i<tab->channelsTable->rowCount(); ++i)
        if (tab->channelsTable->item(i,0)->isSelected())
            indexes << i;

    QVector<int> newIndexes = computeIndexes(indexes, up, tab->record->channelsCount());

    tab->record->move(up, indexes, newIndexes);

    updateChannelsTable(tab->record);

    tab->channelsTable->blockSignals(true);

    tab->channelsTable->clearSelection();
    Q_FOREACH (const int &i, newIndexes)
        tab->channelsTable->item(i,0)->setSelected(true);
    tab->channelsTable->blockSignals(false);
}

void MainWindow::updateChannelsHeaderState()
{DD;
    if (!tab) return;
    int checked=0;
    const int column = 0;
    for (int i=0; i<tab->channelsTable->rowCount(); ++i) {
        if (tab->channelsTable->item(i, column)
            && tab->channelsTable->item(i,column)->checkState()==Qt::Checked)
            checked++;
    }

    if (checked==0)
        tab->tableHeader->setCheckState(column, Qt::Unchecked);
    else if (checked==tab->channelsTable->rowCount())
        tab->tableHeader->setCheckState(column, Qt::Checked);
    else
        tab->tableHeader->setCheckState(column, Qt::PartiallyChecked);
}

void MainWindow::updateChannelsTable(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{DD;
    Q_UNUSED(previous)
    if (!tab) return;
    if (!current /*|| current==previous*/) return;

    SortableTreeWidgetItem *item = dynamic_cast<SortableTreeWidgetItem *>(current);
    if (!item) {
        tab->record = 0;
        return;
    }

    updateChannelsTable(item->fileDescriptor);


}

void MainWindow::updateChannelsTable(FileDescriptor *dfd)
{DD;
    if (!dfd) return;
    tab->channelsTable->blockSignals(true);
    tab->channelsTable->setRowCount(0);
    tab->channelsTable->blockSignals(false);

    if (!dfd->fileExists()) {
        QMessageBox::warning(this,"Не могу получить список каналов","Такого файла уже нет");
        return;
    }

    tab->record = dfd;
    tab->filePathLabel->setText(tab->record->fileName());

    int chanCount = tab->record->channelsCount();
    if (chanCount == 0) return;

    QStringList headers = tab->record->getHeadersForChannel(0);

    tab->channelsTable->blockSignals(true);
    tab->channelsTable->clear();
    tab->channelsTable->setColumnCount(headers.size());
    tab->channelsTable->setHorizontalHeaderLabels(headers);
    tab->channelsTable->setRowCount(0);
    tab->channelsTable->setRowCount(chanCount);

    QFont boldFont = tab->channelsTable->font();
    boldFont.setBold(true);

    for (int i=0; i<chanCount; ++i) {
        Channel *ch = tab->record->channel(i);
        QStringList data = ch->getInfoData();
        Qt::CheckState state = ch->checkState();
        for (int col=0; col<headers.size(); ++col) {
            QTableWidgetItem *ti = new QTableWidgetItem(data.at(col));
            if (col==0) {
                ti->setCheckState(state);
                if (state==Qt::Checked) ti->setFont(boldFont);
                if (ch->color().isValid()) {
                    ti->setTextColor(Qt::white);
                    ti->setBackgroundColor(ch->color());
                }
//                if (ch->type()==Descriptor::TimeResponse && ch->samplesCount()>32768)
//                    ti->setFlags(Qt::ItemIsSelectable);
            }
            tab->channelsTable->setItem(i,col,ti);
        }
    }
    updateChannelsHeaderState();
    tab->channelsTable->blockSignals(false);
}

void MainWindow::updateRecordsTable(const QList<FileDescriptor *> &records)
{
    foreach (FileDescriptor *dfd, records) {
        updateFile(dfd);
    }
}

void MainWindow::maybePlotChannel(QTableWidgetItem *item)
{DD;
    if (!tab) return;
    if (!item) return;
    int column = item->column();
    Channel *ch = tab->record->channel(item->row());

    if (tab->channelsTable->horizontalHeaderItem(column)->text() == "Описание") {
        ch->setDescription(item->text());
        tab->record->setChanged(true);
        tab->record->write();

        if (tab->tree->selectedItems().size()>1) {
            if (QMessageBox::question(this,"DeepSea Base","Выделено несколько файлов. Записать такое описание канала\n"
                                      "во все эти файлы?")==QMessageBox::Yes)
            {
                for (int i=0; i<tab->tree->topLevelItemCount(); ++i) {
                    SortableTreeWidgetItem *it = dynamic_cast<SortableTreeWidgetItem *>(tab->tree->topLevelItem(i));
                    if (it->fileDescriptor == tab->record) continue;
                    if (it && it->isSelected()) {
                        if (it->fileDescriptor->channel(item->row())) {
                            it->fileDescriptor->channel(item->row())->setDescription(item->text());
                            it->fileDescriptor->setChanged(true);
                            it->fileDescriptor->write();
                        }
                    }
                }
            }
        }
    }

    else if (column == 0) {
        if (ch->name() != item->text()) {
            ch->setName(item->text());
            tab->record->setChanged(true);
            tab->record->write();

            if (tab->tree->selectedItems().size()>1) {
                if (QMessageBox::question(this,"DeepSea Base","Выделено несколько файлов. Записать такое название канала\n"
                                          "во все эти файлы?")==QMessageBox::Yes)
                {
                    for (int i=0; i<tab->tree->topLevelItemCount(); ++i) {
                        SortableTreeWidgetItem *it = dynamic_cast<SortableTreeWidgetItem *>(tab->tree->topLevelItem(i));
                        if (it->fileDescriptor == tab->record) continue;
                        if (it && it->isSelected()) {
                            if (it->fileDescriptor->channel(item->row())) {
                                it->fileDescriptor->channel(item->row())->setName(item->text());
                                it->fileDescriptor->setChanged(true);
                                it->fileDescriptor->write();
                            }
                        }
                    }
                }
            }

            plot->updateLegends();
            return;
        }

        const Qt::CheckState state = item->checkState();
        bool plotOnRight = item->data(Qt::UserRole+2).toBool();


        QFont oldFont = tab->channelsTable->font();
        QFont boldFont = oldFont;
        boldFont.setBold(true);

//        if (state == Qt::Checked && !ch->populated())
//            ch->populate();

        tab->channelsTable->blockSignals(true);
        bool plotted = true;

        if (state == Qt::Checked) {
            QColor col;
            plotted = plot->plotChannel(tab->record, item->row(), &col, plotOnRight);
            if (plotted) {
                item->setFont(boldFont);
                ch->setCheckState(Qt::Checked);
                ch->setColor(col);
                item->setBackgroundColor(col);
                item->setTextColor(Qt::white);
            }
            else {
                item->setCheckState(Qt::Unchecked);
                ch->setCheckState(Qt::Unchecked);
                ch->setColor(QColor());
                item->setTextColor(Qt::black);
            }

            if (tab->tree->selectedItems().size()>1 && QApplication::keyboardModifiers() & Qt::ControlModifier) {
                for (int i=0; i<tab->tree->topLevelItemCount(); ++i) {
                    SortableTreeWidgetItem *it = dynamic_cast<SortableTreeWidgetItem *>(tab->tree->topLevelItem(i));
                    if (it->fileDescriptor == tab->record || !it->isSelected()) continue;
                    if (it->fileDescriptor->channelsCount()<=item->row()) continue;

                    plotted = plot->plotChannel(it->fileDescriptor, item->row(), &col, plotOnRight);
                    if (plotted) {
                        it->fileDescriptor->channel(item->row())->setCheckState(Qt::Checked);
                        it->fileDescriptor->channel(item->row())->setColor(col);
                    }
                    else {
                        it->fileDescriptor->channel(item->row())->setCheckState(Qt::Unchecked);
                        it->fileDescriptor->channel(item->row())->setColor(QColor());
                    }
                    it->setFont(1, it->fileDescriptor->allUnplotted()?oldFont:boldFont);
                }
            }
        }
        else if (state == Qt::Unchecked) {
            plot->deleteGraph(tab->record, item->row());
            item->setFont(tab->channelsTable->font());
            ch->setCheckState(Qt::Unchecked);
            item->setBackgroundColor(Qt::white);
            item->setTextColor(Qt::black);
            ch->setColor(QColor());

            if (tab->tree->selectedItems().size()>1 && QApplication::keyboardModifiers() & Qt::ControlModifier) {
                for (int i=0; i<tab->tree->topLevelItemCount(); ++i) {
                    SortableTreeWidgetItem *it = dynamic_cast<SortableTreeWidgetItem *>(tab->tree->topLevelItem(i));
                    if (it->fileDescriptor == tab->record || !it->isSelected()) continue;
                    if (it->fileDescriptor->channelsCount()<=item->row()) continue;

                    plot->deleteGraph(it->fileDescriptor, item->row());
                    it->fileDescriptor->channel(item->row())->setCheckState(Qt::Unchecked);
                    it->fileDescriptor->channel(item->row())->setColor(QColor());

                    it->setFont(1, it->fileDescriptor->allUnplotted()?oldFont:boldFont);
                }
            }
        }
        tab->channelsTable->blockSignals(false);

        tab->tree->currentItem()->setFont(1, tab->record->allUnplotted()?oldFont:boldFont);

        if (tab->tableHeader->isSectionCheckable(column))
            updateChannelsHeaderState();
    }
}

void MainWindow::plotAllChannels()
{DD;
    if (!tab) return;
    for (int i=0; i<tab->channelsTable->rowCount(); ++i) {
        tab->channelsTable->item(i,0)->setCheckState(Qt::Checked);
    }
}

void MainWindow::plotAllChannelsAtRight()
{
    if (!tab) return;
    for (int i=0; i<tab->channelsTable->rowCount(); ++i) {
        tab->channelsTable->item(i,0)->setData(Qt::UserRole+2, true);
        tab->channelsTable->item(i,0)->setCheckState(Qt::Checked);
    }
}

void MainWindow::calculateSpectreRecords()
{DD;
    if (!tab) return;
    QList<FileDescriptor *> records;
    for (int i=0; i<tab->tree->topLevelItemCount(); ++i) {
        if (tab->tree->topLevelItem(i)->isSelected()) {
            SortableTreeWidgetItem *item = dynamic_cast<SortableTreeWidgetItem *>(tab->tree->topLevelItem(i));
            if (!item) continue;

            if (item->fileDescriptor->isSourceFile()) {
                // only convert source files
                records << item->fileDescriptor;
            }
        }
    }
    if (records.isEmpty()) {
        QMessageBox::warning(this,QString("DeepSea Base"),
                             QString("Не выделено ни одного файла с исходными временными данными"));
        return;
    }

    CalculateSpectreDialog dialog(&records, this);

    dialog.exec();

    QStringList newFiles = dialog.getNewFiles();
    addFiles(newFiles);

}

void MainWindow::convertFiles()
{
    if (!tab) return;
    QList<FileDescriptor *> records;
    for (int i=0; i<tab->tree->topLevelItemCount(); ++i) {
        if (tab->tree->topLevelItem(i)->isSelected()) {
            SortableTreeWidgetItem *item = dynamic_cast<SortableTreeWidgetItem *>(tab->tree->topLevelItem(i));
            if (!item) continue;
            records << item->fileDescriptor;
        }
    }
//    if (records.isEmpty()) {
//        QMessageBox::warning(this,QString("DeepSea Base"),
//                             QString("Не выделено ни одного файла"));
//        return;
//    }

    ConverterDialog dialog(records, this);

    dialog.exec();

    QStringList newFiles = dialog.getConvertedFiles();
    addFiles(newFiles);
}

void MainWindow::calculateThirdOctave()
{DD;
    if (!tab) return;
    QList<FileDescriptor *> records;
    for (int i=0; i<tab->tree->topLevelItemCount(); ++i) {
        if (tab->tree->topLevelItem(i)->isSelected()) {
            SortableTreeWidgetItem *item = dynamic_cast<SortableTreeWidgetItem *>(tab->tree->topLevelItem(i));
            if (!item) continue;

            if (item->fileDescriptor->type() > Descriptor::TimeResponse) {
                // only convert spectres
                records << item->fileDescriptor;
            }
        }
    }
    if (records.isEmpty()) {
        QMessageBox::warning(this,QString("DeepSea Base"),
                             QString("Не выделено ни одного файла со спектрами"));
        return;
    }


    foreach (FileDescriptor *fd, records) {
        FileDescriptor *dfd = fd->calculateThirdOctave();
        if (findDescriptor(dfd)) updateFile(dfd);
        else addFile(dfd);
    }
}

void MainWindow::calculateMovingAvg()
{
    const int graphsSize = plot->graphsCount();
    if (graphsSize<1) return;

    int windowSize = MainWindow::getSetting("movingAvgSize",3).toInt();
    bool ok;
    windowSize = QInputDialog::getInt(this,"Скользящее среднее","Выберите величину окна усреднения",windowSize,
                                      3,15,2,&ok);
    if (ok)
        MainWindow::setSetting("movingAvgSize",windowSize);
    else
        return;

    QList<QPair<FileDescriptor *, int> > channels;

    bool oneFile = true; // каналы из одного файла
    bool writeToSeparateFile = true;

    Curve *firstCurve = plot->graphs.first();
    channels.append({firstCurve->descriptor, firstCurve->channelIndex});

    bool allFilesDfd = firstCurve->descriptor->fileName().toLower().endsWith("dfd");

    for (int i = 1; i<graphsSize; ++i) {
        Curve *curve = plot->graphs.at(i);
        channels.append({curve->descriptor, curve->channelIndex});

        allFilesDfd &= firstCurve->descriptor->fileName().toLower().endsWith("dfd");
        if (firstCurve->descriptor->fileName() != curve->descriptor->fileName())
            oneFile = false;
    }
    if (oneFile) {
        QMessageBox box("Скользящее среднее каналов", QString("Каналы взяты из одной записи %1.\n").arg(firstCurve->descriptor->fileName())
                        +
                        QString("Сохранить скользящее среднее в эту запись дополнительными каналами?"),
                        QMessageBox::Question,
                        QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel);
        box.setButtonText(QMessageBox::Yes, "Да, записать в этот файл");
        box.setButtonText(QMessageBox::No, "Нет, экспортировать в отдельный файл");
        box.setEscapeButton(QMessageBox::Cancel);

        int result = box.exec();
        if (result == QMessageBox::Cancel) return;
        writeToSeparateFile = (result == QMessageBox::No);
    }

    QString avgDfdFile;
    FileDescriptor *avgDfd = 0;
    bool descriptorFound = false;

    if (writeToSeparateFile) {
        QString meanD = firstCurve->descriptor->fileName();
        meanD.chop(4);

        meanD = MainWindow::getSetting("lastMovingAvgFile", meanD).toString();

        QFileDialog dialog(this, "Выбор файла для записи каналов", meanD,
                           "Поддерживаемые форматы (*.dfd *.uff *.unv)");
        dialog.setOption(QFileDialog::DontUseNativeDialog, true);
        dialog.setFileMode(QFileDialog::AnyFile);
        dialog.setDefaultSuffix("uff");

        if (allFilesDfd) {
            QSortFilterProxyModel *proxy = new DfdFilterProxy(firstCurve->descriptor, this);
            dialog.setProxyModel(proxy);
        }

        QStringList selectedFiles;
        if (dialog.exec()) {
            selectedFiles = dialog.selectedFiles();
        }
        if (selectedFiles.isEmpty()) return;

        avgDfdFile = selectedFiles.first();
        if (avgDfdFile.isEmpty()) return;

        avgDfd = findDescriptor(avgDfdFile);
        if (avgDfd)
            descriptorFound = true;
        else
            avgDfd = createDescriptor(avgDfdFile);

        if (!avgDfd) return;

        if (QFileInfo(avgDfdFile).exists() && !descriptorFound)
            avgDfd->read();
        else
            avgDfd->fillPreliminary(firstCurve->channel->type());

        MainWindow::setSetting("lastMovingAvgFile", avgDfdFile);
    }
    else {
        avgDfd = firstCurve->descriptor;
        avgDfdFile = avgDfd->fileName();
        descriptorFound = true;
    }


    avgDfd->calculateMovingAvg(channels,windowSize);

    if (writeToSeparateFile)
        avgDfd->fillRest();

    avgDfd->setChanged(true);
    avgDfd->setDataChanged(true);
    avgDfd->write();
    avgDfd->writeRawFile();

    if (descriptorFound) {
        updateFile(avgDfd);
    }
    else {
        addFile(avgDfd);
    }
    setCurrentAndPlot(avgDfd, avgDfd->channelsCount()-1);
}

void MainWindow::headerToggled(int column, Qt::CheckState state)
{DD;
    if (!tab || column<0 || column >= tab->channelsTable->columnCount()) return;

    if (state == Qt::PartiallyChecked) return;
    for (int i=0; i<tab->channelsTable->rowCount(); ++i)
        tab->channelsTable->item(i,column)->setCheckState(state);
}

void MainWindow::clearPlot()
{DD;
    plot->deleteGraphs();
    for (int index = 0; index<tabWidget->count(); ++index) {
        Tab *t = qobject_cast<Tab*>(tabWidget->widget(index));
        if (t) {
            QFont boldFont = t->tree->font();
            boldFont.setBold(true);
            for (int i=0; i<t->tree->topLevelItemCount(); ++i) {
                if (t->tree->topLevelItem(i)->font(1) == boldFont) {
                    FileDescriptor *dfd = dynamic_cast<SortableTreeWidgetItem *>(t->tree->topLevelItem(i))->fileDescriptor;
                    for (int i=0; i<dfd->channelsCount(); ++i) {
                        Channel *c = dfd->channel(i);
                        c->setCheckState(Qt::Unchecked);
                        c->setColor(QColor());
                    }
                    t->tree->topLevelItem(i)->setFont(1, t->tree->font());
                }
            }
            t->channelsTable->blockSignals(true);
            for (int i=0; i<t->channelsTable->rowCount(); ++i) {
                t->channelsTable->item(i,0)->setCheckState(Qt::Unchecked);
                t->channelsTable->item(i,0)->setFont(t->channelsTable->font());
                t->channelsTable->item(i,0)->setBackgroundColor(Qt::white);
                t->channelsTable->item(i,0)->setTextColor(Qt::black);
            }
            t->tableHeader->setCheckState(0,Qt::Unchecked);
            t->channelsTable->blockSignals(false);
        }
    }
}

void MainWindow::treeItemChanged(QTreeWidgetItem *item, int column)
{DD;
    if (column==9) {//легенда
        SortableTreeWidgetItem *i = dynamic_cast<SortableTreeWidgetItem *>(item);
        if (i) {
            if (!i->fileDescriptor->fileExists()) {
                QMessageBox::warning(this,"Не могу изменить легенду","Такого файла уже нет");
                return;
            }
            i->fileDescriptor->setLegend(item->text(column));
            //i->fileDescriptor->setChanged(true);
            //i->fileDescriptor->write();
            plot->updateLegends();
        }
    }

    if (column==6) {//шаг по оси х
        //if (QMessageBox::question(this, "Изменение шага", "Изменение ")
        SortableTreeWidgetItem *i = dynamic_cast<SortableTreeWidgetItem *>(item);
        if (i) {
            if (!i->fileDescriptor->fileExists()) {
                QMessageBox::warning(this,"Не могу изменить шаг по оси Х","Такого файла уже нет");
                return;
            }
            i->fileDescriptor->setXStep(item->text(column).toDouble());
            plot->update();
        }
    }

    if (column==2) {// описание
        SortableTreeWidgetItem *i = dynamic_cast<SortableTreeWidgetItem *>(item);
        if (i) {
            QDateTime dt = QDateTime::fromString(i->text(2), "dd.MM.yy hh:mm:ss");
            if (dt.isValid()) {
                i->fileDescriptor->setDateTime(dt);
            }
        }
    }
}

void MainWindow::rescanBase()
{DD;
    if (!tab) return;

    // first we delete all graphs affected
    for (int i=0; i<tab->tree->topLevelItemCount(); ++i) {
        SortableTreeWidgetItem *item = dynamic_cast<SortableTreeWidgetItem *>(tab->tree->topLevelItem(i));
        if (item) {
            plot->deleteGraphs(item->fileDescriptor/*->DFDGUID*/);
        }
    }

    // next we clear all tab and populate it with folders anew
    tab->tree->clear();
    tab->channelsTable->setRowCount(0);
    tab->channelsTable->blockSignals(true);
    tab->tableHeader->setCheckState(0,Qt::Unchecked);
    tab->channelsTable->blockSignals(false);

    tab->filePathLabel->clear();

    QStringList folders = tab->folders;
    foreach (QString folder, folders) {
        if (folder.endsWith(":0")) {
            folder.chop(2);
            addFolder(folder, false, false);
        }
        else if (folder.endsWith(":1")) {
            folder.chop(2);
            addFolder(folder, true, false);
        }
        else addFolder(folder, true, false);
    }

    //QMessageBox::information(this, "База данных", "В базе данных все записи \"живые\"!");
}

QVariant MainWindow::getSetting(const QString &key, const QVariant &defValue)
{DD;
    QSettings se("Alex Novichkov","DeepSea Database");
    return se.value(key, defValue);
}

void MainWindow::setSetting(const QString &key, const QVariant &value)
{DD;
    QSettings se("Alex Novichkov","DeepSea Database");
    se.setValue(key, value);
}


void MainWindow::addFile(FileDescriptor *descriptor)
{
    if (!tab) return;
    if (!descriptor) return;

    int count = tab->tree->topLevelItemCount();
    addFiles(QList<FileDescriptor *>()<<descriptor);
    tab->tree->setCurrentItem(tab->tree->topLevelItem(count));
}

bool MainWindow::findDescriptor(FileDescriptor *d)
{
    if (!d) return false;

    if (tab) {
        for (int j=0; j<tab->tree->topLevelItemCount(); ++j) {
            SortableTreeWidgetItem *item = dynamic_cast<SortableTreeWidgetItem *>(tab->tree->topLevelItem(j));
            if (item->fileDescriptor == d) {
                return true;
            }
        }
    }
    for (int i=0; i<tabWidget->count(); ++i) {
        Tab *t = qobject_cast<Tab *>(tabWidget->widget(i));
        if (t==tab) continue;

        for (int j=0; j<t->tree->topLevelItemCount(); ++j) {
            SortableTreeWidgetItem *item = dynamic_cast<SortableTreeWidgetItem *>(t->tree->topLevelItem(j));
            if (item->fileDescriptor == d) {
                return true;
            }
        }
    }
    return false;
}

FileDescriptor *MainWindow::findDescriptor(const QString &file)
{DD;
    if (tab) {
        for (int j=0; j<tab->tree->topLevelItemCount(); ++j) {
            SortableTreeWidgetItem *item = dynamic_cast<SortableTreeWidgetItem *>(tab->tree->topLevelItem(j));
            if (item->fileDescriptor->fileName() == file) {
                return item->fileDescriptor;
            }
        }
    }
    for (int i=0; i<tabWidget->count(); ++i) {
        Tab *t = qobject_cast<Tab *>(tabWidget->widget(i));
        if (t==tab) continue;

        for (int j=0; j<t->tree->topLevelItemCount(); ++j) {
            SortableTreeWidgetItem *item = dynamic_cast<SortableTreeWidgetItem *>(t->tree->topLevelItem(j));
            if (item->fileDescriptor->fileName() == file) {
                return item->fileDescriptor;
            }
        }
    }
    return 0;
}

void MainWindow::updateFile(FileDescriptor *descriptor)
{DD;
    for (int i=0; i<tabWidget->count(); ++i) {
        Tab *t = qobject_cast<Tab *>(tabWidget->widget(i));
        for (int j=0; j<t->tree->topLevelItemCount(); ++j) {
            SortableTreeWidgetItem *item = dynamic_cast<SortableTreeWidgetItem *>(t->tree->topLevelItem(j));
            if (item->fileDescriptor->fileName() == descriptor->fileName()) {
                item->setText(2, descriptor->dateTime());
                item->setText(7, QString::number(descriptor->channelsCount()));
            }

        }
    }
}

void MainWindow::setCurrentAndPlot(FileDescriptor *descriptor, int index)
{DD;
    if (tab) {
        int i = -1;
        for (int j=0; j<tab->tree->topLevelItemCount(); ++j) {
            SortableTreeWidgetItem *item = dynamic_cast<SortableTreeWidgetItem *>(tab->tree->topLevelItem(j));
            if (item->fileDescriptor == descriptor) {
                i = j;
            }
        }
        if (i > -1) {
            tab->tree->setCurrentItem(0);
            tab->tree->setCurrentItem(tab->tree->topLevelItem(i));
            tab->channelsTable->item(index, 0)->setCheckState(Qt::Checked);
        }
    }
}

void setLineColor(QAxObject *obj, int color)
{DD;
    QAxObject *format = obj->querySubObject("Format");
    QAxObject *formatLine = format->querySubObject("Line");
    formatLine->setProperty("Visible",true);
    QAxObject *formatLineForeColor = formatLine->querySubObject("ForeColor");
    formatLineForeColor->setProperty("ObjectThemeColor", color);

    delete formatLineForeColor;
    delete formatLine;
    delete format;
}

void setAxis(QAxObject *xAxis, const QString &title)
{DD;
    xAxis->dynamicCall("SetHasTitle(bool)",true);
    QAxObject *axisTitle = xAxis->querySubObject("AxisTitle()");
    axisTitle->setProperty("Text", title);
    xAxis->setProperty("MinorTickMark", /*xlOutside*/3);
    xAxis->dynamicCall("SetHasMajorGridlines(bool)", true);
    QAxObject *xGridLines = xAxis->querySubObject("MajorGridlines()");
    QAxObject *xGridFormat = xGridLines->querySubObject("Format");
    QAxObject *xGridFormatLine = xGridFormat->querySubObject("Line");
    xGridFormatLine->setProperty("Visible",true);
    xGridFormatLine->setProperty("DashStyle",7);

    QAxObject *format = xAxis->querySubObject("Format");
    QAxObject *formatLine = format->querySubObject("Line");
    formatLine->setProperty("Visible",true);
    QAxObject *formatLineForeColor = formatLine->querySubObject("ForeColor");
    formatLineForeColor->setProperty("ObjectThemeColor", 13);

    delete formatLineForeColor;
    delete formatLine;
    delete format;
    delete xGridFormatLine;
    delete xGridFormat;
    delete xGridLines;
    delete axisTitle;
}

void MainWindow::exportToExcel()
{DD;
    exportToExcel(false);
}

void MainWindow::exportToExcelFull()
{DD;
    exportToExcel(true);
}

void MainWindow::exportToExcelData()
{DD;
    exportToExcel(false, true);
}

QStringList twoStringDescription(const DescriptionList &list)
{
    QStringList result;
    if (list.size()>0) result << descriptionEntryToString(list.first()); else result << "";
    if (list.size()>1) result << descriptionEntryToString(list.at(1)); else result << "";
    return result;
}

void MainWindow::exportToExcel(bool fullRange, bool dataOnly)
{
    static QAxObject *excel = 0;

    if (!plot->hasGraphs()) {
        QMessageBox::warning(this, "Графиков нет", "Постройте хотя бы один график!");
        return;
    }

    if (!excel) {
        //excel = new QAxObject("Excel.Application",this);
        excel = new QAxObject("{00024500-0000-0000-c000-000000000046}&",this);
    }
    if (!excel) return;
    //qDebug()<<excel->generateDocumentation();

    excel->setProperty("Visible", true);

    //получаем рабочую книгу
    QAxObject * workbooks = excel->querySubObject("WorkBooks");
    QAxObject * workbook = excel->querySubObject("ActiveWorkBook");
    if (!workbook) {
        workbooks->dynamicCall("Add");
    }
    workbook = excel->querySubObject("ActiveWorkBook");

    // получаем список листов и добавляем новый лист
    QAxObject *worksheets = workbook->querySubObject("Sheets");
    worksheets->dynamicCall("Add()");

    // переименовываем новый лист согласно выбранным записям и каналам
    //QString newSheetName = QDateTime::currentDateTime().toString();
    QAxObject * worksheet = workbook->querySubObject("ActiveSheet");
    //worksheet->setProperty("Name", newSheetName);

    // экспортируем данные графиков на лист
     QList<Curve *> curves = plot->curves();

     // проверяем, все ли каналы из одного файла
     FileDescriptor *descriptor = curves.at(0)->descriptor;
     Channel *channel = curves.at(0)->channel;
     bool allChannelsFromOneFile = true;
     for (int i=1; i<curves.size(); ++i) {
         if (curves.at(i)->descriptor->fileName() != descriptor->fileName()) {
             allChannelsFromOneFile = false;
             break;
         }
     }


     //проверяем, все ли каналы имеют одинаковое разрешение по х
     bool allChannelsHaveSameXStep = true;
     if (channel->xStep()==0.0) allChannelsHaveSameXStep = false;
     for (int i=1; i<curves.size(); ++i) {
         if (curves.at(i)->channel->xStep() == 0.0) {
             allChannelsHaveSameXStep = false;
             break;
         }
         if (qAbs(curves.at(i)->channel->xStep() - channel->xStep()) >= 1e-10) {
             allChannelsHaveSameXStep = false;
             break;
         }
     }

     // ищем максимальное количество отсчетов
     // и максимальный шаг
     quint32 maxInd = channel->samplesCount();
     double maxStep = channel->xStep();
     bool zeroStepDetected = maxStep<1e-9;
     double minX = channel->xBegin();
     double maxX = channel->xMaxInitial();

     Range range = plot->xRange();

     for (int i=1; i<curves.size(); ++i) {
         Channel *ch = curves.at(i)->channel;
         if (ch->samplesCount() > maxInd)
             maxInd = ch->samplesCount();
         if (ch->xStep() > maxStep)
             maxStep = ch->xStep();
         zeroStepDetected |= (ch->xStep() < 1e-9);
         if (ch->xBegin()< minX)
             minX = ch->xBegin();
         if (ch->xMaxInitial() > maxX)
             maxX = ch->xMaxInitial();
     }
     if (minX >= range.min && maxX <= range.max) fullRange = true;

     //if (zeroStepDetected) allChannelsHaveSameXStep = false;

     bool exportPlots = true;
     if (maxInd > 32000 && fullRange && !dataOnly) {
         QMessageBox::warning(this, "Слишком много данных",
                              "В одном или всех каналах число отсчетов превышает 32000.\n"
                              "Будут экспортированы только данные, но не графики");
         exportPlots = false;
     }
     if (dataOnly) exportPlots = false;

     // записываем название файла
     if (allChannelsFromOneFile) {
         QAxObject *cells = worksheet->querySubObject("Cells(Int,Int)", 1, 1);
         if (cells) cells->setProperty("Value", descriptor->fileName());
         delete cells;
     }
     else {
         for (int i=0; i<curves.size(); ++i) {
             Curve *curve = curves.at(i);
             QAxObject *cells = allChannelsHaveSameXStep ? worksheet->querySubObject("Cells(Int,Int)", 1, 2+i)
                                                         : worksheet->querySubObject("Cells(Int,Int)", 1, 2+i*2);
             cells->setProperty("Value", curve->descriptor->fileName());
             delete cells;
         }
     }

     //записываем описатели

     if (allChannelsFromOneFile) {
         QStringList descriptions = twoStringDescription(descriptor->dataDescriptor());
         QAxObject *cells = worksheet->querySubObject("Cells(Int,Int)", 1, 1);
         if (cells) cells->setProperty("Value", descriptor->fileName());

         cells = worksheet->querySubObject("Cells(Int,Int)", 2, 2);
         if (cells) cells->setProperty("Value", descriptions.first());

         cells = worksheet->querySubObject("Cells(Int,Int)", 3, 2);
         if (cells) cells->setProperty("Value", descriptions.at(1));

         delete cells;
     }
     else {
         for (int i=0; i<curves.size(); ++i) {
             Curve *curve = curves.at(i);

             QStringList descriptions = twoStringDescription(curve->descriptor->dataDescriptor());

             QAxObject *cells = allChannelsHaveSameXStep ? worksheet->querySubObject("Cells(Int,Int)", 1, 2+i)
                                                         : worksheet->querySubObject("Cells(Int,Int)", 1, 2+i*2);
             cells->setProperty("Value", curve->descriptor->fileName());

             cells = allChannelsHaveSameXStep ? worksheet->querySubObject("Cells(Int,Int)", 2, 2+i)
                                              : worksheet->querySubObject("Cells(Int,Int)", 2, 2+i*2);
             if (cells) cells->setProperty("Value", descriptions.first());

             cells = allChannelsHaveSameXStep ? worksheet->querySubObject("Cells(Int,Int)", 3, 2+i)
                                              : worksheet->querySubObject("Cells(Int,Int)", 3, 2+i*2);
             if (cells) cells->setProperty("Value", descriptions.at(1));

             delete cells;
         }
     }

     // записываем название канала
     for (int i=0; i<curves.size(); ++i) {
         Curve *curve = curves.at(i);
         QAxObject *cells = allChannelsHaveSameXStep ? worksheet->querySubObject("Cells(Int,Int)", 4, 2+i)
                                                     : worksheet->querySubObject("Cells(Int,Int)", 4, 2+i*2);
         cells->setProperty("Value", curve->title().text());
         delete cells;
     }

     QVector<int> selectedSamples;

     // если все каналы имеют одинаковый шаг по х, то в первый столбец записываем
     // данные х
     // если каналы имеют разный шаг по х, то для каждого канала отдельно записываем
     // по два столбца
     if (allChannelsHaveSameXStep) {
         const quint32 numCols = curves.size();

         QList<QVariant> cellsList;
         QList<QVariant> rowsList;
         for (uint i = 0; i < maxInd; ++i) {
             double val = channel->xBegin() + i*channel->xStep();
             if (/*channel->xStep()<1e-9 &&*/ !channel->xValues().isEmpty())
                 val = channel->xValues().at(i);
             if (!fullRange && (val < range.min || val > range.max) ) continue;

             cellsList.clear();
             cellsList << val;
             for (uint j = 0; j < numCols; ++j) {
                 cellsList << ((curves.at(j)->channel->samplesCount() < maxInd) ? 0 : curves.at(j)->channel->yValues()[i]);
             }
             rowsList << QVariant(cellsList);
         }
         quint32 numRows = rowsList.size();

         QAxObject* Cell1 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 5, 1);
         QAxObject* Cell2 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 5 + numRows - 1, 1 + numCols);
         QAxObject* range = worksheet->querySubObject("Range(const QVariant&,const QVariant&)", Cell1->asVariant(), Cell2->asVariant() );

         range->setProperty("Value", QVariant(rowsList) );

         // выделяем диапазон, чтобы он автоматически использовался для построения диаграммы
         Cell1 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 4, 1);
         Cell2 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 4 + numRows, 1 + numCols);
         range = worksheet->querySubObject("Range(const QVariant&,const QVariant&)", Cell1->asVariant(), Cell2->asVariant() );
         range->dynamicCall("Select (void)");

         delete range;
         delete Cell1;
         delete Cell2;
     }
     else {
         for (int i=0; i<curves.size(); ++i) {
             Curve *curve = curves.at(i);
             Channel *ch = curve->channel;

             QList<QVariant> cellsList;
             QList<QVariant> rowsList;
             for (uint j = 0; j < ch->samplesCount(); j++) {
                 double val = ch->xBegin() + j*ch->xStep();
                 if (/*ch->xStep()<1e-9 &&*/ !ch->xValues().isEmpty())
                     val = ch->xValues().at(j);
                 if (!fullRange && (val < range.min || val > range.max) ) continue;

                 cellsList.clear();
                 cellsList << val << ch->yValues()[j];
                 rowsList << QVariant(cellsList);
             }
             quint32 numRows = rowsList.size();
             selectedSamples << numRows;

             QAxObject* Cell1 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 5, 1+i*2);
             QAxObject* Cell2 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 5 + numRows-1, 2 + i*2);
             QAxObject* range = worksheet->querySubObject("Range(const QVariant&,const QVariant&)", Cell1->asVariant(), Cell2->asVariant() );

             range->setProperty("Value", QVariant(rowsList) );

             delete Cell1;
             delete Cell2;
             delete range;
         }
     }

     if (exportPlots) {

         QAxObject *charts = workbook->querySubObject("Charts");
         charts->dynamicCall("Add()");
         QAxObject *chart = workbook->querySubObject("ActiveChart");
         //     chart->dynamicCall("Select()");
//              chart->setProperty("Name", "Диагр."+newSheetName);
         chart->setProperty("ChartType", 75);

         QAxObject * series = chart->querySubObject("SeriesCollection");


         // отдельно строить кривые нужно, только если у нас много пар столбцов с данными
         if (!allChannelsHaveSameXStep) {
             int seriesCount = series->property("Count").toInt();
             // удаляем предыдущие графики, созданные по умолчанию
             bool ok=true;
             while ((seriesCount>0) && ok) {
                 QAxObject * serie = series->querySubObject("Item (int)", 1);
                 if (serie) {
                     serie->dynamicCall("Delete()");
                     delete serie;
                     seriesCount--;
                 }
                 else ok=false;
             }

             for (int i=0; i<curves.size(); ++i) {
                 Curve *curve = curves.at(i);
                 QAxObject * serie;

                 serie = series->querySubObject("NewSeries()");
                 if (serie) {
                     QAxObject* Cell1 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 5, 1+i*2);
                     QAxObject* Cell2 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 5 + selectedSamples.at(i)-1, 1 + i*2);
                     if (Cell1 && Cell2) {
                         QAxObject * xvalues = worksheet->querySubObject("Range(const QVariant&,const QVariant&)", Cell1->asVariant(), Cell2->asVariant());
                         if (xvalues)
                             serie->setProperty("XValues", xvalues->asVariant());
                         delete xvalues;
                         delete Cell1;
                         delete Cell2;
                     }

                     Cell1 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 5, 2+i*2);
                     Cell2 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 5 + selectedSamples.at(i)-1, 2 + i*2);
                     if (Cell1 && Cell2) {
                         QAxObject * yvalues = worksheet->querySubObject("Range(const QVariant&,const QVariant&)", Cell1->asVariant(), Cell2->asVariant());
                         if (yvalues)
                             serie->setProperty("Values", yvalues->asVariant());
                         delete yvalues;
                         delete Cell1;
                         delete Cell2;
                     }

                     QStringList fullName;// = twoStringDescription(curve->descriptor->dataDescriptor());
                     fullName.append(curve->channel->name());
                     serie->setProperty("Name", fullName.join(" "));
                     delete serie;
                 }
             }
         }
         else {
             // добавляем к названиям графиков информацию
             int seriesCount = series->property("Count").toInt();
             for ( int i=0; i<seriesCount; ++i) {
                 QAxObject * serie = series->querySubObject("Item (int)", i+1);
                 Curve *curve = curves.at(i);
                 if (serie) {
                     QStringList fullName;// = twoStringDescription(curve->descriptor->dataDescriptor());
                     fullName.append(curve->channel->name());
                     serie->setProperty("Name", fullName.join(" "));
                     delete serie;
                 }
             }
         }

         // перемещаем графики на дополнительную вертикальную ось,
         // если они были там в программе
         int seriesCount = series->property("Count").toInt();
         for ( int i=0; i<seriesCount; ++i) {
             Curve *curve = curves.at(i);
             if (curve->yAxis()==QwtPlot::yRight) {
                 QAxObject * serie = series->querySubObject("Item (int)", i+1);
                 if (serie) {
                     serie->setProperty("AxisGroup", 2);
                     delete serie;
                 }
                 QAxObject *yAxis = chart->querySubObject("Axes(const QVariant&,const QVariant&)", 2,2);
                 if (yAxis) setAxis(yAxis, plot->axisTitle(QwtPlot::yRight).text());
                 delete yAxis;
             }
         }



         // добавляем подписи осей
         QAxObject *xAxis = chart->querySubObject("Axes(const QVariant&)", 1);
         if (xAxis) {
             setAxis(xAxis, "Частота, Гц");
             xAxis->setProperty("MaximumScale", range.max);
             xAxis->setProperty("MinimumScale", int(range.min/10)*10);
         }
         delete xAxis;

         QAxObject *yAxis = chart->querySubObject("Axes(const QVariant&)", 2);
         if (yAxis) {
             setAxis(yAxis, "Уровень, дБ");
             yAxis->setProperty("CrossesAt", -1000);

         }
         delete yAxis;

         // рамка вокруг графика
         QAxObject *plotArea = chart->querySubObject("PlotArea");
         if (plotArea) setLineColor(plotArea, 13);
         delete plotArea;

         // цвета графиков
         for (int i = 0; i< curves.size(); ++i) {
             Curve *curve = curves.at(i);
             QAxObject * serie = series->querySubObject("Item(int)", i+1);
             if (serie) {
                 QAxObject *format = serie->querySubObject("Format");
                 QAxObject *formatLine = format->querySubObject("Line");
                 if (formatLine) formatLine->setProperty("Weight", 1);

                 QAxObject *formatLineForeColor = formatLine->querySubObject("ForeColor");
                 if (formatLineForeColor) formatLineForeColor->setProperty("RGB", curves.at(i)->pen().color().rgb());

                 foreach(PointLabel *label, curve->labels) {
                     QAxObject* point = serie->querySubObject("Points(QVariant)", label->point()+1);
                     QVariantList options = {0, 0, 0, 0, 0, -1, 0, 0, 0, 0};

                     point->dynamicCall("ApplyDataLabels()", options);
                     QAxObject* dataLabel = point->querySubObject("DataLabel");
                     dataLabel->setProperty("ShowCategoryName", -1);
                     dataLabel->setProperty("ShowValue", 0);
                     dataLabel->setProperty("Position", 0);
                     delete dataLabel;
                     delete point;
                 }

                 delete formatLineForeColor;
                 delete formatLine;
                 delete format;
             }
             delete serie;
         }

         QAxObject *chartArea = chart->querySubObject("ChartArea");
         chartArea->querySubObject("Format")->querySubObject("Line")->setProperty("Visible", 0);
         delete chartArea;

         QAxObject *legendObject = chart->querySubObject("Legend");
         QAxObject *legendFormat = legendObject->querySubObject("Format");
         QAxObject *legendFormatFill = legendFormat->querySubObject("Fill");

         legendFormatFill->setProperty("Visible", 1);
         legendFormatFill->querySubObject("ForeColor")->setProperty("RGB", QColor(Qt::white).rgb());
         legendFormat->querySubObject("Line")->setProperty("Visible", 1);
         legendFormat->querySubObject("Line")->querySubObject("ForeColor")->setProperty("RGB", QColor(Qt::black).rgb());

         delete legendFormatFill;
         delete legendFormat;
         delete legendObject;
         delete series;
         delete chart;
         delete charts;
     }

//    QFile file1("chartArea.html");
//    file1.open(QIODevice::WriteOnly | QIODevice::Text);
//    QTextStream out(&file1);
//    out << chartArea->generateDocumentation();
//    file1.close();


     delete worksheet;
     delete worksheets;
     delete workbook;
     delete workbooks;
}

void MainWindow::onCurveColorChanged(Curve *curve)
{DD;
    if (tab->record == curve->descriptor) {
        updateChannelsTable(curve->descriptor);
    }
}

void MainWindow::onCurveDeleted(FileDescriptor *descriptor, int channelIndex)
{DD;
    for (int index = 0; index < tabWidget->count(); ++index) {
        Tab *t = qobject_cast<Tab*>(tabWidget->widget(index));
        if (t) {
            QFont boldFont = t->tree->font();
            boldFont.setBold(true);
            for (int i=0; i<t->tree->topLevelItemCount(); ++i) {
                FileDescriptor *dfd = dynamic_cast<SortableTreeWidgetItem *>(t->tree->topLevelItem(i))->fileDescriptor;
                if (dfd == descriptor) {
                   // QList<Channel *> list = dfd->channels;
                    for (int channel=0; channel<dfd->channelsCount(); ++channel) {
                        if (channelIndex == channel) {
                            dfd->channel(channel)->setCheckState(Qt::Unchecked);
                            dfd->channel(channel)->setColor(QColor());
                        }
                    }
                    bool unchecked = true;
                    for (int channel=0; channel<dfd->channelsCount(); ++channel) {
                        if (dfd->channel(channel)->checkState() == Qt::Checked) {
                            unchecked = false;
                            break;
                        }
                    }

                    if (unchecked) {
                        t->tree->topLevelItem(i)->setFont(1, t->tree->font());
                    }
                }
            }
        }
    }
    if (tab->record == descriptor)
        updateChannelsTable(descriptor);
}

void MainWindow::addFiles(const QList<FileDescriptor*> &files)
{DD;
    if (!tab) return;

    QList<QTreeWidgetItem *> items;

    int pos = tab->tree->topLevelItemCount();

    foreach (FileDescriptor *dfd, files) {
        QStringList info = dfd->info();
        info.prepend(QString::number(++pos));

        QTreeWidgetItem *item = new SortableTreeWidgetItem(dfd, info);
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
        items << item;
    }
    tab->tree->setSortingEnabled(false);
    tab->tree->addTopLevelItems(items);
//    tab->tree->sortItems(0,Qt::AscendingOrder);
    tab->tree->setSortingEnabled(true);
}

void MainWindow::addFiles(QStringList &files)
{DD;
    if (!tab) return;

    QList<FileDescriptor *> items;

    for (int i=files.size()-1; i>=0; --i) {
        QString file = files[i];

        if (checkForContains(tab, file)) {//этот файл уже есть во вкладке
            files.removeAt(i);
        }
        else {
            FileDescriptor *dfd = findDescriptor(file);
            if (!dfd) {
                dfd = createDescriptor(file);
                dfd->read();
            }
            if (dfd) {
                items.prepend(dfd);
            }
        }
    }
    addFiles(items);
}

void MainWindow::deleteFiles(const QVector<int> &indexes)
{DD;
    if (!tab) return;
    bool taken = false;

    for (int i=indexes.size()-1; i>=0; --i) {
        SortableTreeWidgetItem *item = dynamic_cast<SortableTreeWidgetItem *>(tab->tree->topLevelItem(indexes.at(i)));
        FileDescriptor *d = item->fileDescriptor;
        plot->deleteGraphs(d);

        if (tab->folders.contains(d->fileName()))
            tab->folders.removeOne(d->fileName());
        else if (tab->folders.contains(d->fileName()+":0"))
            tab->folders.removeOne(d->fileName()+":0");
        else if (tab->folders.contains(d->fileName()+":1"))
            tab->folders.removeOne(d->fileName()+":1");

        delete tab->tree->takeTopLevelItem(indexes.at(i));
        if (!findDescriptor(d)) delete d;
        taken = true;
    }
    if (taken) {
        tab->channelsTable->setRowCount(0);
    }
}
