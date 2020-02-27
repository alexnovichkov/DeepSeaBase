#include "mainwindow.h"

#include <QtWidgets>

#include "calculatespectredialog.h"
#include "filesprocessordialog.h"
#include "sortabletreewidgetitem.h"
#include "checkableheaderview.h"
#include "plot.h"
#include "tabwidget.h"
#include "colorselector.h"
#include "coloreditdialog.h"
#include "correctiondialog.h"
#include "curve.h"
#include "pointlabel.h"
#include "model.h"
#include "sortfiltermodel.h"
#include "filterheaderview.h"

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
#include "timeslicer.h"
#include <QTime>

#define DSB_VERSION "1.6.9.1"

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
        this->xStep = dfd->xStep();
        if (DfdFileDescriptor *d = dynamic_cast<DfdFileDescriptor *>(dfd))
            this->dataType = d->DataType;
        else
            this->dataType = dfdDataTypeFromDataType(dfd->type());
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
                //частный случай: мы можем записать данные из SourceData в CuttedData, преобразовав их в floats,
                //но не наоборот
                if (dfd.xStep() == xStep) {
                    if (dfd.DataType == CuttedData && (dataType == FilterData ||
                                                       dataType == SourceData))
                        return true;
                    else if (dfd.DataType == dataType) {
                        return true;
                    }
                }
                else {
                    return false;
                }
            }
            else if (fi.suffix().toLower()=="uff" || fi.suffix().toLower()=="unv") {
                return true;
            }
            else return false;
        }

        return true;
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

class StepItemDelegate : public QStyledItemDelegate
{
public:
    StepItemDelegate(QObject *parent = Q_NULLPTR) : QStyledItemDelegate(parent)
    {}

    // QAbstractItemDelegate interface
public:
    virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

QWidget *StepItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QWidget *ed = QStyledItemDelegate::createEditor(parent, option, index);
    if (QDoubleSpinBox *spin = qobject_cast<QDoubleSpinBox*>(ed)) {
        spin->setDecimals(10);
    }

