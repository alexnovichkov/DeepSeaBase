#include "mainwindow.h"

#include <QtWidgets>


#include "calculatespectredialog.h"
#include "filesprocessordialog.h"
#include "sortabletreewidgetitem.h"
#include "headerview.h"
#include "plot/plot.h"
#include "tabwidget.h"
#include "colorselector.h"
#include "coloreditdialog.h"
#include "correctiondialog.h"
#include "plot/curve.h"
#include "plot/pointlabel.h"
#include "model.h"
#include "sortfiltermodel.h"
#include "filterheaderview.h"
#include "wavexporter.h"

#include <ActiveQt/ActiveQt>
#include "logging.h"

#include "editdescriptionsdialog.h"
#include "converters/matlabconvertor.h"
#include "converters/matlabconverterdialog.h"
#include "converters/esoconverterdialog.h"
#include "converters/converterdialog.h"
#include "timeslicer.h"
#include <QTime>
#include "channeltablemodel.h"
#include "converters/tdmsconverterdialog.h"
#include "htmldelegate.h"
#include "longoperation.h"
#include "filestable.h"
#include "channelstable.h"

#include "fileformats/formatfactory.h"

class DfdFilterProxy : public QSortFilterProxyModel
{
public:
    DfdFilterProxy(FileDescriptor *filter, QObject *parent)
        : QSortFilterProxyModel(parent), filter(filter)
    {
        if (filter) {
            filterByContent = true;
        }
    }
protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
    {
        QFileSystemModel *model = qobject_cast<QFileSystemModel *>(sourceModel());
        if (!model) return false;

        QModelIndex index0 = model->index(source_row, 0, source_parent);

        QFileInfo fi = model->fileInfo(index0);

        if (fi.isFile()) {
            if (suffixes.contains(fi.suffix().toLower())) {
                //принимаем все файлы, если не сравниваем с конкретным
                if (!filterByContent)
                    return true;

                QScopedPointer<FileDescriptor> descriptor(FormatFactory::createDescriptor(fi.canonicalFilePath()));

                if (descriptor->canTakeAnyChannels())
                    return true;

                descriptor->read();
                //частный случай: мы можем записать данные из SourceData в CuttedData, преобразовав их в floats,
                //но не наоборот
                if (descriptor->canTakeChannelsFrom(filter))
                    return true;
                else
                    return false;
            }
            else //не файлы dfd, uff, d94
                return false;
        }
        else //папки
            return true;
    }
private:
    FileDescriptor *filter;
    bool filterByContent = false;
    QStringList suffixes = FormatFactory::allSuffixes(true);
};

class StepItemDelegate : public QStyledItemDelegate
{
public:
    StepItemDelegate(QObject *parent = Q_NULLPTR) : QStyledItemDelegate(parent)
    {}

    virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QWidget *ed = QStyledItemDelegate::createEditor(parent, option, index);
        if (QDoubleSpinBox *spin = qobject_cast<QDoubleSpinBox*>(ed)) {
            spin->setDecimals(10);
        }