    return ed;
}


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
    connect(plot, SIGNAL(saveTimeSegment(QList<FileDescriptor*>,double,double)),this, SLOT(saveTimeSegment(QList<FileDescriptor*>,double,double)));

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

    switchSergeiModeAct = new QAction("Режим Сергея", this);
    switchSergeiModeAct->setIcon(QIcon(":/icons/sergei.png"));
    switchSergeiModeAct->setCheckable(true);
    connect(switchSergeiModeAct, SIGNAL(triggered()), SLOT(switchSergeiMode()));


    delFilesAct = new QAction(QString("Удалить записи"), this);
    delFilesAct->setShortcut(Qt::Key_Delete);
    delFilesAct->setShortcutContext(Qt::WidgetShortcut);
    connect(delFilesAct, SIGNAL(triggered()), SLOT(deleteFiles()));

    plotAllChannelsAct = new QAction(QString("Построить все каналы"), this);
    connect(plotAllChannelsAct, SIGNAL(triggered()), SLOT(plotAllChannels()));

    plotAllChannelsAtRightAct = new QAction(QString("...на правой оси"), this);
    connect(plotAllChannelsAtRightAct, SIGNAL(triggered()), SLOT(plotAllChannelsAtRight()));


    QAction *plotHelpAct = new QAction("Справка", this);
    connect(plotHelpAct, &QAction::triggered, [](){QDesktopServices::openUrl(QUrl("help.html"));});

    calculateSpectreAct = new QAction(QString("Обработать записи..."), this);
    connect(calculateSpectreAct, SIGNAL(triggered()), SLOT(calculateSpectreRecords()));

    convertAct = new QAction("Конвертировать файлы...", this);
    connect(convertAct, SIGNAL(triggered()), SLOT(convertFiles()));

    copyToLegendAct = new QAction("Перенести сюда названия файлов", this);
    connect(copyToLegendAct, SIGNAL(triggered()), SLOT(copyToLegend()));

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
    connect(trackingCursorAct, &QAction::triggered, [this](){
        plot->switchTrackingCursor();
    });
    connect(plot,SIGNAL(trackingPanelCloseRequested()), trackingCursorAct, SLOT(toggle()));

    playAct  = new QAction("Открыть панель плеера", this);
    playAct->setIcon(QIcon(":/icons/play.png"));
    playAct->setCheckable(true);
    connect(playAct, &QAction::triggered, [this](){
        plot->switchPlayerVisibility();
    });
    connect(plot,SIGNAL(playerPanelCloseRequested()),playAct,SLOT(toggle()));

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
    connect(interactionModeAct, &QAction::triggered, [this](){
        plot->switchInteractionMode();
    });

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

    editYNameAct = new QAction("Изменить ед. измерения", this);
    connect(editYNameAct,SIGNAL(triggered()), SLOT(editYName()));

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
    mainToolBar->addAction(switchSergeiModeAct);
    mainToolBar->addAction(playAct);
    mainToolBar->addSeparator();
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

    autoscaleXAct = new QAction("Автомасштабирование по оси X", this);
    autoscaleXAct->setIcon(QIcon(":/icons/autoscale-x.png"));
    autoscaleXAct->setCheckable(true);
    bool autoscale = getSetting("autoscale-x", true).toBool();
    connect(autoscaleXAct, &QAction::toggled, [this](bool toggled){
        plot->toggleAutoscale(0 /* x axis */,toggled);
        setSetting("autoscale-x", toggled);
    });
    autoscaleXAct->setChecked(autoscale);
    plot->toggleAutoscale(0 /* x axis */, autoscale);

    autoscaleYAct = new QAction("Автомасштабирование по оси Y", this);
    autoscaleYAct->setIcon(QIcon(":/icons/autoscale-y-main.png"));
    autoscaleYAct->setCheckable(true);
    autoscale = getSetting("autoscale-y", true).toBool();
    connect(autoscaleYAct, &QAction::toggled, [this](bool toggled){
        plot->toggleAutoscale(1 /* y axis */,toggled);
        setSetting("autoscale-y", toggled);
    });
    autoscaleYAct->setChecked(autoscale);
    plot->toggleAutoscale(1 /* x axis */, autoscale);

    autoscaleYSlaveAct = new QAction("Автомасштабирование по правой оси Y", this);
    autoscaleYSlaveAct->setIcon(QIcon(":/icons/autoscale-y-slave.png"));
    autoscaleYSlaveAct->setCheckable(true);
    autoscale = getSetting("autoscale-y-slave", true).toBool();
    connect(autoscaleYSlaveAct, &QAction::toggled, [this](bool toggled){
        plot->toggleAutoscale(2 /* y slave axis */,toggled);
        setSetting("autoscale-y-slave", toggled);
    });
    autoscaleYSlaveAct->setChecked(autoscale);
    plot->toggleAutoscale(2 /* x axis */, autoscale);

    autoscaleAllAct  = new QAction("Автомасштабирование по всем осям", this);
    autoscaleAllAct->setIcon(QIcon(":/icons/autoscale-all.png"));
    connect(autoscaleAllAct, &QAction::triggered, [this](){
        plot->autoscale(0 /* x axis */);
        plot->autoscale(1 /* y axis */);
        plot->autoscale(2 /* y slave axis */);
    });

    removeLabelsAct  = new QAction("Удалить все подписи", this);
    removeLabelsAct->setIcon(QIcon(":/icons/remove-labels.png"));
    connect(removeLabelsAct, &QAction::triggered, [this](){
        plot->removeLabels();
    });

    QToolBar *scaleToolBar = new QToolBar(this);
    scaleToolBar->setOrientation(Qt::Vertical);
    scaleToolBar->addAction(autoscaleXAct);
    scaleToolBar->addAction(autoscaleYAct);
    scaleToolBar->addAction(autoscaleYSlaveAct);
    scaleToolBar->addAction(autoscaleAllAct);
    scaleToolBar->addSeparator();
    scaleToolBar->addAction(removeLabelsAct);

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

    tab->model = new Model(tab);
    tab->sortModel = new SortFilterModel(tab);
    tab->sortModel->setSourceModel(tab->model);

    tab->filesTable = new QTreeView(this);
    tab->filesTable->setModel(tab->sortModel);

    tab->filesTable->setRootIsDecorated(false);
    tab->filesTable->setSortingEnabled(true);
    tab->filesTable->sortByColumn(0, Qt::AscendingOrder);

    tab->filesTable->setContextMenuPolicy(Qt::CustomContextMenu);
    tab->filesTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    tab->filesTable->addAction(delFilesAct);
    tab->filesTable->addAction(saveAct);

    connect(tab->filesTable, &QTreeView::customContextMenuRequested, [=](){
        QMenu menu(tab->filesTable);
        int column = tab->filesTable->currentIndex().column();
        if (column == 1) {
            menu.addAction(addFolderAct);
            menu.addAction(addFileAct);
            menu.addAction(delFilesAct);
            menu.addAction(plotAllChannelsAct);
            menu.addAction(plotAllChannelsAtRightAct);
            menu.addAction(calculateSpectreAct);
            menu.addAction(convertAct);
            menu.exec(QCursor::pos());
        }
        else if (column == 9) {
            //legend
            menu.addAction(copyToLegendAct);
            menu.exec(QCursor::pos());
        }
    });


    FilterHeaderView *filterHeader = new FilterHeaderView(Qt::Horizontal, tab->filesTable);
    tab->filesTable->setHeader(filterHeader);
    connect(filterHeader, SIGNAL(filterChanged(QString,int)), tab->sortModel, SLOT(setFilter(QString,int)));
    filterHeader->setFilterBoxes(tab->model->columnCount());

    tab->filesTable->header()->setStretchLastSection(false);
    tab->filesTable->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    tab->filesTable->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    tab->filesTable->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    tab->filesTable->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    tab->filesTable->header()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    tab->filesTable->header()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    tab->filesTable->header()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    tab->filesTable->header()->setSectionResizeMode(7, QHeaderView::ResizeToContents);


    //QByteArray treeHeaderState = getSetting("treeHeaderState").toByteArray();
    //if (!treeHeaderState.isEmpty())
    //    tab->filesTable->header()->restoreState(treeHeaderState);

    connect(tab->filesTable->selectionModel(),SIGNAL(selectionChanged(QItemSelection,QItemSelection)),tab, SLOT(filesSelectionChanged(QItemSelection,QItemSelection)));
    connect(tab->filesTable->selectionModel(),SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),SLOT(updateChannelsTable(QModelIndex,QModelIndex)));

    tab->filesTable->setItemDelegateForColumn(6 /*шаг по оси х*/, new StepItemDelegate);

    connect(tab->model, SIGNAL(legendsChanged()), plot, SLOT(updateLegends()));
    connect(tab->model, SIGNAL(plotNeedsUpdate()), plot, SLOT(update()));

    tab->channelsTable = new QTableWidget(0,7,this);

    tab->tableHeader = new CheckableHeaderView(Qt::Horizontal, tab->channelsTable);

    tab->channelsTable->setHorizontalHeader(tab->tableHeader);
    tab->tableHeader->setCheckState(0,Qt::Checked);
    tab->tableHeader->setCheckable(0,true);
    tab->tableHeader->setCheckState(0,Qt::Unchecked);
    connect(tab->tableHeader,SIGNAL(toggled(int,Qt::CheckState)),this,SLOT(headerToggled(int,Qt::CheckState)));

    tab->channelsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    tab->channelsTable->horizontalHeader()->setStretchLastSection(false);
    connect(tab->channelsTable, SIGNAL(itemChanged(QTableWidgetItem*)), SLOT(maybePlotChannel(QTableWidgetItem*)));

    tab->channelsTable->addAction(moveChannelsUpAct);
    tab->channelsTable->addAction(moveChannelsDownAct);

    tab->channelsTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(tab->channelsTable, &QTableWidget::customContextMenuRequested, [=](){
        QMenu menu(tab->channelsTable);
        int column = tab->channelsTable->currentIndex().column();
        if (column == 1) {
            menu.addAction(editYNameAct);
            menu.exec(QCursor::pos());
        }
//        else if (column == 9) {
//            //legend
//            menu.addAction(copyToLegendAct);
//            menu.exec(QCursor::pos());
//        }
    });

    tab->filePathLabel = new QLabel(this);
    tab->filePathLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);




    QWidget *treeWidget = new QWidget(this);
    QVBoxLayout *treeLayout = new QVBoxLayout;
    treeLayout->addWidget(tab->filesTable);

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

    QToolButton *editFileButton = new QToolButton(this);
    QAction *editFileAct = new QAction("Редактировать этот файл в текстовом редакторе", this);
    editFileAct->setIcon(QIcon(":/icons/edit.png"));
    editFileButton->setDefaultAction(editFileAct);
    connect(editFileAct, &QAction::triggered, [=](){
        QString file = QDir::toNativeSeparators(tab->filePathLabel->text());
        if (!file.isEmpty()) {
            QString executable = getSetting("editor").toString();
            if (executable.isEmpty())
                executable = QInputDialog::getText(this, "Текстовый редактор не задан",
                                                   "Введите путь к текстовому редактору,\n"
                                                   "название исполняемого файла или команду для выполнения");

            if (!executable.isEmpty()) {
                if (!QProcess::startDetached(executable, QStringList()<<file)) {
                    executable = QStandardPaths::findExecutable(executable);
                    if (!executable.isEmpty())
                        if (QProcess::startDetached(executable, QStringList()<<file))
                            setSetting("editor", executable);
                }
                else
                    setSetting("editor", executable);
            }
        }
    });

    QWidget *channelsWidget = new QWidget(this);
    QGridLayout *channelsLayout = new QGridLayout;
    channelsLayout->addWidget(tab->filePathLabel,0,0);
    channelsLayout->addWidget(tab->channelsTable,1,0,1,3);
    channelsLayout->addWidget(editFileButton, 0,1);
    channelsLayout->addWidget(openFolderButton, 0,2);
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
    if (!tabWidget) return;
    if (tabWidget->count()==1) return;
    int index=i;
    if (i<0) index = tabWidget->currentIndex();

    tabWidget->setCurrentIndex(index);
    //tab now points to current tab

    if (!tab) return;

    // удаление графиков тех файлов, которые были в закрываемой вкладке
    for (int i=plot->graphsCount()-1; i>=0; --i) {
        FileDescriptor *f = plot->graphs[i]->descriptor;
        if (tab->model->contains(f) && !duplicated(f))
            plot->deleteGraph(plot->graphs[i]);
    }

    tab->filesTable->selectAll();

    // костыль, позволяющий иметь несколько одинаковых файлов в разных вкладках
    QStringList duplicatedFiles;
    foreach (FileDescriptor *f, tab->model->selectedFiles()) {
        if (duplicated(f)) duplicatedFiles << f->fileName();
    }


    tab->model->deleteFiles(duplicatedFiles);

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
    tab = 0;
    if (currentIndex<0) return;

    tab = qobject_cast<Tab *>(tabWidget->currentWidget());
}

void MainWindow::editColors()
{DD;
    ColorEditDialog dialog(this);
    dialog.exec();
}

MainWindow::~MainWindow()
{DD;
//    setSetting("mainSplitterState",splitter->saveState());

//    if (tab) {
//        setSetting("upperSplitterState",tab->saveState());
//        QByteArray treeHeaderState = tab->filesTable->header()->saveState();
//        setSetting("treeHeaderState", treeHeaderState);
//    }

//    QVariantMap map;
//    for (int i=0; i<tabWidget->count(); ++i) {
//        if (Tab *t = qobject_cast<Tab *>(tabWidget->widget(i))) {
//            if (!t->folders.isEmpty())
//                map.insert(tabWidget->tabText(i), t->folders);
//        }
//    }

//    setSetting("folders1", map);

//    plot->deleteGraphs();

//    for (int i= tabWidget->count()-1; i>=0; --i) {
//        closeTab(i);
//    }

//    ColorSelector::instance()->drop();
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
    addFolder(directory, false /*with subfolders*/, false /*silent*/);
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
    addFolder(directory, true /*with subfolders*/, false /*silent*/);
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
        if (!tab->model->contains(file))
            toAdd << file;
    }
    if (toAdd.isEmpty()) return;

    addFiles(toAdd);
    if (toAdd.size() < filesToAdd.size() && !silent) {
        QMessageBox::information(this,QString("База данных"),
                                 toAdd.isEmpty()?QString("Некоторые файлы уже есть в базе"):
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

    QStringList duplicatedFiles;
    QList<FileDescriptor *> files = tab->model->selectedFiles();
    foreach (FileDescriptor *d, files) {
        plot->deleteGraphs(d);

        if (tab->folders.contains(d->fileName()))
            tab->folders.removeOne(d->fileName());
        else if (tab->folders.contains(d->fileName()+":0"))
            tab->folders.removeOne(d->fileName()+":0");
        else if (tab->folders.contains(d->fileName()+":1"))
            tab->folders.removeOne(d->fileName()+":1");
        if (duplicated(d)) duplicatedFiles << d->fileName();
    }
    tab->channelsTable->setRowCount(0);

    tab->model->deleteFiles(duplicatedFiles);

    if (tab->model->size() == 0)
        tab->folders.clear();
}

QVector<int> MainWindow::selectedChannels() const
{
    QVector<int>  selectedIndexes;
    for (int i=0; i<tab->channelsTable->rowCount(); ++i)
        if (tab->channelsTable->item(i,0)->isSelected())
            selectedIndexes << i;
    return selectedIndexes;
}

QVector<int> MainWindow::plottedChannels() const
{
    QVector<int>  indexes;
    for (int i=0; i<tab->channelsTable->rowCount(); ++i)
        if (tab->channelsTable->item(i,0)->checkState()==Qt::Checked)
            indexes << i;
    return indexes;
}

void MainWindow::deleteChannels() /** SLOT */
{DD;
    QVector<int> channelsToDelete = selectedChannels();
    if (channelsToDelete.isEmpty()) return;

    if (QMessageBox::question(this,"DeepSea Base",
                              QString("Выделенные %1 каналов будут \nудалены из записи. Продолжить?").arg(channelsToDelete.size())
                              )==QMessageBox::Yes) {
        deleteChannels(tab->record, channelsToDelete);
        updateChannelsTable(tab->record);
    }
}

void MainWindow::deleteChannelsBatch()
{
    QVector<int> channels = selectedChannels();
    if (channels.isEmpty()) return;

    QList<FileDescriptor*> filesToDelete = tab->model->selectedFiles();
    if (filesToDelete.isEmpty()) return;

    // выделен только один файл
    if (filesToDelete.size()<2) {
        deleteChannels();
        return;
    }

    if (QMessageBox::question(this,"DeepSea Base",
                              QString("Выделенные %1 каналов будут \n"
                              "удалены из всех выделенных записей. Продолжить?").arg(channels.size()))
        !=QMessageBox::Yes)
        return;

    // проверка на количество каналов в файлах
    foreach (FileDescriptor *d, filesToDelete) {
        if (d->channelsCount()<=channels.last()) {
            if (QMessageBox::question(this,"DeepSea Base","В некоторых записях меньше каналов, чем заявлено\n"
                                      "к удалению. Продолжить?")==QMessageBox::Yes)
                break;
            else
                return;
        }
    }

    foreach (FileDescriptor *d, filesToDelete)
        deleteChannels(d, channels);

    updateChannelsTable(tab->record);
}

void MainWindow::copyChannels() /** SLOT */
{DD;
    QVector<int> channelsToCopy = selectedChannels();
    if (channelsToCopy.isEmpty()) return;

    if (copyChannels(tab->record, channelsToCopy)) {
        updateChannelsTable(tab->record);
    }
}

void MainWindow::moveChannels() /** SLOT */
{DD;
    // сначала копируем каналы, затем удаляем
    QVector<int> channelsToMove = selectedChannels();
    if (channelsToMove.isEmpty()) return;

    if (QMessageBox::question(this,"DeepSea Base","Выделенные каналы будут \nудалены из файла. Продолжить?")==QMessageBox::Yes) {
        if (copyChannels(tab->record, channelsToMove)) {
            deleteChannels(tab->record, channelsToMove);
            updateChannelsTable(tab->record);
        }
    }
}

void MainWindow::addCorrection()
{DD;
    if (plot->hasGraphs()) {
        QList<FileDescriptor*> files = tab->model->selectedFiles();

        CorrectionDialog dialog(plot, files);
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

                double corr = value.toDouble();
                if (corr != 0.0) {
                    corrected = true;
                    dfd->channel(row-2)->data()->setTemporaryCorrection(corr, 0);
                    dfd->channel(row-2)->data()->makeCorrectionConstant();
                    dfd->channel(row-2)->setCorrection(dfd->channel(row-2)->data()->correctionString());
                    dfd->channel(row-2)->data()->removeCorrection();
                }
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

bool MainWindow::deleteChannels(FileDescriptor *file, const QVector<int> &channelsToDelete)
{DD;
    for (int i=0; i<channelsToDelete.size() - 1; ++i)
        plot->deleteGraph(file, channelsToDelete.at(i), false);
    plot->deleteGraph(file, channelsToDelete.last(), true);

    file->deleteChannels(channelsToDelete);

    return true;
}

bool MainWindow::copyChannels(FileDescriptor *descriptor, const QVector<int> &channelsToCopy)
{DD;
    QString startFile = MainWindow::getSetting("startDir").toString();

    QFileDialog dialog(this, "Выбор файла для записи каналов", startFile,
                       "Файлы dfd (*.dfd);;Файлы uff (*.uff);;Файлы unv (*.unv)");
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);
    QString defaultSuffix = QFileInfo(startFile).suffix();
    if (defaultSuffix.isEmpty()) defaultSuffix = "dfd";
    dialog.setDefaultSuffix(defaultSuffix);

    QStringList suffixes = QStringList()<<"dfd"<<"uff"<<"unv";
    QStringList filters = QStringList()<<"Файлы dfd (*.dfd)"<<"Файлы uff (*.uff)"<<"Файлы unv (*.unv)";

    // если файл - dfd, то мы можем записать его только в dfd файл с таким же типом данных и шагом по x
    // поэтому если они различаются, то насильно записываем каналы в файл uff

    // мы не можем записывать каналы с разным типом/шагом в один файл,
    //поэтому добавляем фильтр
    QSortFilterProxyModel *proxy = new DfdFilterProxy(descriptor, this);
    dialog.setProxyModel(proxy);

    dialog.setFileMode(QFileDialog::AnyFile);


    QStringList selectedFiles;
    QString selectedFilter;
    if (dialog.exec()) {
        selectedFiles = dialog.selectedFiles();
        selectedFilter = dialog.selectedNameFilter();
    }
    if (selectedFiles.isEmpty()) return false;

    QString file = selectedFiles.first();

    QString currentSuffix = QFileInfo(file).suffix().toLower();
    QString filterSuffix = suffixes.at(filters.indexOf(selectedFilter));
    if (currentSuffix != filterSuffix)
        file.append(filterSuffix);

    MainWindow::setSetting("startDir", file);

    // ИЩЕМ ЭТОТ ФАЙЛ СРЕДИ ДОБАВЛЕННЫХ В БАЗУ
    FileDescriptor *dfd = findDescriptor(file);

    if (!dfd) {//не нашли файл в базе, нужно создать новый объект
        dfd = createDescriptor(file);
        if (!dfd) return false; // неизвестный тип файла

        if (QFileInfo(file).exists()) {// добавляем каналы в существующий файл
            dfd->read();
        }
        else {// такого файла не существует, создаем новый файл и записываем в него каналы
            dfd->fillPreliminary(descriptor->type());
        }
        if (dfd->legend().isEmpty())
            dfd->setLegend(descriptor->legend());
        dfd->copyChannelsFrom(descriptor, channelsToCopy);

        dfd->fillRest();


        dfd->setChanged(true);
        //dfd->setDataChanged(true);
        dfd->write();
        //dfd->writeRawFile();
        addFile(dfd);
        if (!tab->folders.contains(file)) tab->folders << file;
    }
    else {
        dfd->copyChannelsFrom(descriptor, channelsToCopy);
        if (dfd->legend().isEmpty())
            dfd->setLegend(descriptor->legend());

        dfd->setChanged(true);
        dfd->setDataChanged(true);
        dfd->write();
        dfd->writeRawFile();
        tab->model->updateFile(dfd);
    }
    return true;
}

void MainWindow::calculateMean()
{DD;
    const int graphsSize = plot->graphs.size();
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
        else {
            meanDfd = createDescriptor(meanDfdFile);
        }

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
        tab->model->updateFile(meanDfd);
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
    QList<FileDescriptor *> records = tab->model->selectedFiles();

    EditDescriptionsDialog dialog(records, this);
    if (dialog.exec()) {
        QHash<FileDescriptor*,DescriptionList> descriptions = dialog.descriptions();
        QHashIterator<FileDescriptor*,DescriptionList> it(descriptions);
        while (it.hasNext()) {
            it.next();
            tab->model->setDataDescriptor(it.key(), it.value());
        }
    }
}

void MainWindow::save()
{
    for (int i=0; i<tabWidget->count(); ++i) {
        Tab *t = qobject_cast<Tab *>(tabWidget->widget(i));
        if (t)
            t->model->save();
    }
}

void MainWindow::convertMatFiles()
{
    MatlabConverterDialog dialog(this);
    if (dialog.exec()) {
        if (dialog.addFiles()) {
            QStringList files = dialog.getConvertedFiles();
            this->addFiles(files);
            foreach (const QString &file, files)
                if (!tab->folders.contains(file)) tab->folders << file;
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

void MainWindow::saveTimeSegment(const QList<FileDescriptor *> &files, double from, double to)
{
    QThread *thread = new QThread;
    TimeSlicer *converter = new TimeSlicer(files, from, to);
    converter->moveToThread(thread);

    QProgressDialog *progress = new QProgressDialog("Сохранение вырезки...", "Отменить сохранение", 0, files.size(), this);
    progress->setWindowModality(Qt::WindowModal);

    connect(thread, SIGNAL(started()), converter, SLOT(start()));
    connect(converter, SIGNAL(finished()), thread, SLOT(quit()));
    connect(converter, SIGNAL(finished()), progress, SLOT(accept()));
    connect(progress, SIGNAL(canceled()), thread, SLOT(quit()));
    connect(converter, &TimeSlicer::finished, [=](){
        QStringList newFiles = converter->getNewFiles();
        addFiles(newFiles);
    });
    connect(converter, SIGNAL(tick(int)), progress, SLOT(setValue(int)));

    progress->show();
    progress->setValue(0);

    thread->start();
}

void MainWindow::switchSergeiMode()
{
    sergeiMode = !sergeiMode;
}

void MainWindow::editYName()
{
    if (!tab) return;

    QVector<int>  selectedIndexes;
    for (int i=0; i<tab->channelsTable->rowCount(); ++i)
        if (tab->channelsTable->item(i,1)->isSelected())
            selectedIndexes << i;
    if (selectedIndexes.isEmpty()) return;

    QString newYName = QInputDialog::getText(this, "Новая единица измерения", "Введите новую единицу");
    if (newYName.isEmpty()) return;

    foreach (int i, selectedIndexes) {
        tab->channelsTable->item(i,1)->setText(newYName);
    }
}

QVector<int> computeIndexes(QVector<int> notYetMoved, bool up, int totalSize)
{DD;
    QVector<int> moved;
    if (up) {
        while (notYetMoved.size()>0) {
            int j=notYetMoved.takeFirst();
            if (j==0 || moved.contains(j-1)) moved << j;
            else moved << j-1;
        }
    }
    else {
        int lastIndex = totalSize-1;
        while (notYetMoved.size()>0) {
            int j=notYetMoved.takeLast();
            if (j==lastIndex || moved.contains(j+1)) moved.prepend(j);
            else moved.prepend(j+1);
        }
    }
    return moved;
}

void MainWindow::moveChannels(bool up)
{DD;
    QVector<int> selectedIndexes = selectedChannels();

    QVector<int> newIndexes = computeIndexes(selectedIndexes, up, tab->record->channelsCount());

    tab->record->move(up, selectedIndexes, newIndexes);

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

void MainWindow::updateChannelsTable(const QModelIndex &current, const QModelIndex &previous)
{DD;
    Q_UNUSED(previous);

    if (!tab || !current.isValid()) return;

    if (current.model() != tab->sortModel) return;

    QModelIndex index = tab->sortModel->mapToSource(current);
    updateChannelsTable(tab->model->file(index.row()));
}

void MainWindow::updateChannelsTable(FileDescriptor *dfd)
{DD;
    QVector<int> plottedChannelsNumbers;
    if (sergeiMode) {
        plottedChannelsNumbers = plottedChannels();

        if (!plottedChannelsNumbers.isEmpty()) {
            foreach (int channel, plottedChannelsNumbers) {
                if (tab->channelsTable->rowCount()>channel)
                    tab->channelsTable->item(channel,0)->setCheckState(Qt::Unchecked);
            }
            updateChannelsHeaderState();
        }
    }

    tab->record = dfd;
    if (!dfd) return;



    tab->channelsTable->blockSignals(true);
    tab->channelsTable->setRowCount(0);
    tab->channelsTable->blockSignals(false);

    if (!dfd->fileExists()) {
        QMessageBox::warning(this,"Не могу получить список каналов","Такого файла уже нет");
        return;
    }

    tab->filePathLabel->setText(dfd->fileName());

    int chanCount = dfd->channelsCount();
    if (chanCount == 0) return;

    QStringList headers = dfd->getHeadersForChannel(0);

    tab->channelsTable->blockSignals(true);
    tab->channelsTable->clear();
    tab->channelsTable->setColumnCount(headers.size());
    tab->channelsTable->setHorizontalHeaderLabels(headers);
    tab->channelsTable->setRowCount(0);
    tab->channelsTable->setRowCount(chanCount);

    QFont boldFont = tab->channelsTable->font();
    boldFont.setBold(true);

    for (int i=0; i<chanCount; ++i) {
        Channel *ch = dfd->channel(i);
        QStringList data = ch->getInfoData();
        Qt::CheckState state = ch->checkState();
        for (int col=0; col<headers.size(); ++col) {
            QTableWidgetItem *ti = new QTableWidgetItem(data.at(col));
            if (col == 0) {
                ti->setCheckState(state);
                if (state==Qt::Checked) ti->setFont(boldFont);
                if (ch->color().isValid()) {
                    ti->setTextColor(Qt::white);
                    ti->setBackgroundColor(ch->color());
                }
            }
            if (col != 0 &&
                headers.at(col) != "Ед.изм." &&
                headers.at(col) != "Описание")
                ti->setFlags(ti->flags() ^ Qt::ItemIsEnabled);
            tab->channelsTable->setItem(i,col,ti);
        }
    }
    updateChannelsHeaderState();

    tab->channelsTable->blockSignals(false);

    if (sergeiMode) {
        if (!plottedChannelsNumbers.isEmpty()) {
            foreach (int channel, plottedChannelsNumbers) {
                if (tab->channelsTable->rowCount()>channel) tab->channelsTable->item(channel,0)->setCheckState(Qt::Checked);
            }
            updateChannelsHeaderState();
        }
    }
}

void MainWindow::maybePlotChannel(QTableWidgetItem *item)
{DD;
    if (!tab) return;
    if (!item) return;
    int column = item->column();
    int row = item->row();
    Channel *ch = tab->record->channel(row);

    if (tab->channelsTable->horizontalHeaderItem(column)->text() == "Описание") {
        ch->setDescription(item->text());
        tab->record->setChanged(true);
//        tab->record->write();

        if (tab->model->selected().size()>1) {
            if (QMessageBox::question(this,"DeepSea Base","Выделено несколько файлов. Записать такое описание канала\n"
                                      "во все эти файлы?")==QMessageBox::Yes)
            {
                tab->model->setChannelDescription(row, item->text());
            }
        }
    }

    else if (tab->channelsTable->horizontalHeaderItem(column)->text() == "Ед.изм.") {
        QString oldYName = ch->yName();
        if (item->text() != oldYName) {
            ch->setYName(item->text());
            tab->record->setChanged(true);
        }
    }



    else if (column == 0) {
        if (ch->name() != item->text()) {
            ch->setName(item->text());
            tab->record->setChanged(true);

            if (tab->model->selected().size()>1) {
                if (QMessageBox::question(this,"DeepSea Base","Выделено несколько файлов. Записать такое название канала\n"
                                          "во все эти файлы?")==QMessageBox::Yes)
                {
                    tab->model->setChannelName(row, item->text());
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

        tab->channelsTable->blockSignals(true);
        bool plotted = true;

        if (state == Qt::Checked) {
            QColor col;
            int fileNumber = tab->model->data(tab->model->modelIndexOfFile(tab->record,0), 0).toInt();

            plotted = plot->plotChannel(tab->record, row, &col, plotOnRight, fileNumber);
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

            if (tab->model->selected().size()>1 && QApplication::keyboardModifiers() & Qt::ControlModifier) {
                QList<FileDescriptor*> selectedFiles = tab->model->selectedFiles();
                foreach (FileDescriptor *f, selectedFiles) {
                    if (f == tab->record) continue;
                    if (f->channelsCount()<=row) continue;

                    int fileNumber = tab->model->data(tab->model->modelIndexOfFile(f,0), 0).toInt();
                    plotted = plot->plotChannel(f, row, &col, plotOnRight, fileNumber);
                    if (plotted) {
                        f->channel(row)->setCheckState(Qt::Checked);
                        f->channel(row)->setColor(col);
                    }
                    else {
                        f->channel(row)->setCheckState(Qt::Unchecked);
                        f->channel(row)->setColor(QColor());
                    }
                    tab->model->updateFile(f, 1);
                }
            }
        }
        else if (state == Qt::Unchecked) {
            plot->deleteGraph(tab->record, row);
            item->setFont(tab->channelsTable->font());
            ch->setCheckState(Qt::Unchecked);
            item->setBackgroundColor(Qt::white);
            item->setTextColor(Qt::black);
            ch->setColor(QColor());

            if (tab->model->selected().size()>1 && QApplication::keyboardModifiers() & Qt::ControlModifier) {
                QList<FileDescriptor*> selectedFiles = tab->model->selectedFiles();
                foreach (FileDescriptor *f, selectedFiles) {
                    if (f == tab->record) continue;
                    if (f->channelsCount()<=row) continue;

                    plot->deleteGraph(f, row);
                    f->channel(row)->setCheckState(Qt::Unchecked);
                    f->channel(row)->setColor(QColor());

                    tab->model->updateFile(f, 1);
                }
            }
        }
        tab->channelsTable->blockSignals(false);

        tab->model->updateFile(tab->record, 1);

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

    QList<FileDescriptor *> records = tab->model->selectedFiles();
    for (int i=records.size()-1; i>=0; --i) {
        if (!records[i]->isSourceFile()) {
            // only convert source files
            records.removeAt(i);
        }
    }
    if (records.isEmpty()) {
        QMessageBox::warning(this,QString("DeepSea Base"),
                             QString("Не выделено ни одного файла с исходными временными данными"));
        return;
    }

    CalculateSpectreDialog dialog(records, this);
//    FilesProcessorDialog dialog(records, this);

    if (dialog.exec()) {
        QStringList newFiles = dialog.getNewFiles();
        addFiles(newFiles);
        foreach (const QString &file, newFiles)
            if (!tab->folders.contains(file)) tab->folders << file;
    }
}

void MainWindow::convertFiles()
{
    if (!tab) return;
    QList<FileDescriptor *> records = tab->model->selectedFiles();

    ConverterDialog dialog(records, this);
    dialog.exec();

    QStringList newFiles = dialog.getConvertedFiles();
    addFiles(newFiles);
}

void MainWindow::copyToLegend()
{
    if (!tab) return;
    QList<FileDescriptor *> records = tab->model->selectedFiles();

    foreach (FileDescriptor *f, records) {
        f->setLegend(QFileInfo(f->fileName()).completeBaseName());
        tab->model->updateFile(f, 9);
        plot->updateLegends();
    }
}

void MainWindow::calculateThirdOctave()
{DD;
    if (!tab) return;
    QList<FileDescriptor *> records = tab->model->selectedFiles();
    for (int i=records.size()-1; i>=0; --i) {
        if (records[i]->type() <= Descriptor::TimeResponse) {
            // only convert spectres
            records.removeAt(i);
        }
    }

    if (records.isEmpty()) {
        QMessageBox::warning(this,QString("DeepSea Base"),
                             QString("Не выделено ни одного файла со спектрами"));
        return;
    }


    foreach (FileDescriptor *fd, records) {
        QString dfd = fd->calculateThirdOctave();
        if (FileDescriptor *found = findDescriptor(dfd)) {
            tab->model->updateFile(found);
        }
        else {
            addFiles(QStringList()<<dfd);
        }
    }
}

void MainWindow::calculateMovingAvg()
{
    const int graphsSize = plot->graphs.size();
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
        tab->model->updateFile(avgDfd);
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

    QVector<int> plotted = plottedChannels();

    tab->channelsTable->blockSignals(true);
    foreach (int i, plotted) {
        tab->channelsTable->item(i,0)->setCheckState(Qt::Unchecked);
        tab->channelsTable->item(i,0)->setFont(tab->channelsTable->font());
        tab->channelsTable->item(i,0)->setBackgroundColor(Qt::white);
        tab->channelsTable->item(i,0)->setTextColor(Qt::black);
    }
    tab->tableHeader->setCheckState(0,Qt::Unchecked);
    tab->channelsTable->blockSignals(false);
}

void MainWindow::rescanBase()
{DD;
    if (!tab) return;

    // first we delete all graphs affected
    plot->deleteGraphs();

    // next we clear all tab and populate it with folders anew
    tab->model->clear();
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

    addDescriptors(QList<FileDescriptor *>()<<descriptor);
//    tab->filesTable->setCurrentIndex(tab->model->modelIndexOfFile(descriptor, 1));
}

FileDescriptor *MainWindow::findDescriptor(const QString &file)
{DD;
    if (tab) {
        if (FileDescriptor *f = tab->model->find(file))
            return f;
    }
    for (int i=0; i<tabWidget->count(); ++i) {
        Tab *t = qobject_cast<Tab *>(tabWidget->widget(i));
        if (t==tab) continue;

        if (FileDescriptor *f = t->model->find(file))
            return f;
    }
    return 0;
}

bool MainWindow::duplicated(FileDescriptor *file) const
{
    if (!file) return false;

    for (int i=0; i<tabWidget->count(); ++i) {
        Tab *t = qobject_cast<Tab *>(tabWidget->widget(i));
        if (t==tab) continue;

        if (t->model->find(file))
            return true;
    }
    return false;
}

void MainWindow::setCurrentAndPlot(FileDescriptor *d, int channelIndex)
{DD;
    if (tab) {
        int i = tab->model->rowOfFile(d);

        if (i > -1) {
            updateChannelsTable(d);
            //tab->filesTable->setCurrentIndex(tab->model->modelIndexOfFile(d, 0));
            //tab->filesTable->selectionModel()->select(tab->model->modelIndexOfFile(d, 0), QItemSelectionModel::ClearAndSelect);
            tab->channelsTable->item(channelIndex, 0)->setCheckState(Qt::Checked);
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
    QAxObject * worksheet = workbook->querySubObject("ActiveSheet");


     FileDescriptor *descriptor = plot->graphs.at(0)->descriptor;
     Channel *channel = plot->graphs.at(0)->channel;

     // проверяем, все ли каналы из одного файла
     bool allChannelsFromOneFile = true;
     for (int i=1; i<plot->graphs.size(); ++i) {
         if (plot->graphs.at(i)->descriptor->fileName() != descriptor->fileName()) {
             allChannelsFromOneFile = false;
             break;
         }
     }

     //проверяем, все ли каналы имеют одинаковое разрешение по х
     bool allChannelsHaveSameXStep = true;
     if (channel->xStep()==0.0) allChannelsHaveSameXStep = false;
     for (int i=1; i<plot->graphs.size(); ++i) {
         if (plot->graphs.at(i)->channel->xStep() == 0.0) {
             allChannelsHaveSameXStep = false;
             break;
         }
         if (qAbs(plot->graphs.at(i)->channel->xStep() - channel->xStep()) >= 1e-10) {
             allChannelsHaveSameXStep = false;
             break;
         }
     }

     //проверяем, все ли каналы имеют одинаковую длину
     bool allChannelsHaveSameLength = true;
     for (int i=1; i<plot->graphs.size(); ++i) {
         if (plot->graphs.at(i)->channel->samplesCount() != channel->samplesCount()) {
             allChannelsHaveSameLength = false;
             break;
         }
     }

     const int samplesCount = channel->samplesCount();
     const double step = channel->xStep();
     const bool zeroStepDetected = step<1e-9;

     const bool writeToSeparateColumns = !allChannelsHaveSameXStep ||
                                         !allChannelsHaveSameLength ||
                                         zeroStepDetected;

     double minX = channel->xMin();
     double maxX = channel->xMax();

     Range range = plot->xRange();

     for (int i=1; i<plot->graphs.size(); ++i) {
         Channel *ch = plot->graphs.at(i)->channel;
         if (ch->xMin() < minX) minX = ch->xMin();
         if (ch->xMax() > maxX) maxX = ch->xMax();
     }
     if (minX >= range.min && maxX <= range.max)
         fullRange = true;

     // определяем, будут ли экспортированы графики;
     bool exportPlots = true;
     if (samplesCount > 32000 && fullRange && !dataOnly) {
         QMessageBox::warning(this, "Слишком много данных",
                              "В одном или всех каналах число отсчетов превышает 32000.\n"
                              "Будут экспортированы только данные, но не графики");
         exportPlots = false;
     }
     if (dataOnly) exportPlots = false;

     // записываем название файла и описатели
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
         for (int i=0; i<plot->graphs.size(); ++i) {
             Curve *curve = plot->graphs.at(i);
             QStringList descriptions = twoStringDescription(curve->descriptor->dataDescriptor());

             QAxObject *cells = !writeToSeparateColumns ? worksheet->querySubObject("Cells(Int,Int)", 1, 2+i)
                                                       : worksheet->querySubObject("Cells(Int,Int)", 1, 2+i*2);
             cells->setProperty("Value", curve->descriptor->fileName());

             cells = !writeToSeparateColumns ? worksheet->querySubObject("Cells(Int,Int)", 2, 2+i)
                                            : worksheet->querySubObject("Cells(Int,Int)", 2, 2+i*2);
             if (cells) cells->setProperty("Value", descriptions.first());

             cells = !writeToSeparateColumns ? worksheet->querySubObject("Cells(Int,Int)", 3, 2+i)
                                            : worksheet->querySubObject("Cells(Int,Int)", 3, 2+i*2);
             if (cells) cells->setProperty("Value", descriptions.at(1));

             delete cells;
         }
     }

     // записываем название канала
     for (int i=0; i<plot->graphs.size(); ++i) {
         Curve *curve = plot->graphs.at(i);
         QAxObject *cells = !writeToSeparateColumns ? worksheet->querySubObject("Cells(Int,Int)", 4, 2+i)
                                                   : worksheet->querySubObject("Cells(Int,Int)", 4, 2+i*2);
         cells->setProperty("Value", curve->title());
         delete cells;
     }

     QVector<int> selectedSamples;

     // если все каналы имеют одинаковый шаг по х, то в первый столбец записываем
     // данные х
     // если каналы имеют разный шаг по х, то для каждого канала отдельно записываем
     // по два столбца
     if (!writeToSeparateColumns) {
         const int numCols = plot->graphs.size();

         QList<QVariant> cellsList;
         QList<QVariant> rowsList;
         for (int i = 0; i < samplesCount; ++i) {
             double val = channel->data()->xValue(i);
             if (!fullRange && (val < range.min || val > range.max) ) continue;

             cellsList.clear();
             cellsList << val;
             for (int j = 0; j < numCols; ++j) {
                 cellsList << plot->graphs.at(j)->channel->data()->yValue(i);
             }
             rowsList << QVariant(cellsList);
         }
         int numRows = rowsList.size();

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
         for (int i=0; i<plot->graphs.size(); ++i) {
             Curve *curve = plot->graphs.at(i);
             Channel *ch = curve->channel;

             QList<QVariant> cellsList;
             QList<QVariant> rowsList;
             for (int j = 0; j < ch->samplesCount(); j++) {
                 double val = ch->data()->xValue(j);
                 if (!fullRange && (val < range.min || val > range.max) ) continue;

                 cellsList.clear();
                 cellsList << val << ch->data()->yValue(j);
                 rowsList << QVariant(cellsList);
             }
             int numRows = rowsList.size();
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
         chart->setProperty("ChartType", 75);
         QAxObject * series = chart->querySubObject("SeriesCollection");

         // отдельно строить кривые нужно, только если у нас много пар столбцов с данными
         if (writeToSeparateColumns) {
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

             for (int i=0; i<plot->graphs.size(); ++i) {
                 QAxObject * serie = series->querySubObject("NewSeries()");
                 if (serie) {
                     //xvalues
                     QAxObject* Cell1 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 5, 1+i*2);
                     QAxObject* Cell2 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 5 + selectedSamples.at(i)-1, 1 + i*2);
                     if (Cell1 && Cell2) {
                         QAxObject * xvalues = worksheet->querySubObject("Range(const QVariant&,const QVariant&)", Cell1->asVariant(), Cell2->asVariant());
                         if (xvalues)
                             serie->setProperty("XValues", xvalues->asVariant());
                         delete xvalues;
                     }
                     delete Cell1;
                     delete Cell2;

                     //yvalues
                     Cell1 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 5, 2+i*2);
                     Cell2 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 5 + selectedSamples.at(i)-1, 2 + i*2);
                     if (Cell1 && Cell2) {
                         QAxObject * yvalues = worksheet->querySubObject("Range(const QVariant&,const QVariant&)", Cell1->asVariant(), Cell2->asVariant());
                         if (yvalues)
                             serie->setProperty("Values", yvalues->asVariant());
                         delete yvalues;
                     }
                     delete Cell1;
                     delete Cell2;
                 }
                 delete serie;
             }
         }

         // перемещаем графики на дополнительную вертикальную ось,
         // если они были там в программе
         // и меняем название кривой
         int seriesCount = series->property("Count").toInt();
         bool addRightAxis = false;
         for ( int i=0; i<seriesCount; ++i) {
             Curve *curve = plot->graphs.at(i);
             QAxObject * serie = series->querySubObject("Item (int)", i+1);
             if (serie) {
                 if (curve->yAxis()==QwtPlot::yRight) {
                     serie->setProperty("AxisGroup", 2);
                     addRightAxis = true;
                 }
                 serie->setProperty("Name", curve->channel->name());
             }
             delete serie;
         }
         if (addRightAxis) {
             QAxObject *yAxis = chart->querySubObject("Axes(const QVariant&,const QVariant&)", 2,2);
             if (yAxis) setAxis(yAxis, stripHtml(plot->axisTitle(QwtPlot::yRight).text()));
             delete yAxis;
         }


         // добавляем подписи осей
         QAxObject *xAxis = chart->querySubObject("Axes(const QVariant&)", 1);
         if (xAxis) {
             setAxis(xAxis, stripHtml(plot->axisTitle(QwtPlot::xBottom).text()));
             xAxis->setProperty("MaximumScale", range.max);
             xAxis->setProperty("MinimumScale", int(range.min/10)*10);
//             if (zeroStepDetected) {
//                 xAxis->setProperty("ScaleType", "xlLogarithmic");
//                 xAxis->setProperty("LogBase", 2);
//             }
         }
         delete xAxis;

         QAxObject *yAxis = chart->querySubObject("Axes(const QVariant&)", 2);
         if (yAxis) {
             setAxis(yAxis, stripHtml(plot->axisTitle(QwtPlot::yLeft).text()));
             yAxis->setProperty("CrossesAt", -1000);
         }
         delete yAxis;

         // рамка вокруг графика
         QAxObject *plotArea = chart->querySubObject("PlotArea");
         if (plotArea) setLineColor(plotArea, 13);
         delete plotArea;

         // цвета графиков
         for (int i = 0; i< plot->graphs.size(); ++i) {
             Curve *curve = plot->graphs.at(i);
             QAxObject * serie = series->querySubObject("Item(int)", i+1);
             if (serie) {
                 QAxObject *format = serie->querySubObject("Format");
                 QAxObject *formatLine = format->querySubObject("Line");
                 if (formatLine) formatLine->setProperty("Weight", 1);

                 QAxObject *formatLineForeColor = formatLine->querySubObject("ForeColor");
                 if (formatLineForeColor) formatLineForeColor->setProperty("RGB", plot->graphs.at(i)->pen().color().rgb());

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
        if (Tab *t = qobject_cast<Tab*>(tabWidget->widget(index)))
            t->model->invalidateGraph(descriptor, channelIndex);
    }
    if (tab->record == descriptor)
        updateChannelsTable(descriptor);
}

void MainWindow::addDescriptors(const QList<FileDescriptor*> &files)
{DD;
    if (!tab) return;

    tab->model->addFiles(files);
}

void MainWindow::addFiles(QStringList &files)
{DD;
    if (!tab) return;

    QList<FileDescriptor *> items;

    foreach (const QString fileName, files) {
        if (fileName.isEmpty()) continue;

        if (tab->model->contains(fileName))
            continue;


        FileDescriptor *file = findDescriptor(fileName);
        if (!file) {
            file = createDescriptor(fileName);
            file->read();
        }
        if (file)
            items << file;
    }
    addDescriptors(items);
}

void Tab::filesSelectionChanged(const QItemSelection &newSelection, const QItemSelection &oldSelection)
{
    Q_UNUSED(oldSelection);
    if (newSelection.isEmpty()) filesTable->selectionModel()->setCurrentIndex(QModelIndex(), QItemSelectionModel::NoUpdate);

    QSet<int> indexes;

    QModelIndexList list = filesTable->selectionModel()->selection().indexes();
    foreach (const QModelIndex &i, list) indexes << sortModel->mapToSource(i).row();

    QList<int> l = indexes.toList();
    std::sort(l.begin(), l.end());

    model->setSelected(l);
    if (indexes.isEmpty()) channelsTable->setRowCount(0);
}


void MainWindow::closeEvent(QCloseEvent *event)
{
//    if (closeRequested()) {
//        event->accept();
//    }
//    else {
//        event->ignore();
//    }
//    QEventLoop q;
//    connect(this, SIGNAL(allClosed()), &q, SLOT(quit()));
    closeRequested();
//    q.exec();

    event->accept();
}

bool MainWindow::closeRequested()
{

    // сохранение состояния, сохранение файлов
    setSetting("mainSplitterState",splitter->saveState());

    if (tab) {
        setSetting("upperSplitterState",tab->saveState());
        QByteArray treeHeaderState = tab->filesTable->header()->saveState();
        setSetting("treeHeaderState", treeHeaderState);
    }

    QVariantMap map;
    for (int i=0; i<tabWidget->count(); ++i) {
        if (Tab *t = qobject_cast<Tab *>(tabWidget->widget(i))) {
            if (!t->folders.isEmpty())
                map.insert(tabWidget->tabText(i), t->folders);
        }
    }

    setSetting("folders1", map);

    plot->deleteGraphs();

    for (int i= tabWidget->count()-1; i>=0; --i) {
        closeTab(i);
    }

    ColorSelector::instance()->drop();

    emit allClosed();
    return true;
}