        return ed;
    }
};


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), tab(0)
{DD;
    setWindowTitle(tr("DeepSea Database ")+DEEPSEABASE_VERSION);

    mainToolBar = new QToolBar(this);
    addToolBar(mainToolBar);
    mainToolBar->setIconSize(QSize(24,24));

    plot = new Plot(this);
    connect(plot, SIGNAL(curveChanged(Curve*)), SLOT(onCurveColorChanged(Curve*)));
    connect(plot, SIGNAL(curveDeleted(Channel*)), SLOT(onCurveDeleted(Channel*)));
    connect(plot, SIGNAL(saveTimeSegment(QList<FileDescriptor*>,double,double)), SLOT(saveTimeSegment(QList<FileDescriptor*>,double,double)));
    connect(plot, SIGNAL(curvesChanged()), SLOT(updatePlottedChannelsNumbers()));
    connect(plot, &Plot::curvesCountChanged, this, &MainWindow::updateActions);
    connect(plot, &Plot::needPlotChannels, [=](const QVector<int> &channels, bool plotOnRight){
        if (tab) tab->channelModel->plotChannels(channels, plotOnRight);
    });

    QIcon addFolderIcon(":/icons/open24.png");
    addFolderIcon.addFile(":/icons/open.png");
    addFolderAct = new QAction(addFolderIcon, tr("Добавить папку"),this);
    addFolderAct->setShortcut(tr("Ctrl+O"));
    connect(addFolderAct, SIGNAL(triggered()), SLOT(addFolder()));

    addFolderWithSubfoldersAct = new QAction(tr("Добавить папку со всеми вложенными папками"),this);
    connect(addFolderWithSubfoldersAct, SIGNAL(triggered()), SLOT(addFolderWithSubfolders()));

    QIcon addFileIcon(":/icons/open_file_24.png");
    addFileIcon.addFile(":/icons/open_file_16.png");
    addFileAct = new QAction(addFileIcon, tr("Добавить файл"),this);
    connect(addFileAct, SIGNAL(triggered()), SLOT(addFile()));

    QIcon moveChannelsUpIcon(":/icons/move_up.png");
    moveChannelsUpIcon.addFile(":/icons/move_up_24.png");
    moveChannelsUpAct = new QAction(moveChannelsUpIcon, "Сдвинуть каналы вверх", this);
    moveChannelsUpAct->setShortcutContext(Qt::WidgetShortcut);
    moveChannelsUpAct->setShortcut(tr("Ctrl+Up"));
    connect(moveChannelsUpAct, SIGNAL(triggered()), SLOT(moveChannelsUp()));

    QIcon moveChannelsDownIcon(":/icons/move_down.png");
    moveChannelsDownIcon.addFile(":/icons/move_down_24.png");
    moveChannelsDownAct = new QAction(moveChannelsDownIcon, "Сдвинуть каналы вниз", this);
    moveChannelsDownAct->setShortcutContext(Qt::WidgetShortcut);
    moveChannelsDownAct->setShortcut(tr("Ctrl+Down"));
    connect(moveChannelsDownAct, SIGNAL(triggered()), SLOT(moveChannelsDown()));

    saveAct = new QAction("Сохранить файлы", this);
    saveAct->setIcon(QIcon(":/icons/disk.png"));
    saveAct->setShortcut(tr("Ctrl+S"));
    connect(saveAct, SIGNAL(triggered()), SLOT(save()));

    renameAct = new QAction(QIcon(":/icons/rename.png"),"Переименовать файл", this);
    connect(renameAct, &QAction::triggered, this, &MainWindow::renameDescriptor);

    delFilesAct = new QAction(QString("Удалить записи"), this);
    delFilesAct->setShortcut(Qt::Key_Delete);
    delFilesAct->setShortcutContext(Qt::WidgetShortcut);
    connect(delFilesAct, SIGNAL(triggered()), SLOT(deleteFiles()));

    plotAllChannelsAct = new QAction(QString("Построить все каналы"), this);
    connect(plotAllChannelsAct, SIGNAL(triggered()), SLOT(plotAllChannels()));

    plotAllChannelsOnRightAct = new QAction(QString("...на правой оси"), this);
    connect(plotAllChannelsOnRightAct, SIGNAL(triggered()), SLOT(plotAllChannelsAtRight()));

    plotSelectedChannelsAct = new QAction(QString("Построить выделенные каналы"), this);
    connect(plotSelectedChannelsAct, &QAction::triggered, [=](){
        tab->channelModel->plotChannels(tab->channelModel->selected(), false);
    });

    plotSelectedChannelsOnRightAct = new QAction(QString("...на правой оси"), this);
    connect(plotSelectedChannelsOnRightAct, &QAction::triggered, [=](){
        tab->channelModel->plotChannels(tab->channelModel->selected(), true);
    });

    exportChannelsToWavAct = new QAction(QString("Экспортировать в WAV"), this);
    connect(exportChannelsToWavAct, SIGNAL(triggered(bool)), SLOT(exportChannelsToWav()));


    QAction *plotHelpAct = new QAction(QIcon(":/icons/help.png"), "Справка", this);
    connect(plotHelpAct, &QAction::triggered, [](){QDesktopServices::openUrl(QUrl("help.html"));});

    aboutAct = new QAction("О програме", this);
    connect(aboutAct, &QAction::triggered, [=](){
        QString version = QString("DeepSea Database - версия %1").arg(qApp->applicationVersion());
        QMessageBox::about(this, tr("О программе"), version);
    });

    QIcon calculateSpectreIcon(":/icons/function.png");
    calculateSpectreIcon.addFile(":/icons/function16.png");
    calculateSpectreAct = new QAction(calculateSpectreIcon, QString("Обработать записи..."), this);
    connect(calculateSpectreAct, SIGNAL(triggered()), SLOT(calculateSpectreRecords()));

    convertAct = new QAction("Конвертировать файлы...", this);
    connect(convertAct, SIGNAL(triggered()), SLOT(convertFiles()));

    copyToLegendAct = new QAction("Перенести сюда названия файлов", this);
    connect(copyToLegendAct, SIGNAL(triggered()), SLOT(copyToLegend()));

    clearPlotAct  = new QAction(QString("Очистить график"), this);
    clearPlotAct->setIcon(QIcon(":/icons/cross.png"));
    connect(clearPlotAct, SIGNAL(triggered()), plot, SLOT(deleteAllCurves()));

    savePlotAct = new QAction(QString("Сохранить график..."), this);
    savePlotAct->setIcon(QIcon(":/icons/picture.png"));
    connect(savePlotAct, SIGNAL(triggered()), plot, SLOT(savePlot()));

    rescanBaseAct = new QAction(QString("Пересканировать базу"), this);
    rescanBaseAct->setIcon(QIcon(":/icons/revert.png"));
    rescanBaseAct->setShortcut(QKeySequence::Refresh);
    connect(rescanBaseAct, SIGNAL(triggered()), this, SLOT(rescanBase()));

    switchCursorAct = new QAction(QString("Показать/скрыть курсор"), this);
    switchCursorAct->setIcon(QIcon(":/icons/cursor.png"));
    switchCursorAct->setCheckable(true);
    switchCursorAct->setObjectName("simpleCursor");
    bool pickerEnabled = App->getSetting("pickerEnabled", true).toBool();
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

    meanAct = new QAction(QIcon(":/icons/mean.png"), QString("Вывести среднее (энерг.)"), this);
    connect(meanAct, SIGNAL(triggered()), this, SLOT(calculateMean()));

    movingAvgAct = new QAction(QIcon(":/icons/average.png"), QString("Рассчитать скользящее среднее"), this);
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
    deleteChannelsAct->setIcon(QIcon(":/icons/remove.png"));
    connect(deleteChannelsAct, SIGNAL(triggered()), this, SLOT(deleteChannels()));

    deleteChannelsBatchAct = new QAction("Удалить каналы в нескольких файлах...",this);
    connect(deleteChannelsBatchAct, SIGNAL(triggered()), this, SLOT(deleteChannelsBatch()));

    QMenu *deleteChannelsMenu = new QMenu(this);
    deleteChannelsMenu->addAction(deleteChannelsBatchAct);

    deleteChannelsAct->setMenu(deleteChannelsMenu);

    copyChannelsAct = new QAction(QIcon(":/icons/channel-copy.png"), "Копировать выделенные каналы в файл...", this);
    connect(copyChannelsAct, SIGNAL(triggered()), SLOT(copyChannels()));

    moveChannelsAct = new QAction(QIcon(":/icons/channel-cut.png"), "Переместить выделенные каналы в файл...", this);
    connect(moveChannelsAct, SIGNAL(triggered()), SLOT(moveChannels()));

    editDescriptionsAct = new QAction("Редактировать описание...", this);
    editDescriptionsAct->setIcon(QIcon(":/icons/descriptor.png"));
    connect(editDescriptionsAct, SIGNAL(triggered()), SLOT(editDescriptions()));

    QMenu *addFolderMenu = new QMenu(this);
    addFolderMenu->addAction(addFolderWithSubfoldersAct);
    addFolderAct->setMenu(addFolderMenu);

    convertMatFilesAct = new QAction("Конвертировать Matlab файлы...", this);
    connect(convertMatFilesAct,SIGNAL(triggered()),SLOT(convertMatFiles()));

    convertTDMSFilesAct = new QAction("Конвертировать TDMS файлы...", this);
    connect(convertTDMSFilesAct,SIGNAL(triggered()),SLOT(convertTDMSFiles()));

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


    mainToolBar->addAction(copyChannelsAct);
    mainToolBar->addAction(moveChannelsAct);
    mainToolBar->addAction(deleteChannelsAct);
    mainToolBar->addAction(meanAct);
    mainToolBar->addAction(movingAvgAct);
    mainToolBar->addAction(addCorrectionAct);
    mainToolBar->addAction(moveChannelsUpAct);
    mainToolBar->addAction(moveChannelsDownAct);

    mainToolBar->addSeparator();
    mainToolBar->addWidget(new QLabel("  График:"));
    mainToolBar->addAction(clearPlotAct);
    mainToolBar->addAction(savePlotAct);
    mainToolBar->addAction(copyToClipboardAct);
    mainToolBar->addAction(printPlotAct);
    //mainToolBar->addWidget(new QLabel("         ",this));

    mainToolBar->addAction(exportToExcelAct);
    mainToolBar->addAction(switchCursorAct);
//    mainToolBar->addSeparator();

    mainToolBar->addAction(interactionModeAct);
    mainToolBar->addAction(trackingCursorAct);
    mainToolBar->addAction(playAct);
    //mainToolBar->addSeparator();
    QWidget *fillerWidget = new QWidget(this);
    fillerWidget->setContentsMargins(0,0,0,0);
    fillerWidget->setSizePolicy(QSizePolicy::Expanding, fillerWidget->sizePolicy().verticalPolicy());
    mainToolBar->addWidget(fillerWidget);
    mainToolBar->addAction(plotHelpAct);


    QMenu *fileMenu = menuBar()->addMenu(tr("Файл"));
    fileMenu->addAction(addFolderAct);
    fileMenu->addAction(addFileAct);
    fileMenu->addAction(convertAct);
    fileMenu->addAction(convertMatFilesAct);
    fileMenu->addAction(convertTDMSFilesAct);
    fileMenu->addAction(convertEsoFilesAct);

    QMenu *recordsMenu = menuBar()->addMenu(QString("Записи"));
    recordsMenu->addAction(delFilesAct);
    recordsMenu->addAction(rescanBaseAct);
    recordsMenu->addAction(saveAct);
    recordsMenu->addAction(renameAct);

    QMenu *settingsMenu = menuBar()->addMenu(QString("Настройки"));
    settingsMenu->addAction(editColorsAct);
    settingsMenu->addAction(aboutAct);

    tabWidget = new TabWidget(this);
    connect(tabWidget,SIGNAL(newTab()),this, SLOT(createNewTab()));
    connect(tabWidget,SIGNAL(closeTab(int)),this, SLOT(closeTab(int)));
    connect(tabWidget,SIGNAL(closeOtherTabs(int)), this, SLOT(closeOtherTabs(int)));
    connect(tabWidget,SIGNAL(renameTab(int)),this, SLOT(renameTab(int)));
    connect(tabWidget,SIGNAL(currentChanged(int)),SLOT(changeCurrentTab(int)));

    autoscaleXAct = new QAction("Автомасштабирование по оси X", this);
    autoscaleXAct->setIcon(QIcon(":/icons/autoscale-x.png"));
    autoscaleXAct->setCheckable(true);
    bool autoscale = App->getSetting("autoscale-x", true).toBool();
    connect(autoscaleXAct, &QAction::toggled, [this](bool toggled){
        plot->toggleAutoscale(0 /* x axis */,toggled);
        App->setSetting("autoscale-x", toggled);
    });
    autoscaleXAct->setChecked(autoscale);
    plot->toggleAutoscale(0 /* x axis */, autoscale);

    autoscaleYAct = new QAction("Автомасштабирование по оси Y", this);
    autoscaleYAct->setIcon(QIcon(":/icons/autoscale-y-main.png"));
    autoscaleYAct->setCheckable(true);
    autoscale = App->getSetting("autoscale-y", true).toBool();
    connect(autoscaleYAct, &QAction::toggled, [this](bool toggled){
        plot->toggleAutoscale(1 /* y axis */,toggled);
        App->setSetting("autoscale-y", toggled);
    });
    autoscaleYAct->setChecked(autoscale);
    plot->toggleAutoscale(1 /* x axis */, autoscale);

    autoscaleYSlaveAct = new QAction("Автомасштабирование по правой оси Y", this);
    autoscaleYSlaveAct->setIcon(QIcon(":/icons/autoscale-y-slave.png"));
    autoscaleYSlaveAct->setCheckable(true);
    autoscale = App->getSetting("autoscale-y-slave", true).toBool();
    connect(autoscaleYSlaveAct, &QAction::toggled, [this](bool toggled){
        plot->toggleAutoscale(2 /* y slave axis */,toggled);
        App->setSetting("autoscale-y-slave", toggled);
    });
    autoscaleYSlaveAct->setChecked(autoscale);
    plot->toggleAutoscale(2 /* x axis */, autoscale);

    autoscaleAllAct  = new QAction("Автомасштабирование по всем осям", this);
    autoscaleAllAct->setIcon(QIcon(":/icons/autoscale-all.png"));
    connect(autoscaleAllAct, &QAction::triggered, [this](){
        plot->autoscale();
    });

    removeLabelsAct  = new QAction("Удалить все подписи", this);
    removeLabelsAct->setIcon(QIcon(":/icons/remove-labels.png"));
    connect(removeLabelsAct, &QAction::triggered, [this](){
        plot->removeLabels();
    });

    previousDescriptorAct = new QAction("Предыдущая запись", this);
    previousDescriptorAct->setIcon(QIcon(":/icons/rminus.png"));
    connect(previousDescriptorAct, SIGNAL(triggered(bool)), SLOT(previousDescriptor()));

    nextDescriptorAct = new QAction("Следущая запись", this);
    nextDescriptorAct->setIcon(QIcon(":/icons/rplus.png"));
    connect(nextDescriptorAct, SIGNAL(triggered(bool)), SLOT(nextDescriptor()));

    arbitraryDescriptorAct = new QAction("Произвольная запись", this);
    arbitraryDescriptorAct->setIcon(QIcon(":/icons/rany.png"));
    arbitraryDescriptorAct->setCheckable(true);
    connect(arbitraryDescriptorAct, SIGNAL(triggered()), SLOT(arbitraryDescriptor()));

    cycleChannelsUpAct = new QAction("Предыдущий канал", this);
    cycleChannelsUpAct->setIcon(QIcon(":/icons/cminus.png"));
    connect(cycleChannelsUpAct, SIGNAL(triggered(bool)), SLOT(cycleChannelsUp()));

    cycleChannelsDownAct = new QAction("Следующий канал", this);
    cycleChannelsDownAct->setIcon(QIcon(":/icons/cplus.png"));
    connect(cycleChannelsDownAct, SIGNAL(triggered(bool)), SLOT(cycleChannelsDown()));


    QToolBar *scaleToolBar = new QToolBar(this);
    scaleToolBar->setOrientation(Qt::Vertical);
    scaleToolBar->addAction(autoscaleXAct);
    scaleToolBar->addAction(autoscaleYAct);
    scaleToolBar->addAction(autoscaleYSlaveAct);
    scaleToolBar->addAction(autoscaleAllAct);
    scaleToolBar->addSeparator();
    scaleToolBar->addAction(removeLabelsAct);
    scaleToolBar->addSeparator();
    scaleToolBar->addAction(previousDescriptorAct);
    scaleToolBar->addAction(nextDescriptorAct);
    scaleToolBar->addAction(arbitraryDescriptorAct);
    scaleToolBar->addAction(cycleChannelsUpAct);
    scaleToolBar->addAction(cycleChannelsDownAct);

    QWidget *plotsWidget = new QWidget(this);
    QGridLayout *plotsLayout = new QGridLayout;
    plotsLayout->addWidget(scaleToolBar,0,0);
    plotsLayout->addWidget(plot,0,1);
    plotsWidget->setLayout(plotsLayout);

    splitter = new QSplitter(Qt::Vertical, this);
    splitter->addWidget(tabWidget);
    splitter->addWidget(plotsWidget);

    QByteArray mainSplitterState = App->getSetting("mainSplitterState").toByteArray();
    if (!mainSplitterState.isEmpty())
        splitter->restoreState(mainSplitterState);

    setCentralWidget(splitter);



    QVariantMap v = App->getSetting("folders1").toMap();

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

    updateActions();
}

void MainWindow::createTab(const QString &name, const QStringList &folders)
{DD;
    tab = new Tab(this);
    tab->setOrientation(Qt::Horizontal);

    tab->model = new Model(tab);
    connect(tab->model, &Model::needAddFiles, this, &MainWindow::addFiles);
    tab->sortModel = new SortFilterModel(tab);
    tab->sortModel->setSourceModel(tab->model);

    tab->channelModel = new ChannelTableModel(tab);
    connect(tab->channelModel, SIGNAL(modelChanged()), SLOT(updateActions()));
    connect(tab->channelModel,SIGNAL(maybeUpdateChannelDescription(int,QString)),
            SLOT(onChannelDescriptionChanged(int,QString)));
    connect(tab->channelModel,SIGNAL(maybeUpdateChannelName(int,QString)),
            SLOT(onChannelNameChanged(int,QString)));
    connect(tab->channelModel,SIGNAL(maybePlot(int)),SLOT(plotChannel(int)));
    connect(tab->channelModel,SIGNAL(deleteCurve(int)),SLOT(deleteCurve(int)));

    tab->filesTable = new FilesTable(this);
    //connect(tab->filesTable, &FilesTable::addFiles, this, &MainWindow::addFiles);
    tab->filesTable->setModel(tab->sortModel);

    tab->filesTable->setRootIsDecorated(false);
    tab->filesTable->setSortingEnabled(true);
    tab->filesTable->sortByColumn(MODEL_COLUMN_INDEX, Qt::AscendingOrder);

    tab->filesTable->setContextMenuPolicy(Qt::CustomContextMenu);
    tab->filesTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    tab->filesTable->addAction(delFilesAct);
    tab->filesTable->addAction(saveAct);

    connect(tab->filesTable, &QTreeView::customContextMenuRequested, [=](){
        QMenu menu(tab->filesTable);
        int column = tab->filesTable->currentIndex().column();
        if (!tab->filesTable->selectionModel()->hasSelection()) {
            menu.addAction(addFolderAct);
            menu.addAction(addFileAct);
            menu.exec(QCursor::pos());
        }
        else if (column == MODEL_COLUMN_FILENAME) {
            menu.addAction(addFolderAct);
            menu.addAction(addFileAct);
            menu.addAction(delFilesAct);
            menu.addAction(plotAllChannelsAct);
            menu.addAction(plotAllChannelsOnRightAct);
            menu.addAction(calculateSpectreAct);
            menu.addAction(convertAct);
            menu.addAction(renameAct);
            menu.exec(QCursor::pos());
        }
        else if (column == MODEL_COLUMN_LEGEND) {
            //legend
            menu.addAction(copyToLegendAct);
            menu.exec(QCursor::pos());
        }
    });


    tab->filterHeader = new FilteredHeaderView(Qt::Horizontal, tab->filesTable);
    tab->filesTable->setHeader(tab->filterHeader);
    connect(tab->filterHeader, SIGNAL(filterChanged(QString,int)), tab->sortModel, SLOT(setFilter(QString,int)));
//    tab->filterHeader->setFilterBoxes(tab->model->columnCount());

    tab->filesTable->header()->setStretchLastSection(false);
    tab->filesTable->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    tab->filesTable->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    tab->filesTable->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    tab->filesTable->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    tab->filesTable->header()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    tab->filesTable->header()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    tab->filesTable->header()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    tab->filesTable->header()->setSectionResizeMode(7, QHeaderView::ResizeToContents);


    connect(tab->filesTable->selectionModel(),SIGNAL(selectionChanged(QItemSelection,QItemSelection)),tab, SLOT(filesSelectionChanged(QItemSelection,QItemSelection)));
    connect(tab->filesTable->selectionModel(),SIGNAL(currentChanged(QModelIndex,QModelIndex)),SLOT(updateChannelsTable(QModelIndex,QModelIndex)));

    tab->filesTable->setItemDelegateForColumn(MODEL_COLUMN_XSTEP, new StepItemDelegate);

    connect(tab->model, SIGNAL(legendsChanged()), plot, SLOT(updateLegends()));
    connect(tab->model, SIGNAL(plotNeedsUpdate()), plot, SLOT(update()));
    connect(tab->model, SIGNAL(modelChanged()), SLOT(updateActions()));

    tab->channelsTable = new ChannelsTable(this);
    tab->channelsTable->setModel(tab->channelModel);
    tab->channelsTable->setDragEnabled(true);
    tab->channelsTable->setDragDropMode(QAbstractItemView::DragOnly);
   // tab->channelsTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
//    tab->channelsTable->setItemDelegateForColumn(1, new HtmlDelegate);
    connect(tab->channelsTable->selectionModel(),SIGNAL(selectionChanged(QItemSelection,QItemSelection)),tab, SLOT(channelsSelectionChanged(QItemSelection,QItemSelection)));



    tab->tableHeader = new HeaderView(Qt::Horizontal, tab->channelsTable);
    tab->channelsTable->setHorizontalHeader(tab->tableHeader);
    tab->tableHeader->setCheckable(0,true);

    tab->channelsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    tab->channelsTable->horizontalHeader()->setStretchLastSection(false);

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
        else if (column == 0) {
            menu.addAction(plotSelectedChannelsAct);
            menu.addAction(plotSelectedChannelsOnRightAct);
            if (tab->record && tab->record->isSourceFile())
                menu.addAction(exportChannelsToWavAct);
            menu.addAction(moveChannelsUpAct);
            menu.addAction(moveChannelsDownAct);
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
    tab->filePathLabel->setSizePolicy(QSizePolicy::Expanding, tab->filePathLabel->sizePolicy().verticalPolicy());




    QWidget *treeWidget = new QWidget(this);
    QVBoxLayout *treeLayout = new QVBoxLayout;
    treeLayout->addWidget(tab->filesTable);

    treeWidget->setLayout(treeLayout);

    QAction *openFolderAct = new QAction("Открыть папку с этой записью", this);
    openFolderAct->setIcon(QIcon(":/icons/open.png"));
    connect(openFolderAct, &QAction::triggered, [=](){
        if (!tab->filePathLabel->text().isEmpty()) {
            QDir dir(tab->filePathLabel->text());
            dir.cdUp();
            QProcess::startDetached("explorer.exe", QStringList(dir.toNativeSeparators(dir.absolutePath())));
        }
    });

    QAction *editFileAct = new QAction("Редактировать этот файл в текстовом редакторе", this);
    editFileAct->setIcon(QIcon(":/icons/edit.png"));
    connect(editFileAct, &QAction::triggered, [=](){
        QString file = QDir::toNativeSeparators(tab->filePathLabel->text());
        if (!file.isEmpty()) {
            QString executable = App->getSetting("editor").toString();
            if (executable.isEmpty())
                executable = QInputDialog::getText(this, "Текстовый редактор не задан",
                                                   "Введите путь к текстовому редактору,\n"
                                                   "название исполняемого файла или команду для выполнения");

            if (!executable.isEmpty()) {
                if (!QProcess::startDetached(executable, QStringList()<<file)) {
                    executable = QStandardPaths::findExecutable(executable);
                    if (!executable.isEmpty())
                        if (QProcess::startDetached(executable, QStringList()<<file))
                            App->setSetting("editor", executable);
                }
                else
                    App->setSetting("editor", executable);
            }
        }
    });

    QToolBar *channelsToolBar = new QToolBar(this);
    channelsToolBar->setFloatable(false);
    channelsToolBar->setMovable(false);
    channelsToolBar->setIconSize(QSize(16,16));
    channelsToolBar->setContentsMargins(0,0,0,0);
    channelsToolBar->addWidget(tab->filePathLabel);
    channelsToolBar->addAction(editFileAct);
    channelsToolBar->addAction(openFolderAct);

    QWidget *channelsWidget = new QWidget(this);
    channelsWidget->setContentsMargins(0,0,0,0);
    QGridLayout *channelsLayout = new QGridLayout;
    channelsLayout->addWidget(channelsToolBar,0,0,1,3);
    channelsLayout->addWidget(tab->channelsTable,1,0,1,3);
    channelsWidget->setLayout(channelsLayout);


    tab->addWidget(treeWidget);
    tab->addWidget(channelsWidget);



    QByteArray upperSplitterState = App->getSetting("upperSplitterState").toByteArray();
    if (!upperSplitterState.isEmpty())
        tab->restoreState(upperSplitterState);

    int i = tabWidget->addTab(tab, name);
    tabWidget->setCurrentIndex(i);

    for (QString folder: folders) {
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
    //if (tabWidget->count()==1) return;
    int index=i;
    if (i<0) index = tabWidget->currentIndex();

    tabWidget->setCurrentIndex(index);
    //tab now points to current tab

    if (!tab) return;

    // удаление графиков тех файлов, которые были в закрываемой вкладке
    for (int i=0; i<tab->model->size(); ++i) {
        auto f = tab->model->file(i);
    }


    for (int i=plot->curvesCount()-1; i>=0; --i) {
        FileDescriptor *f = plot->curves[i]->channel->descriptor();
        F sm = App->find(f->fileName());
        if (sm.use_count()<=2)
        if (tab->model->contains(f) && !duplicated(f))
            plot->deleteCurve(plot->curves[i]);
    }

    tab->channelModel->clear();
    tab->model->clear();

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
    updateActions();
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

//    plot->deleteAllCurves();

//    for (int i= tabWidget->count()-1; i>=0; --i) {
//        closeTab(i);
//    }

//    ColorSelector::instance()->drop();
}

void MainWindow::addFolder() /*SLOT*/
{DD;
    QString directory = App->getSetting("lastDirectory").toString();

    directory = QFileDialog::getExistingDirectory(this,
                                                  tr("Добавление папки"),
                                                  directory,
                                                  QFileDialog::ShowDirsOnly | QFileDialog::ReadOnly);

    if (directory.isEmpty()) return;
    App->setSetting("lastDirectory", directory);
    addFolder(directory, false /*with subfolders*/, false /*silent*/);
}

void MainWindow::addFolderWithSubfolders() /*SLOT*/
{
    QString directory = App->getSetting("lastDirectory").toString();

    directory = QFileDialog::getExistingDirectory(this,
                                                  tr("Добавление папки со всеми вложенными папками"),
                                                  directory,
                                                  QFileDialog::ShowDirsOnly | QFileDialog::ReadOnly);

    if (directory.isEmpty()) return;
    App->setSetting("lastDirectory", directory);
    addFolder(directory, true /*with subfolders*/, false /*silent*/);
}

void MainWindow::addFile()
{
    QString directory = App->getSetting("lastDirectory").toString();

    QFileDialog dialog(this, "Добавить файлы", directory);
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);
    QString defaultSuffix = QFileInfo(directory).suffix();
    if (defaultSuffix.isEmpty()) defaultSuffix = "dfd";
    dialog.setDefaultSuffix(defaultSuffix);
    QSortFilterProxyModel *proxy = new DfdFilterProxy(0, this);
    dialog.setProxyModel(proxy);
    dialog.setFileMode(QFileDialog::ExistingFiles);


    QStringList fileNames;
    if (dialog.exec()) {
        fileNames = dialog.selectedFiles();
    }
    if (fileNames.isEmpty()) return;

    App->setSetting("lastDirectory", fileNames.constFirst());
    addFiles(fileNames);
    for (const QString &file: fileNames)
        if (!tab->folders.contains(file)) tab->folders << file;
}

void MainWindow::addFolder(const QString &directory, bool withAllSubfolders, bool silent)
{DD;
    if (directory.isEmpty() || !tab) return;

    QStringList filesToAdd;
    processDir(directory, filesToAdd, withAllSubfolders);

    QStringList toAdd;
    for (const QString &file: filesToAdd) {
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

    QList<FileDescriptor *> files = tab->model->selectedFiles();
    for (FileDescriptor *d: files) {
        // удаление графиков удаляемых файлов, если они только в одной вкладке
        if (!duplicated(d))
            plot->deleteCurvesForDescriptor(d);

        if (tab->folders.contains(d->fileName()))
            tab->folders.removeOne(d->fileName());
        else if (tab->folders.contains(d->fileName()+":0"))
            tab->folders.removeOne(d->fileName()+":0");
        else if (tab->folders.contains(d->fileName()+":1"))
            tab->folders.removeOne(d->fileName()+":1");
    }
    tab->channelModel->clear();

    tab->model->deleteFiles();

    if (tab->model->size() == 0)
        tab->folders.clear();
}

void MainWindow::deleteChannels() /** SLOT */
{DD;
    QVector<int> channelsToDelete = tab->channelModel->selected();
    if (channelsToDelete.isEmpty()) return;

    if (QMessageBox::question(this,"DeepSea Base",
                              QString("Выделенные %1 каналов будут \nудалены из записи. Продолжить?").arg(channelsToDelete.size())
                              )==QMessageBox::Yes) {
        LongOperation op;
        deleteChannels(tab->record, channelsToDelete);
        updateChannelsTable(tab->record);
    }
}

void MainWindow::deleteChannelsBatch()
{
    QVector<int> channels = tab->channelModel->selected();
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
    for (FileDescriptor *d: filesToDelete) {
        if (d->channelsCount()<=channels.last()) {
            if (QMessageBox::question(this,"DeepSea Base","В некоторых записях меньше каналов, чем заявлено\n"
                                      "к удалению. Продолжить?")==QMessageBox::Yes)
                break;
            else
                return;
        }
    }

    LongOperation op;
    for (FileDescriptor *d: filesToDelete)
        deleteChannels(d, channels);

    updateChannelsTable(tab->record);
}

void MainWindow::copyChannels() /** SLOT */
{DD;
    QVector<int> channelsToCopy = tab->channelModel->selected();
    if (channelsToCopy.isEmpty()) return;

    if (copyChannels(tab->record, channelsToCopy)) {
        updateChannelsTable(tab->record);
    }
}

void MainWindow::moveChannels() /** SLOT */
{DD;
    // сначала копируем каналы, затем удаляем
    QVector<int> channelsToMove = tab->channelModel->selected();
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
    if (plot->hasCurves()) {
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
        QMessageBox::critical(this, "DeepSea Database",
                              "Не удалось открыть текущую рабочую книгу Excel");
        return;
    }
    QAxObject * worksheet = workbook->querySubObject("ActiveSheet");
    if (!worksheet) {
        QMessageBox::critical(this, "DeepSea Database",
                              "Не удалось открыть текущий рабочий лист Excel");
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
            FileDescriptor * dfd = App->find(fileName).get();
            bool deleteAfter=false;
            if (!dfd) {
                dfd = FormatFactory::createDescriptor(fileName);
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
                    dfd->channel(row-2)->setChanged(true);
                    dfd->channel(row-2)->setDataChanged(true);
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
        plot->deleteCurveForChannelIndex(file, channelsToDelete.at(i), false);
    plot->deleteCurveForChannelIndex(file, channelsToDelete.last(), true);

    LongOperation op;
    file->deleteChannels(channelsToDelete);

    return true;
}

bool MainWindow::copyChannels(FileDescriptor *source, const QVector<int> &channelsToCopy)
{DD;
    QString startFile = App->getSetting("startDir").toString();
    QStringList filters = FormatFactory::allFilters();

    QFileDialog dialog(this, "Выбор файла для записи каналов", startFile,
                       filters.join(";;"));
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);

    QStringList suffixes = FormatFactory::allSuffixes(true);


    // если файл - dfd, то мы можем записать его только в dfd файл с таким же типом данных и шагом по x
    // поэтому если они различаются, то насильно записываем каналы в файл uff

    // мы не можем записывать каналы с разным типом/шагом в один файл dfd,
    //поэтому добавляем фильтр
    QSortFilterProxyModel *proxy = new DfdFilterProxy(source, this);
    dialog.setProxyModel(proxy);

    dialog.setFileMode(QFileDialog::AnyFile);


    QStringList selectedFiles;
    QString selectedFilter;
    if (dialog.exec()) {
        selectedFiles = dialog.selectedFiles();
        selectedFilter = dialog.selectedNameFilter();
    }
    if (selectedFiles.isEmpty()) return false;

    QString file = selectedFiles.constFirst();

    QString currentSuffix = QFileInfo(file).suffix().toLower();
    QString filterSuffix = suffixes.at(filters.indexOf(selectedFilter));

    if (currentSuffix != filterSuffix) {
        //удаляем суффикс, если это суффикс известного нам типа файлов
        if (suffixes.contains(currentSuffix))
            file.chop(currentSuffix.length()+1);
        file.append(QString(".%1").arg(filterSuffix));
    }
    App->setSetting("startDir", file);


    LongOperation op;

    // ИЩЕМ ЭТОТ ФАЙЛ СРЕДИ ДОБАВЛЕННЫХ В БАЗУ

    F destination = App->find(file);
    const bool found = destination.get() != nullptr;
    const bool exists = QFile::exists(file);

    bool isNew = false;
    if (!exists) {//такого файла не существует
        destination = App->addFile(*source, file, channelsToCopy, &isNew);
    }
    else {
        //файл существует, два варианта:
        if (found) {//уже добавлен в базу
            //записываем все изменения данных, если они были
            destination->write();
            destination->writeRawFile();
        }
        else {//еще не добавлен в базу
            destination = App->addFile(file, &isNew);
            destination->read();
        }
        destination->copyChannelsFrom(source, channelsToCopy);
    }

    if (!destination) {
        qDebug()<<"Неизвестный тип файла"<< file;
        return false;
    }

    if (found) {
        tab->model->updateFile(destination.get());
    }
    else {
        addFile(destination);
        if (!tab->folders.contains(file)) tab->folders << file;
    }

    return true;
}

void MainWindow::calculateMean()
{DD;
    if (plot->curves.size()<2) return;

    QList<Channel*> channels;

    bool dataTypeEqual = true;
    bool stepsEqual = true; // одинаковый шаг по оси Х
    bool namesEqual = true; // одинаковые названия осей Y
    bool oneFile = true; // каналы из одного файла
    bool writeToSeparateFile = true;
    bool writeToUff = false;

    Channel *firstChannel = plot->curves.first()->channel;
    channels.append(firstChannel);
    FileDescriptor *firstDescriptor = firstChannel->descriptor();

    bool allFilesDfd = firstDescriptor->fileName().toLower().endsWith("dfd");
    auto firstChannelFileName = firstDescriptor->fileName();

    for (int i = 1; i<plot->curves.size(); ++i) {
        Curve *curve = plot->curves.at(i);
        channels.append(curve->channel);

        allFilesDfd &= firstChannelFileName.toLower().endsWith("dfd");
        if (firstChannel->data()->xStep() != curve->channel->data()->xStep())
            stepsEqual = false;
        if (firstChannel->yName() != curve->channel->yName())
            namesEqual = false;
        if (firstChannelFileName != curve->channel->descriptor()->fileName())
            oneFile = false;
        if (!firstDescriptor->dataTypeEquals(curve->channel->descriptor()))
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
        QMessageBox box("Среднее графиков",
                        QString("Графики взяты из одной записи %1.\n").arg(firstChannelFileName)
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

    QString meanFileName;
    F meanFile;

    if (writeToSeparateFile) {
        QString meanD = firstChannelFileName;
        meanD.chop(4);
        if (writeToUff) meanD.append(".uff");

        meanD = App->getSetting(writeToUff?"lastMeanUffFile":"lastMeanFile", meanD).toString();

        QStringList  filters = FormatFactory::allFilters();
        QStringList suffixes = FormatFactory::allSuffixes(true);

        QFileDialog dialog(this, "Выбор файла для записи каналов", meanD,
                           filters.join(";;"));
        dialog.setOption(QFileDialog::DontUseNativeDialog, true);
        dialog.setFileMode(QFileDialog::AnyFile);
//        dialog.setDefaultSuffix(writeToUff?"uff":"dfd");

        if (!writeToUff) {
            QSortFilterProxyModel *proxy = new DfdFilterProxy(firstDescriptor, this);
            dialog.setProxyModel(proxy);
        }

        QStringList selectedFiles;
        QString selectedFilter;
        if (dialog.exec()) {
            selectedFiles = dialog.selectedFiles();
            selectedFilter = dialog.selectedNameFilter();
        }
        if (selectedFiles.isEmpty()) return;

        meanFileName = selectedFiles.constFirst();
        if (meanFileName.isEmpty()) return;

        QString currentSuffix = QFileInfo(meanFileName).suffix().toLower();
        QString filterSuffix = suffixes.at(filters.indexOf(selectedFilter));

        if (currentSuffix != filterSuffix) {
            //удаляем суффикс, если это суффикс известного нам типа файлов
            if (suffixes.contains(currentSuffix))
                meanFileName.chop(currentSuffix.length()+1);

            meanFileName.append(QString(".%1").arg(filterSuffix));
        }

        bool isNew = false;
        const bool fileExists = QFile::exists(meanFileName);
        meanFile = App->addFile(meanFileName, &isNew);
        if (!meanFile) {
            QMessageBox::critical(this, "Расчет среднего", "Не удалось создать файл неизвестного типа:"
                                  + meanFileName);
            qDebug()<<"Не удалось создать файл"<<meanFileName;
            return;
        }

        if (fileExists) {
            if (isNew) meanFile->read();
        }
        else
            meanFile->fillPreliminary(firstDescriptor);

        App->setSetting(writeToUff?"lastMeanUffFile":"lastMeanFile", meanFileName);
    }
    else {
        meanFileName = firstChannelFileName;
        meanFile = App->find(meanFileName);
    }

    meanFile->calculateMean(channels);

    int idx;
    if (tab->model->contains(meanFile, &idx)) {
        tab->model->updateFile(idx);
    }
    else {
        addFile(meanFile);
    }
    setCurrentAndPlot(meanFile.get(), meanFile->channelsCount()-1);
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

void MainWindow::convertTDMSFiles()
{
    TDMSConverterDialog dialog(this);
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
        converter->deleteLater();
    });
    connect(converter, SIGNAL(tick(int)), progress, SLOT(setValue(int)));

    progress->show();
    progress->setValue(0);

    thread->start();
}

void MainWindow::editYName()
{
    if (!tab) return;

    QVector<int>  selectedIndexes = tab->channelModel->selected();
    if (selectedIndexes.isEmpty()) return;

    QString newYName = QInputDialog::getText(this, "Новая единица измерения", "Введите новую единицу");
    if (newYName.isEmpty()) return;

    tab->channelModel->setYName(newYName);
}

void MainWindow::onChannelDescriptionChanged(int index, const QString &value)
{
    if (!tab) return;
    if (tab->model->selected().size()>1) {
        if (QMessageBox::question(this,"DeepSea Base","Выделено несколько файлов. Записать такое описание канала\n"
                                  "во все эти файлы?")==QMessageBox::Yes)
        {
            tab->model->setChannelDescription(index, value);
        }
        else {
            tab->model->updateFile(tab->record);
        }
    }

}

void MainWindow::onChannelNameChanged(int index, const QString &value)
{
    if (!tab) return;
    if (tab->model->selected().size()>1) {
        if (QMessageBox::question(this,"DeepSea Base","Выделено несколько файлов. Записать такое название канала\n"
                                  "во все эти файлы?")==QMessageBox::Yes)
        {
            tab->model->setChannelName(index, value);
        }
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
    QVector<int> selectedIndexes = tab->channelModel->selected();

    const QVector<int> newIndexes = computeIndexes(selectedIndexes, up, tab->record->channelsCount());

    tab->record->move(up, selectedIndexes, newIndexes);

    updateChannelsTable(tab->record);

    tab->channelsTable->clearSelection();
    for (int i: newIndexes)
        tab->channelsTable->selectionModel()->select(tab->channelModel->index(i,0),QItemSelectionModel::Select);
}

void MainWindow::updateChannelsTable(const QModelIndex &current, const QModelIndex &previous)
{DD;
    if (!tab || !current.isValid()) return;
    if (current.model() != tab->sortModel) return;

    if (previous.isValid()) {
        if ((previous.row()==0 && previous.column() != current.column()) ||
            (previous.row() != current.row())) {
            QModelIndex index = tab->sortModel->mapToSource(current);
            updateChannelsTable(tab->model->file(index.row()).get());
        }
    }
    else {
        QModelIndex index = tab->sortModel->mapToSource(current);
        updateChannelsTable(tab->model->file(index.row()).get());
    }



}

void MainWindow::updateChannelsTable(FileDescriptor *descriptor)
{DD;
    QVector<int> plottedChannels = plottedChannelsNumbers;
    if (sergeiMode) {
    //    plottedChannelsNumbers = tab->channelModel->plotted();
        tab->channelModel->deleteCurves();
    }
    //возвращаем после удаления
    plottedChannelsNumbers = plottedChannels;

    tab->record = descriptor;
    tab->channelModel->setDescriptor(descriptor);
    if (!descriptor) return;

    if (!descriptor->fileExists()) {
        QMessageBox::warning(this,"Не могу получить список каналов","Такого файла уже нет");
        return;
    }

    tab->filePathLabel->setText(descriptor->fileName());

    if (descriptor->channelsCount() == 0) return;

    if (sergeiMode) {
        if (!plottedChannelsNumbers.isEmpty()) {
            tab->channelModel->plotChannels(plottedChannelsNumbers);
        }
    }
    //возвращаем после возможного неполного рисования
    plottedChannelsNumbers = plottedChannels;
}

void MainWindow::plotAllChannels()
{DD;
    if (!tab) return;
    tab->channelModel->plotChannels(QVector<int>(), false);
}

void MainWindow::plotAllChannelsAtRight()
{DD;
    if (!tab) return;
    tab->channelModel->plotChannels(QVector<int>(), true);
}

void MainWindow::plotChannel(int index)
{DD;
    QColor col;
    int idx;
    tab->model->contains(tab->record, &idx);

    bool plotOnRight = tab->record->channel(index)->plotted()==2;
    bool plotted = plot->plotCurve(tab->record->channel(index), &col, plotOnRight, idx+1);

    if (plotted) {
        tab->record->channel(index)->setColor(col);
        tab->record->channel(index)->setPlotted(plotOnRight?2:1);
    }
    else {
        tab->record->channel(index)->setColor(QColor());
        tab->record->channel(index)->setPlotted(0);
    }
    tab->channelModel->onCurveChanged(tab->record->channel(index));
    tab->model->updateFile(tab->record, MODEL_COLUMN_FILENAME);

    if (tab->model->selected().size()>1 && QApplication::keyboardModifiers() & Qt::ControlModifier) {
        QList<FileDescriptor*> selectedFiles = tab->model->selectedFiles();
        foreach (FileDescriptor *f, selectedFiles) {
            if (f == tab->record) continue;
            if (f->channelsCount()<=index) continue;

            tab->model->contains(f, &idx);
            plotted = plot->plotCurve(f->channel(index), &col, plotOnRight, idx+1);
            if (plotted) {
                f->channel(index)->setColor(col);
                f->channel(index)->setPlotted(plotOnRight?2:1);
            }
            else {
                f->channel(index)->setPlotted(0);
                f->channel(index)->setColor(QColor());
            }
            tab->model->updateFile(f, MODEL_COLUMN_FILENAME);
        }
    }
}

void MainWindow::calculateSpectreRecords()
{DD;
    if (!tab) return;

    QList<FileDescriptor *> records = tab->model->selectedFiles(Descriptor::TimeResponse);

    if (records.isEmpty()) {
        QMessageBox::warning(this,QString("DeepSea Base"),
                             QString("Не выделено ни одного файла с исходными временными данными"));
        return;
    }

    CalculateSpectreDialog dialog(records, this);
    //FilesProcessorDialog dialog(records, this);

    if (dialog.exec()) {
        const QStringList newFiles = dialog.getNewFiles();
        addFiles(newFiles);
        for (const QString &file: newFiles)
            if (!tab->folders.contains(file)) tab->folders << file;
    }
}

void MainWindow::convertFiles()
{
    if (!tab) return;
    QList<FileDescriptor *> records = tab->model->selectedFiles();

    ConverterDialog dialog(records, this);
    dialog.exec();

    if (dialog.addFiles()) {
        QStringList newFiles = dialog.getConvertedFiles();
        addFiles(newFiles);
        foreach (const QString &file, newFiles)
            if (!tab->folders.contains(file)) tab->folders << file;
    }
}

void MainWindow::copyToLegend()
{
    if (!tab) return;
    const QList<FileDescriptor *> records = tab->model->selectedFiles();

    for (FileDescriptor *f: records) {
        f->setLegend(QFileInfo(f->fileName()).completeBaseName());
        tab->model->updateFile(f, MODEL_COLUMN_LEGEND);
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

    QStringList toAdd;
    for (FileDescriptor *fd: records) {
        QString dfd = fd->calculateThirdOctave();
        if (auto f = App->find(dfd))
            tab->model->updateFile(f.get());
        else
           toAdd << dfd;
    }
    addFiles(toAdd);
}

void MainWindow::calculateMovingAvg()
{
    if (plot->curves.size()<1) return;

    int windowSize = App->getSetting("movingAvgSize",3).toInt();
    bool ok;
    windowSize = QInputDialog::getInt(this,"Скользящее среднее","Выберите величину окна усреднения",windowSize,
                                      3,15,2,&ok);
    if (ok)
        App->setSetting("movingAvgSize",windowSize);
    else
        return;

    QList<Channel*> channels;

    bool oneFile = true; // каналы из одного файла
    bool writeToSeparateFile = true;

    Curve *firstCurve = plot->curves.constFirst();
    channels.append(firstCurve->channel);
    const QString firstName = firstCurve->channel->descriptor()->fileName();

    bool allFilesDfd = firstName.toLower().endsWith("dfd") ||
                       firstName.toLower().endsWith("d94");

    for (int i = 1; i<plot->curves.size(); ++i) {
        Curve *curve = plot->curves.at(i);
        channels.append(curve->channel);
        const QString curveName = curve->channel->descriptor()->fileName();

        allFilesDfd &= (curveName.toLower().endsWith("dfd") ||
                        curveName.toLower().endsWith("d94"));
        if (firstName != curveName)
            oneFile = false;
    }
    if (oneFile) {
        QMessageBox box("Скользящее среднее каналов", QString("Каналы взяты из одной записи %1.\n").arg(firstName)
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

    QString avgFileName;
    F avg;

    if (writeToSeparateFile) {
        QString avgD = firstName;
        avgD.chop(4);

        avgD = App->getSetting("lastMovingAvgFile", avgD).toString();

        QStringList filters = FormatFactory::allFilters();
        QStringList suffixes = FormatFactory::allSuffixes(true);

        QFileDialog dialog(this, "Выбор файла для записи каналов", avgD,
                           filters.join(";;"));
        dialog.setOption(QFileDialog::DontUseNativeDialog, true);
        dialog.setFileMode(QFileDialog::AnyFile);

        if (allFilesDfd) {
            QSortFilterProxyModel *proxy = new DfdFilterProxy(firstCurve->channel->descriptor(), this);
            dialog.setProxyModel(proxy);
        }

        QString selectedFilter;
        QStringList selectedFiles;
        if (dialog.exec()) {
            selectedFiles = dialog.selectedFiles();
            selectedFilter = dialog.selectedNameFilter();
        }
        if (selectedFiles.isEmpty()) return;

        avgFileName = selectedFiles.constFirst();
        if (avgFileName.isEmpty()) return;
        App->setSetting("lastMovingAvgFile", avgFileName);

        //добавляем суффикс
        QString currentSuffix = QFileInfo(avgFileName).suffix().toLower();
        QString filterSuffix = suffixes.at(filters.indexOf(selectedFilter));
        if (currentSuffix != filterSuffix) {
            //удаляем суффикс, если это суффикс известного нам типа файлов
            if (suffixes.contains(currentSuffix))
                avgFileName.chop(currentSuffix.length()+1);

            avgFileName.append(QString(".%1").arg(filterSuffix));
        }

        bool isNew = false;
        avg = App->addFile(avgFileName, &isNew);

        if (!avg) {
            qDebug() << "Не удалось создать файл" << avgFileName;
            return;
        }

        if (QFileInfo(avgFileName).exists()) {
            if (isNew) avg->read();
        }
        else
            avg->fillPreliminary(firstCurve->channel->descriptor());
    }
    else {
        avgFileName = firstCurve->channel->descriptor()->fileName();
        avg = App->find(avgFileName);
    }

    avg->calculateMovingAvg(channels,windowSize);

//    avgFile->setChanged(true);
//    avgFile->setDataChanged(true);
//    avgFile->write();
//    avgFile->writeRawFile();

    int idx;
    if (tab->model->contains(avgFileName, &idx)) {
        tab->model->updateFile(idx);
    }
    else {
        addFile(avg);
    }
    setCurrentAndPlot(avg.get(), avg->channelsCount()-1);
}

void MainWindow::deleteCurve(int index)
{DD;
    //не удаляем, если фиксирована
    if (Curve *c = plot->plotted(tab->record->channel(index))) {
        if (c->fixed) return;
    }

    //удаляем кривую с графика
    plot->deleteCurveForChannelIndex(tab->record, index);
    //обновляем инфу для канала
    tab->record->channel(index)->setPlotted(0);
    tab->record->channel(index)->setColor(QColor());
    //обновляем модель для канала
    tab->channelModel->onCurveChanged(tab->record->channel(index));
    //обновляем модель для файла
    tab->model->updateFile(tab->record, MODEL_COLUMN_FILENAME);

    if (tab->model->selected().size()>1 && QApplication::keyboardModifiers() & Qt::ControlModifier) {
        QList<FileDescriptor*> selectedFiles = tab->model->selectedFiles();
        for (FileDescriptor *f: qAsConst(selectedFiles)) {
            if (f == tab->record) continue;
            if (f->channelsCount()<=index) continue;

            //не удаляем, если фиксирована
            if (Curve *c = plot->plotted(f->channel(index))) {
                if (c->fixed) continue;
            }

            plot->deleteCurveForChannelIndex(f, index);
            f->channel(index)->setPlotted(0);
            f->channel(index)->setColor(QColor());

            tab->model->updateFile(f, MODEL_COLUMN_FILENAME);
        }
    }
    updatePlottedChannelsNumbers();
}

void MainWindow::rescanBase()
{DD;
    if (!tab) return;

    // first we delete all curves affected
    plot->deleteAllCurves();

    // next we clear all tab and populate it with folders anew
    tab->channelModel->clear();
    tab->model->clear();
    tab->filePathLabel->clear();

    for (QString folder: tab->folders) {
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

//QVariant App->getSetting(const QString &key, const QVariant &defValue)
//{DD;
//    if (QFile::exists("portable")) {
//        QSettings se("deepseabase.ini",QSettings::IniFormat);
//        return se.value(key, defValue);
//    }
//    else {
//        QSettings se("Alex Novichkov","DeepSea Database");
//        return se.value(key, defValue);
//    }
//}

//void App->setSetting(const QString &key, const QVariant &value)
//{DD;
//    if (QFile::exists("portable")) {
//        QSettings se("deepseabase.ini",QSettings::IniFormat);
//        se.setValue(key, value);
//    }
//    else {
//        QSettings se("Alex Novichkov","DeepSea Database");
//        se.setValue(key, value);
//    }
//}


void MainWindow::addFile(F descriptor)
{
    if (!tab) return;
    if (!descriptor) return;

    addDescriptors(QList<F>()<<descriptor);
//    tab->filesTable->setCurrentIndex(tab->model->modelIndexOfFile(descriptor, 1));
}

bool MainWindow::duplicated(FileDescriptor *file) const
{
    if (!file) return false;

    for (int i=0; i<tabWidget->count(); ++i) {
        Tab *t = qobject_cast<Tab *>(tabWidget->widget(i));
        if (t==tab) continue;

        if (t->model->contains(file))
            return true;
    }
    return false;
}

void MainWindow::setCurrentAndPlot(FileDescriptor *d, int channelIndex)
{DD;
    if (tab) {
        if (tab->model->contains(d)) {
            updateChannelsTable(d);

            tab->channelModel->plotChannels(QVector<int>()<<channelIndex);
        }
    }
}

void MainWindow::updatePlottedChannelsNumbers()
{
    if (sergeiMode)
        plottedChannelsNumbers = tab->channelModel->plotted();
    else
        plottedChannelsNumbers.clear();
    //    qDebug()<<plottedChannelsNumbers.size();
}

void MainWindow::previousDescriptor()
{
    if (!tab || !tab->record) return;

    //проверяем, есть ли в табе другие записи
    if (tab->model->size() < 2) return;

    bool mode = sergeiMode;
    sergeiMode = true;
    updatePlottedChannelsNumbers();

    QModelIndex current = tab->filesTable->selectionModel()->currentIndex();
    int row = current.row();
    QModelIndex index;
    if (row == 0)
        index = tab->filesTable->model()->index(tab->filesTable->model()->rowCount()-1,current.column());
    else
        index = tab->filesTable->model()->index(row-1,current.column());
    if (index.isValid())
        tab->filesTable->selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Current);
    sergeiMode = mode;
}

void MainWindow::nextDescriptor()
{
    if (!tab || !tab->record) return;

    //проверяем, есть ли в табе другие записи
    if (tab->model->size() < 2) return;

    bool mode = sergeiMode;
    sergeiMode = true;
    updatePlottedChannelsNumbers();

    QModelIndex current = tab->filesTable->selectionModel()->currentIndex();
    int row = current.row();
    QModelIndex index;
    if (row == tab->filesTable->model()->rowCount()-1)
        index = tab->filesTable->model()->index(0,current.column());
    else
        index = tab->filesTable->model()->index(row+1,current.column());
    if (index.isValid())
        tab->filesTable->selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Current);
    sergeiMode = mode;
}

void MainWindow::arbitraryDescriptor()
{
    if (!tab || !tab->record) return;

    //проверяем, есть ли в табе другие записи
    if (tab->model->size() < 2) return;

    sergeiMode = !sergeiMode;
    updatePlottedChannelsNumbers();
}

void MainWindow::cycleChannelsUp()
{
    if (!tab || !tab->record) return;

    bool mode = sergeiMode;
    sergeiMode = true;
    updatePlottedChannelsNumbers();

    QVector<int> plotted = plottedChannelsNumbers;
    if (!plotted.isEmpty()) {
        tab->channelModel->deleteCurves();

        for (int i=0; i<plotted.size(); ++i) {
            //пропускаем фиксированную кривую, остальные сдвигаем
            if (Curve *curve = plot->plotted(tab->record->channel(plotted[i]))) {
                if (curve->fixed) continue;
            }

            if (plotted[i] == 0) plotted[i] = tab->record->channelsCount()-1;
            else plotted[i]=plotted[i]-1;
        }
        tab->channelModel->plotChannels(plotted);
    }
    sergeiMode = mode;
}

void MainWindow::cycleChannelsDown()
{
    if (!tab || !tab->record) return;

    bool mode = sergeiMode;
    sergeiMode = true;
    updatePlottedChannelsNumbers();

    QVector<int> plotted = plottedChannelsNumbers;
    if (!plotted.isEmpty()) {

        tab->channelModel->deleteCurves();

        for (int i=0; i<plotted.size(); ++i) {
            //пропускаем фиксированную кривую, остальные сдвигаем
            if (Curve *curve = plot->plotted(tab->record->channel(plotted[i]))) {
                if (curve->fixed) continue;
            }

            if (plotted[i] == tab->record->channelsCount()-1) plotted[i] = 0;
            else plotted[i]=plotted[i]+1;
        }
        tab->channelModel->plotChannels(plotted);
    }
    sergeiMode = mode;
}

void MainWindow::exportChannelsToWav()
{
    if (!tab || !tab->record) return;
    //проверяем тип файла
    if (!tab->record->isSourceFile()) return;

    QVector<int> toExport = tab->channelModel->selected();
    if (toExport.isEmpty()) return;

    QThread *thread = new QThread;
    WavExporter *exporter = new WavExporter(tab->record, toExport);
    exporter->moveToThread(thread);

    QProgressDialog *progress = new QProgressDialog("Сохранение в WAV...", "Отменить сохранение", 0, exporter->chunksCount(), this);
    progress->setWindowModality(Qt::WindowModal);

    connect(thread, SIGNAL(started()), exporter, SLOT(start()));
    connect(exporter, SIGNAL(finished()), thread, SLOT(quit()));
    connect(exporter, SIGNAL(finished()), progress, SLOT(accept()));
    connect(progress, SIGNAL(canceled()), thread, SLOT(quit()));
    connect(exporter, &WavExporter::finished, [=](){
        exporter->deleteLater();
    });
    connect(exporter, SIGNAL(tick(int)), progress, SLOT(setValue(int)));

    progress->show();
    progress->setValue(0);

    thread->start();
}

void MainWindow::updateActions()
{
    if (!tab || !tab->model ||!tab->channelModel) return;

    const int selectedChannelsCount = tab->channelModel->selected().size();
    const int selectedFilesCount = tab->model->selected().size();

    saveAct->setEnabled(tab->model->changed());

    renameAct->setDisabled(selectedFilesCount==0);
    delFilesAct->setDisabled(selectedFilesCount==0);
    plotAllChannelsAct->setDisabled(selectedFilesCount==0);
    plotAllChannelsOnRightAct->setDisabled(selectedFilesCount==0);
    plotSelectedChannelsAct->setDisabled(selectedChannelsCount==0);
    //exportChannelsToWavAct;
    calculateSpectreAct->setDisabled(tab->model->selectedFiles(Descriptor::TimeResponse).isEmpty());
    const QVector<Descriptor::DataType> types {Descriptor::AutoSpectrum,
                Descriptor::CrossSpectrum,
                Descriptor::AutoCorrelation,
                Descriptor::CrossCorrelation,
                Descriptor::FrequencyResponseFunction,
                Descriptor::PowerSpectralDensity,
                Descriptor::EnergySpectralDensity,
                Descriptor::Spectrum,
                Descriptor::ShockResponseSpectrum};
    calculateThirdOctaveAct->setDisabled(
                tab->model->selectedFiles(types).isEmpty());
    clearPlotAct->setEnabled(plot->hasCurves());
    savePlotAct->setEnabled(plot->hasCurves());
    rescanBaseAct->setEnabled(tab->model->size()>0);
    //QAction *switchCursorAct;
    trackingCursorAct->setEnabled(!plot->spectrogram);
    copyToClipboardAct->setEnabled(plot->hasCurves());
    printPlotAct->setEnabled(plot->hasCurves());
    //QAction *editColorsAct;

    meanAct->setDisabled(plot->curves.size()<2);
    movingAvgAct->setEnabled(plot->hasCurves() && !plot->spectrogram);
    //QAction *interactionModeAct;
    addCorrectionAct->setEnabled(plot->hasCurves() && !plot->spectrogram);
    addCorrectionsAct->setEnabled(plot->hasCurves() && !plot->spectrogram);
    deleteChannelsAct->setDisabled(selectedChannelsCount==0);
    deleteChannelsBatchAct->setDisabled(selectedChannelsCount==0);
    copyChannelsAct->setDisabled(selectedChannelsCount==0);
    moveChannelsAct->setDisabled(selectedChannelsCount==0);

    moveChannelsDownAct->setEnabled(selectedChannelsCount > 0 && selectedChannelsCount < tab->channelModel->channelsCount);
    moveChannelsUpAct->setEnabled(selectedChannelsCount > 0 && selectedChannelsCount < tab->channelModel->channelsCount);
    editDescriptionsAct->setDisabled(selectedFilesCount==0);

    exportToExcelAct->setEnabled(plot->hasCurves() && !plot->spectrogram);

    //convertMatFilesAct;
    //QAction *convertTDMSFilesAct;
    //QAction *convertEsoFilesAct;
    //convertAct;
    copyToLegendAct->setDisabled(selectedFilesCount==0);

    //QAction *autoscaleXAct;
    //QAction *autoscaleYAct;
    //QAction *autoscaleYSlaveAct;
    //QAction *autoscaleAllAct;

    //QAction *removeLabelsAct;
    playAct->setEnabled(plot->curvesCount(Descriptor::TimeResponse)>0);
    editYNameAct->setDisabled(selectedChannelsCount==0);

    previousDescriptorAct->setEnabled(tab->model->size()>1 && plot->hasCurves());
    nextDescriptorAct->setEnabled(tab->model->size()>1 && plot->hasCurves());
    arbitraryDescriptorAct->setEnabled(tab->model->size()>1 && plot->hasCurves());
    cycleChannelsUpAct->setEnabled(tab->channelModel->channelsCount>1 && plot->hasCurves());
    cycleChannelsDownAct->setEnabled(tab->channelModel->channelsCount>1 && plot->hasCurves());
}

void MainWindow::renameDescriptor()
{
    if (!tab) return;
    QList<FileDescriptor *> records = tab->model->selectedFiles();
    if (records.isEmpty()) return;

    FileDescriptor *file = records.constFirst();
    QFileInfo fi(file->fileName());
    QString newName = QInputDialog::getText(this, "Переименование файла",
                                            "Введите новое имя файла",
                                            QLineEdit::Normal,
                                            fi.fileName());
    if (newName.isEmpty() || newName == fi.fileName()) return;

    if (!file->rename(newName)) QMessageBox::warning(this,"DeepSea Base", "Не удалось переименовать файл");
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
    if (list.size()>0) result << descriptionEntryToString(list.constFirst()); else result << "";
    if (list.size()>1) result << descriptionEntryToString(list.at(1)); else result << "";
    return result;
}

void MainWindow::exportToExcel(bool fullRange, bool dataOnly)
{
    static QAxObject *excel = 0;

    if (!plot->hasCurves()) {
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


     Channel *channel = plot->curves.at(0)->channel;
     FileDescriptor *descriptor = channel->descriptor();

     // проверяем, все ли каналы из одного файла
     bool allChannelsFromOneFile = true;
     for (int i=1; i<plot->curves.size(); ++i) {
         if (plot->curves.at(i)->channel->descriptor()->fileName() != descriptor->fileName()) {
             allChannelsFromOneFile = false;
             break;
         }
     }

     //проверяем, все ли каналы имеют одинаковое разрешение по х
     bool allChannelsHaveSameXStep = true;
     for (int i=1; i<plot->curves.size(); ++i) {
         if (!qFuzzyCompare(plot->curves.at(i)->channel->data()->xStep()+1.0,
                            channel->data()->xStep()+1.0)) {
             allChannelsHaveSameXStep = false;
             break;
         }
     }

     //проверяем, все ли каналы имеют одинаковую длину
     bool allChannelsHaveSameLength = true;
     for (int i=1; i<plot->curves.size(); ++i) {
         if (plot->curves.at(i)->channel->samplesCount() != channel->samplesCount()) {
             allChannelsHaveSameLength = false;
             break;
         }
     }

     const int samplesCount = channel->samplesCount();
//     const double step = channel->xStep();
//     const bool zeroStepDetected = step<1e-9;

     const bool writeToSeparateColumns = !allChannelsHaveSameXStep || !allChannelsHaveSameLength;

     double minX = channel->data()->xMin();
     double maxX = channel->data()->xMax();

     Range range = plot->xRange();

     for (int i=1; i<plot->curves.size(); ++i) {
         Channel *ch = plot->curves.at(i)->channel;
         if (ch->data()->xMin() < minX) minX = ch->data()->xMin();
         if (ch->data()->xMax() > maxX) maxX = ch->data()->xMax();
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
         if (cells) cells->setProperty("Value", descriptions.constFirst());

         cells = worksheet->querySubObject("Cells(Int,Int)", 3, 2);
         if (cells) cells->setProperty("Value", descriptions.at(1));

         delete cells;
     }
     else {
         for (int i=0; i<plot->curves.size(); ++i) {
             Curve *curve = plot->curves.at(i);
             QStringList descriptions = twoStringDescription(curve->channel->descriptor()->dataDescriptor());

             QAxObject *cells = !writeToSeparateColumns ? worksheet->querySubObject("Cells(Int,Int)", 1, 2+i)
                                                       : worksheet->querySubObject("Cells(Int,Int)", 1, 2+i*2);
             cells->setProperty("Value", curve->channel->descriptor()->fileName());

             cells = !writeToSeparateColumns ? worksheet->querySubObject("Cells(Int,Int)", 2, 2+i)
                                            : worksheet->querySubObject("Cells(Int,Int)", 2, 2+i*2);
             if (cells) cells->setProperty("Value", descriptions.constFirst());

             cells = !writeToSeparateColumns ? worksheet->querySubObject("Cells(Int,Int)", 3, 2+i)
                                            : worksheet->querySubObject("Cells(Int,Int)", 3, 2+i*2);
             if (cells) cells->setProperty("Value", descriptions.at(1));

             delete cells;
         }
     }

     // записываем название канала
     for (int i=0; i<plot->curves.size(); ++i) {
         Curve *curve = plot->curves.at(i);
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
     // если шаг по х нулевой, то предполагаем октаву, размножаем данные для графика
     if (!writeToSeparateColumns) {
         const int numCols = plot->curves.size();
         const bool zeroStep = qFuzzyIsNull(channel->data()->xStep());

         QList<QVariant> cellsList;
         QList<QVariant> rowsList;
         for (int i = 0; i < samplesCount; ++i) {
             double val = channel->data()->xValue(i);
             if (!fullRange && (val < range.min || val > range.max) ) continue;

             cellsList.clear();

             if (zeroStep && exportPlots) {//размножаем каждое значение на 2
                 double f1 = i==0 ? val/pow(10.0,0.05):sqrt(val*channel->data()->xValue(i-1));
                 double f2 = i==samplesCount-1?val*pow(10.0,0.05):sqrt(val*channel->data()->xValue(i+1));
                 //первый ряд: (f1, Li)
                 cellsList << f1;
                 for (int j = 0; j < numCols; ++j) {
                     cellsList << plot->curves.at(j)->channel->data()->yValue(i);
                 }
                 rowsList << QVariant(cellsList);
                 cellsList.clear();
                 //второй ряд: (f2, Li)
                 cellsList << f2;
                 for (int j = 0; j < numCols; ++j) {
                     cellsList << plot->curves.at(j)->channel->data()->yValue(i);
                 }
                 rowsList << QVariant(cellsList);
                 cellsList.clear();
             }
             else {
                 cellsList << val;
                 for (int j = 0; j < numCols; ++j) {
                     cellsList << plot->curves.at(j)->channel->data()->yValue(i);
                 }
                 rowsList << QVariant(cellsList);
             }
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
         for (int i=0; i<plot->curves.size(); ++i) {
             Curve *curve = plot->curves.at(i);
             Channel *ch = curve->channel;
             bool zeroStep = qFuzzyIsNull(ch->data()->xStep());

             QList<QVariant> cellsList;
             QList<QVariant> rowsList;
             for (int j = 0; j < ch->samplesCount(); j++) {
                 double val = ch->data()->xValue(j);
                 if (!fullRange && (val < range.min || val > range.max) ) continue;

                 cellsList.clear();
                 if (zeroStep && exportPlots) {//размножаем каждое значение на 2
                     double f1 = j==0 ? val/pow(10.0,0.05):sqrt(val*ch->data()->xValue(j-1));
                     double f2 = j==ch->samplesCount()-1?val*pow(10.0,0.05):sqrt(val*ch->data()->xValue(j+1));
                     //первый ряд: (f1, Li)
                     cellsList << f1 << ch->data()->yValue(j);
                     rowsList << QVariant(cellsList);
                     cellsList.clear();
                     //второй ряд: (f2, Li)
                     cellsList << f2 << ch->data()->yValue(j);
                     rowsList << QVariant(cellsList);
                     cellsList.clear();
                 }
                 else {
                     cellsList << val << ch->data()->yValue(j);
                     rowsList << QVariant(cellsList);
                 }
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

             for (int i=0; i<plot->curves.size(); ++i) {
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
             Curve *curve = plot->curves.at(i);
             QAxObject * serie = series->querySubObject("Item (int)", i+1);
             if (serie) {
                 if (curve->yAxis()==QwtAxis::yRight) {
                     serie->setProperty("AxisGroup", 2);
                     addRightAxis = true;
                 }
                 serie->setProperty("Name", curve->channel->name());
             }
             delete serie;
         }
         if (addRightAxis) {
             QAxObject *yAxis = chart->querySubObject("Axes(const QVariant&,const QVariant&)", 2,2);
             if (yAxis) setAxis(yAxis, stripHtml(plot->axisTitle(plot->yRightAxis).text()));
             delete yAxis;
         }


         // добавляем подписи осей
         QAxObject *xAxis = chart->querySubObject("Axes(const QVariant&)", 1);
         if (xAxis) {
             setAxis(xAxis, stripHtml(plot->axisTitle(QwtAxis::xBottom).text()));
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
             setAxis(yAxis, stripHtml(plot->axisTitle(QwtAxis::yLeft).text()));
             yAxis->setProperty("CrossesAt", -1000);
         }
         delete yAxis;

         // рамка вокруг графика
         QAxObject *plotArea = chart->querySubObject("PlotArea");
         if (plotArea) setLineColor(plotArea, 13);
         delete plotArea;

         // цвета графиков
         for (int i = 0; i< plot->curves.size(); ++i) {
             Curve *curve = plot->curves.at(i);
             QAxObject * serie = series->querySubObject("Item(int)", i+1);
             if (serie) {
                 QAxObject *format = serie->querySubObject("Format");
                 QAxObject *formatLine = format->querySubObject("Line");
                 if (formatLine) {
                     formatLine->setProperty("Weight", plot->curves.at(i)->pen().width());
                     Qt::PenStyle lineStyle = plot->curves.at(i)->pen().style();
                     int msoLineStyle = 1;
                     switch (lineStyle) {
                         case Qt::NoPen:
                         case Qt::SolidLine: msoLineStyle = 1; break;
                         case Qt::DashLine: msoLineStyle = 4; break;
                         case Qt::DotLine: msoLineStyle = 3; break;
                         case Qt::DashDotLine: msoLineStyle = 5; break;
                         case Qt::DashDotDotLine: msoLineStyle = 6; break;
                         default: break;
                     }
                     formatLine->setProperty("DashStyle", msoLineStyle);

                     QAxObject *formatLineForeColor = formatLine->querySubObject("ForeColor");
                     QColor color = plot->curves.at(i)->channel->color();
                     //меняем местами красный и синий, потому что Excel неправильно понимает порядок
                     int red = color.red();
                     color.setRed(color.blue());
                     color.setBlue(red);
                     if (formatLineForeColor) formatLineForeColor->setProperty("RGB", color.rgb());
                     delete formatLineForeColor;
                 }

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
    if (tab->record == curve->channel->descriptor()) {
        tab->channelModel->onCurveChanged(curve->channel);
//        updateChannelsTable(curve->channel->descriptor());
    }
}

void MainWindow::onCurveDeleted(Channel *channel)
{DD;
    for (int index = 0; index < tabWidget->count(); ++index) {
        if (Tab *t = qobject_cast<Tab*>(tabWidget->widget(index)))
            t->model->invalidateCurve(channel);
    }
    if (tab->record == channel->descriptor())
        tab->channelModel->onCurveChanged(channel);

    updateActions();
}

void MainWindow::addDescriptors(const QList<F> &files)
{DD;
    if (!tab) return;

    tab->model->addFiles(files);
}

void MainWindow::addFiles(const QStringList &files)
{DD;
    if (files.isEmpty()) return;
    if (!tab) return;
    LongOperation op;

    QList<F> items;

    for (const QString &fileName: files) {
        if (fileName.isEmpty()) continue;

        if (tab->model->contains(fileName))
            continue;

        emit loading(fileName);

        bool isNew = false;
        F file = App->addFile(fileName, &isNew);
        if (file) {
            if (isNew) file->read();
            items << file;
        }
    }
    addDescriptors(items);
}

Tab::~Tab()
{

}

void Tab::filesSelectionChanged(const QItemSelection &newSelection, const QItemSelection &oldSelection)
{
    Q_UNUSED(oldSelection);
    if (newSelection.isEmpty()) filesTable->selectionModel()->setCurrentIndex(QModelIndex(), QItemSelectionModel::NoUpdate);

    QSet<int> indexes;

    QModelIndexList list = filesTable->selectionModel()->selection().indexes();
    foreach (const QModelIndex &i, list) indexes << sortModel->mapToSource(i).row();

    QList<int> l = indexes.values();
    std::sort(l.begin(), l.end());

    model->setSelected(l);
    if (indexes.isEmpty()) {
        channelModel->clear();
        this->filePathLabel->clear();
    }

}

void Tab::channelsSelectionChanged(const QItemSelection &newSelection, const QItemSelection &oldSelection)
{
    Q_UNUSED(oldSelection);
    if (newSelection.isEmpty()) channelsTable->selectionModel()->setCurrentIndex(QModelIndex(), QItemSelectionModel::NoUpdate);

    QSet<int> indexes;

    QModelIndexList list = channelsTable->selectionModel()->selection().indexes();
    foreach (const QModelIndex &i, list) indexes << i.row();

    QList<int> l = indexes.values();
    std::sort(l.begin(), l.end());

    channelModel->setSelected(l.toVector());
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (closeRequested())
        event->accept();
    else
        event->ignore();
}

bool MainWindow::closeRequested()
{
    //определяем, были ли изменения
    bool changed = false;
    for (int i=0; i<tabWidget->count(); ++i) {
        if (Tab *t = qobject_cast<Tab *>(tabWidget->widget(i))) {
            if (t->model->changed()) {
                changed = true;
                break;
            }
        }
    }
    if (changed) {
        //спрашиваем, сохранять ли файлы
        QMessageBox msgBox(QMessageBox::Question,
                           tr("Deepsea Base"),
                           tr("Были изменены некоторые файлы."),
                           QMessageBox::NoButton,
                           this);
        msgBox.setInformativeText(tr("Сохранить изменения?"));
        QPushButton *saveB=msgBox.addButton(tr("Да, сохранить"),QMessageBox::YesRole);
        saveB->setIcon(qApp->style()->standardIcon(QStyle::SP_DialogSaveButton));
        QPushButton *discB=msgBox.addButton(tr("Нет, выйти без сохранения"),QMessageBox::NoRole);
        discB->setIcon(qApp->style()->standardIcon(QStyle::SP_DialogDiscardButton));
        QPushButton *cancB=msgBox.addButton(QMessageBox::Cancel);
        cancB->setIcon(qApp->style()->standardIcon(QStyle::SP_DialogCancelButton));
        msgBox.setDefaultButton(saveB);
        msgBox.setEscapeButton(cancB);
        msgBox.setWindowModality(Qt::WindowModal);
        msgBox.exec();

        if (msgBox.clickedButton() == cancB) return false;
        else {
            if (msgBox.clickedButton() == discB) {
                for (int i=0; i<tabWidget->count(); ++i) {
                    if (Tab *t = qobject_cast<Tab *>(tabWidget->widget(i))) {
                        t->model->discardChanges();
                    }
                }
            }
        }
    }

    // сохранение состояния, сохранение файлов
    App->setSetting("mainSplitterState",splitter->saveState());

    if (tab) {
        App->setSetting("upperSplitterState",tab->saveState());
        QByteArray treeHeaderState = tab->filesTable->header()->saveState();
        App->setSetting("treeHeaderState", treeHeaderState);
    }

    QVariantMap map;
    for (int i=0; i<tabWidget->count(); ++i) {
        if (Tab *t = qobject_cast<Tab *>(tabWidget->widget(i))) {
            if (!t->folders.isEmpty())
                map.insert(tabWidget->tabText(i), t->folders);
            tab->filterHeader->clear();
        }
    }

    App->setSetting("folders1", map);

    //насильственно удаляем все графики
    plot->deleteAllCurves(true);

    for (int i= tabWidget->count()-1; i>=0; --i) {
        closeTab(i);
    }

    return true;
}
