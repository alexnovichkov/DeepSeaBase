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
#include "wavexportdialog.h"
#include "filehandler.h"

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
#include "plot/legend.h"

#include "fileformats/formatfactory.h"
#include "descriptorpropertiesdialog.h"
#include "channelpropertiesdialog.h"
#include "tab.h"
#include "filehandler.h"
#include "filehandlerdialog.h"
#include "plot/plotarea.h"
#include "stepitemdelegate.h"
#include "dfdfilterproxy.h"

#include "DockManager.h"
#include "DockAreaWidget.h"
#include "DockWidget.h"
#include "DockAreaTitleBar.h"
#include "DockAreaTabBar.h"
#include "DockComponentsFactory.h"
#include "DockWidgetTab.h"
#include "customdockfactory.h"
#include "plotdockfactory.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), currentTab(0)
{DD;
    setWindowTitle(tr("DeepSea Database ")+DEEPSEABASE_VERSION);

    setDocumentMode(true);

    ads::CDockManager::setConfigFlags(ads::CDockManager::DefaultNonOpaqueConfig);
    ads::CDockManager::setConfigFlag(ads::CDockManager::DockAreaHasCloseButton, false);
    ads::CDockManager::setConfigFlag(ads::CDockManager::DockAreaHasUndockButton, false);
    ads::CDockManager::setConfigFlag(ads::CDockManager::FocusHighlighting, true);
    ads::CDockManager::setConfigFlag(ads::CDockManager::EqualSplitOnInsertion, true);

    m_DockManager = new ads::CDockManager(this);

    createActions();

    mainToolBar = addToolBar("Панель инструментов");
    mainToolBar->setIconSize(QSize(24,24));
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
    mainToolBar->addAction(copyPlottedChannelsAct);
    mainToolBar->addAction(movePlottedChannelsAct);
    mainToolBar->addAction(deletePlottedChannelsAct);
    mainToolBar->addAction(meanAct);
    mainToolBar->addAction(movingAvgAct);
    mainToolBar->addAction(addCorrectionAct);
    mainToolBar->addAction(moveChannelsUpAct);
    mainToolBar->addAction(moveChannelsDownAct);
    mainToolBar->addAction(editChannelDescriptionsAct);
    mainToolBar->addAction(exportChannelsToWavAct);
    mainToolBar->addAction(exportToExcelAct);
//    mainToolBar->addAction(playAct);
    mainToolBar->addSeparator();
    mainToolBar->addAction(newPlotAct);

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

    plotsMenu = menuBar()->addMenu("Графики");
    plotsMenu->addAction(newPlotAct);

    addPlot();

    QVariantMap v = App->getSetting("folders1").toMap();

    if (v.isEmpty())
        createNewTab();
    else {
        for (const auto [key,val]: asKeyValueRange(v)) {
            createTab(key, val.toStringList());
            tabsNames << key;
        }
    }

    updateActions();
}

void MainWindow::createActions()
{
    QIcon newPlotIcon(":/icons/newPlot.png");
    newPlotIcon.addFile(":/icons/newPlot24.png");
    newPlotAct = new QAction(newPlotIcon, "Добавить график", this);
    connect(newPlotAct, SIGNAL(triggered()), this, SLOT(addPlot()));

    QIcon addFolderIcon(":/icons/open24.png");
    addFolderIcon.addFile(":/icons/open.png");
    addFolderAct = new QAction(addFolderIcon, tr("Добавить папку"),this);
    addFolderAct->setShortcut(tr("Ctrl+O"));
    connect(addFolderAct, SIGNAL(triggered()), SLOT(addFolder()));

    addFolderWithSubfoldersAct = new QAction(tr("Добавить папку со всеми вложенными папками"),this);
    connect(addFolderWithSubfoldersAct, SIGNAL(triggered()), SLOT(addFolderWithSubfolders()));

    QIcon addFileIcon(":/icons/open_file.ico");
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
    connect(plotSelectedChannelsAct, &QAction::triggered,
            this, [=](){if (currentTab) currentTab->channelModel->plotChannels();});

    plotSelectedChannelsOnRightAct = new QAction(QString("...на правой оси"), this);
    connect(plotSelectedChannelsOnRightAct, &QAction::triggered, [=](){
        if (currentTab) currentTab->channelModel->plotChannels(true);
    });

    exportChannelsToWavAct = new QAction(QString("Экспортировать в WAV"), this);
    exportChannelsToWavAct->setIcon(QIcon(":/icons/wav.ico"));
    connect(exportChannelsToWavAct, SIGNAL(triggered(bool)), SLOT(exportChannelsToWav()));

    plotHelpAct = new QAction(QIcon(":/icons/help.png"), "Справка", this);
    connect(plotHelpAct, &QAction::triggered, [](){QDesktopServices::openUrl(QUrl("help.html"));});

    aboutAct = new QAction("О програме", this);
    connect(aboutAct, &QAction::triggered, [=](){
        QString version = QString("DeepSea Database - версия %1\n").arg(DEEPSEABASE_VERSION);
        version.append(sizeof(int*) == sizeof(qint32) ? "32-bit версия" : "64-bit версия");
        QMessageBox::about(this, tr("О программе"), version);
    });

    QIcon calculateSpectreIcon(":/icons/function.png");
    calculateSpectreIcon.addFile(":/icons/function16.png");
    calculateSpectreAct = new QAction(calculateSpectreIcon, QString("Рассчитать спектры..."), this);
    connect(calculateSpectreAct, &QAction::triggered, [=](){
        calculateSpectreRecords();
    });

    calculateSpectreDeepSeaAct = new QAction(calculateSpectreIcon, QString("Рассчитать спектры с помощью Deepsea..."), this);
    connect(calculateSpectreDeepSeaAct, &QAction::triggered, [=](){
        calculateSpectreRecords(true);
    });

    QMenu *calculateSpectreMenu = new QMenu(this);
    calculateSpectreMenu->addAction(calculateSpectreDeepSeaAct);
    calculateSpectreAct->setMenu(calculateSpectreMenu);

    convertAct = new QAction("Конвертировать файлы...", this);
    connect(convertAct, SIGNAL(triggered()), SLOT(convertFiles()));

    copyToLegendAct = new QAction("Перенести сюда названия файлов", this);
    connect(copyToLegendAct, SIGNAL(triggered()), SLOT(copyToLegend()));

    rescanBaseAct = new QAction(QString("Пересканировать базу"), this);
    rescanBaseAct->setIcon(QIcon(":/icons/revert.png"));
    rescanBaseAct->setShortcut(QKeySequence::Refresh);
    connect(rescanBaseAct, SIGNAL(triggered()), this, SLOT(rescanBase()));

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

    deletePlottedChannelsAct = new QAction("Удалить построенные каналы...",this);
    deletePlottedChannelsAct->setIcon(QIcon(":/icons/remove-plotted.png"));
    connect(deletePlottedChannelsAct, SIGNAL(triggered()), this, SLOT(deletePlottedChannels()));

    copyPlottedChannelsAct = new QAction(QIcon(":/icons/channel-copy-plotted.png"),
                                         "Копировать построенные каналы в файл...", this);
    connect(copyPlottedChannelsAct, SIGNAL(triggered()), SLOT(copyPlottedChannels()));

    movePlottedChannelsAct = new QAction(QIcon(":/icons/channel-cut-plotted.png"),
                                         "Переместить построенные каналы в файл...", this);
    connect(movePlottedChannelsAct, SIGNAL(triggered()), SLOT(movePlottedChannels()));


    editDescriptionsAct = new QAction("Редактировать описание...", this);
    editDescriptionsAct->setIcon(QIcon(":/icons/descriptor.png"));
    connect(editDescriptionsAct, SIGNAL(triggered()), SLOT(editDescriptions()));

    editChannelDescriptionsAct = new QAction("Редактировать описание каналов...", this);
    editChannelDescriptionsAct->setIcon(QIcon(":/icons/descriptor.png"));
    connect(editChannelDescriptionsAct, SIGNAL(triggered()), SLOT(editChannelDescriptions()));

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
}

PlotArea *MainWindow::createPlotArea()
{
    ads::CDockComponentsFactory::setFactory(new PlotDockFactory(this));
    PlotArea *area = new PlotArea(PlotType::General, this);
    connect(this, SIGNAL(updateLegends()), area, SLOT(updateLegends()));
    return area;
}

void MainWindow::createTab(const QString &name, const QStringList &folders)
{DD;
    currentTab = new Tab(this);
    currentTab->setOrientation(Qt::Horizontal);

    currentTab->model = new Model(currentTab);
    connect(currentTab->model, &Model::needAddFiles, [=](const QStringList &files){
        addFiles(files);
        currentTab->fileHandler->trackFiles(files);
    });
    currentTab->sortModel = new SortFilterModel(currentTab);
    currentTab->sortModel->setSourceModel(currentTab->model);

    currentTab->channelModel = new ChannelTableModel(currentTab);
    connect(currentTab->channelModel, SIGNAL(modelChanged()), SLOT(updateActions()));
    connect(currentTab->channelModel, &ChannelTableModel::maybeUpdateChannelProperty,
            [=](int channel, const QString &description, const QString &p, const QString &val){
        if (!currentTab) return;
        if (currentTab->model->selected().size()>1) {
            if (QMessageBox::question(this,"DeepSea Base",QString("Выделено несколько файлов. Записать %1\n"
                                      "во все эти файлы?").arg(description))==QMessageBox::Yes)
            {
                currentTab->model->setChannelProperty(channel, p, val);
            }
        }
        currentTab->model->updateFile(currentTab->record);
    });
    connect(currentTab->channelModel,SIGNAL(maybePlot(int)),SLOT(plotChannel(int)));
    connect(currentTab->channelModel,SIGNAL(deleteCurve(int)),SLOT(deleteCurve(int)));

    currentTab->filesTable = new FilesTable(this);
    //connect(tab->filesTable, &FilesTable::addFiles, this, &MainWindow::addFiles);
    currentTab->filesTable->setModel(currentTab->sortModel);

    currentTab->filesTable->setRootIsDecorated(false);
    currentTab->filesTable->setSortingEnabled(true);
    currentTab->filesTable->sortByColumn(MODEL_COLUMN_INDEX, Qt::AscendingOrder);

    currentTab->filesTable->setContextMenuPolicy(Qt::CustomContextMenu);
    currentTab->filesTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    currentTab->filesTable->addAction(delFilesAct);
    currentTab->filesTable->addAction(saveAct);

    connect(currentTab->filesTable, &QTreeView::customContextMenuRequested, [=](){
        QMenu menu(currentTab->filesTable);
        int column = currentTab->filesTable->currentIndex().column();
        if (!currentTab->filesTable->selectionModel()->hasSelection()) {
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
            //menu.addAction(calculateSpectreDeepSeaAct);
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


    currentTab->filterHeader = new FilteredHeaderView(Qt::Horizontal, currentTab->filesTable);
    currentTab->filesTable->setHeader(currentTab->filterHeader);
    connect(currentTab->filterHeader, SIGNAL(filterChanged(QString,int)), currentTab->sortModel, SLOT(setFilter(QString,int)));
//    tab->filterHeader->setFilterBoxes(tab->model->columnCount());

    currentTab->filesTable->header()->setStretchLastSection(false);
    currentTab->filesTable->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    currentTab->filesTable->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    currentTab->filesTable->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    currentTab->filesTable->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    currentTab->filesTable->header()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    currentTab->filesTable->header()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    currentTab->filesTable->header()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    currentTab->filesTable->header()->setSectionResizeMode(7, QHeaderView::ResizeToContents);


    connect(currentTab->filesTable->selectionModel(),SIGNAL(selectionChanged(QItemSelection,QItemSelection)),currentTab, SLOT(filesSelectionChanged(QItemSelection,QItemSelection)));
    connect(currentTab->filesTable->selectionModel(),SIGNAL(currentChanged(QModelIndex,QModelIndex)),SLOT(updateChannelsTable(QModelIndex,QModelIndex)));

    currentTab->filesTable->setItemDelegateForColumn(MODEL_COLUMN_XSTEP, new StepItemDelegate);

    connect(currentTab->model, SIGNAL(legendsChanged()), this, SIGNAL(updateLegends()));

    connect(currentTab->model, &Model::plotNeedsUpdate, [=](){
        for (auto dock: plotAreas) {
            if (auto plotArea = dynamic_cast<PlotArea*>(dock))
                plotArea->update();
        }
    });
    connect(currentTab->model, SIGNAL(modelChanged()), SLOT(updateActions()));

    currentTab->channelsTable = new ChannelsTable(this);
    currentTab->channelsTable->setModel(currentTab->channelModel);
    currentTab->channelsTable->setDragEnabled(true);
    currentTab->channelsTable->setDragDropMode(QAbstractItemView::DragOnly);
   // tab->channelsTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
//    tab->channelsTable->setItemDelegateForColumn(1, new HtmlDelegate);
    connect(currentTab->channelsTable->selectionModel(),SIGNAL(selectionChanged(QItemSelection,QItemSelection)),currentTab, SLOT(channelsSelectionChanged(QItemSelection,QItemSelection)));



    currentTab->tableHeader = new HeaderView(Qt::Horizontal, currentTab->channelsTable);
    currentTab->channelsTable->setHorizontalHeader(currentTab->tableHeader);
    currentTab->tableHeader->setCheckable(0,true);

    currentTab->channelsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    currentTab->channelsTable->horizontalHeader()->setStretchLastSection(false);

    currentTab->channelsTable->addAction(moveChannelsUpAct);
    currentTab->channelsTable->addAction(moveChannelsDownAct);

    currentTab->channelsTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(currentTab->channelsTable, &QTableWidget::customContextMenuRequested, [=](){
        QMenu menu(currentTab->channelsTable);
        int column = currentTab->channelsTable->currentIndex().column();
        if (column == 1) {
            menu.addAction(editYNameAct);
            menu.exec(QCursor::pos());
        }
        else if (column == 0) {
            menu.addAction(plotSelectedChannelsAct);
            menu.addAction(plotSelectedChannelsOnRightAct);
            if (currentTab->record && currentTab->record->isSourceFile())
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

    currentTab->filePathLabel = new QLabel(this);
    currentTab->filePathLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    currentTab->filePathLabel->setSizePolicy(QSizePolicy::Expanding, currentTab->filePathLabel->sizePolicy().verticalPolicy());

    QAction *openFolderAct = new QAction("Открыть папку с этой записью", this);
    openFolderAct->setIcon(QIcon(":/icons/open.png"));
    connect(openFolderAct, &QAction::triggered, [=](){
        if (!currentTab->filePathLabel->text().isEmpty()) {
            QDir dir(currentTab->filePathLabel->text());
            dir.cdUp();
            QProcess::startDetached("explorer.exe", QStringList(dir.toNativeSeparators(dir.absolutePath())));
        }
    });

    QAction *editFileAct = new QAction("Редактировать этот файл в текстовом редакторе", this);
    editFileAct->setIcon(QIcon(":/icons/edit.png"));
    connect(editFileAct, &QAction::triggered, [=](){
        QString file = QDir::toNativeSeparators(currentTab->filePathLabel->text());
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
    channelsToolBar->addWidget(currentTab->filePathLabel);
    channelsToolBar->addAction(editFileAct);
    channelsToolBar->addAction(openFolderAct);

    QWidget *channelsWidget = new QWidget(this);
    channelsWidget->setContentsMargins(0,0,0,0);
    QGridLayout *channelsLayout = new QGridLayout;
    channelsLayout->addWidget(channelsToolBar,0,0,1,3);
    channelsLayout->addWidget(currentTab->channelsTable,1,0,1,3);
    channelsWidget->setLayout(channelsLayout);


    currentTab->addWidget(currentTab->filesTable);
    currentTab->addWidget(channelsWidget);

    QByteArray upperSplitterState = App->getSetting("upperSplitterState").toByteArray();
    if (!upperSplitterState.isEmpty())
        currentTab->restoreState(upperSplitterState);

    ads::CDockComponentsFactory::setFactory(new CCustomDockFactory(this));
    auto dockWidget = new ads::CDockWidget(name);
    dockWidget->setWidget(currentTab);
    dockWidget->setFeature(ads::CDockWidget::DockWidgetFloatable, false);
    dockWidget->setFeature(ads::CDockWidget::DockWidgetDeleteOnClose, true);
    dockWidget->setFeature(ads::CDockWidget::DockWidgetFocusable, true);
    dockWidget->setFeature(ads::CDockWidget::CustomCloseHandling, true);

    if (!topArea) {
        topArea = m_DockManager->addDockWidget(ads::TopDockWidgetArea, dockWidget);
        connect(topArea, &ads::CDockAreaWidget::currentChanged, this, &MainWindow::changeCurrentTab);
    }
    else
        m_DockManager->addDockWidget(ads::CenterDockWidgetArea, dockWidget, topArea);
    //ads::CDockComponentsFactory::resetDefaultFactory();

    connect(dockWidget, &ads::CDockWidget::closeRequested, [=](){
        closeTab(dockWidget);
    });

    connect(currentTab->fileHandler, SIGNAL(fileAdded(QString,bool,bool)),this,SLOT(addFolder(QString,bool,bool)));

    currentTab->fileHandler->setFileNames(folders);
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

void MainWindow::closeTab(ads::CDockWidget *t)
{DD;
    if (!t) return;

    qDebug()<<"tab close";

    //TODO: Реализовать удаление графиков
//    if (plot) {
//        for (int i=0; i<tab->model->size(); ++i) {
//            const F f = tab->model->file(i);
//            //use_count==3 if file is only in one tab, (1 for App, 1 for model, 1 for the prev line)
//            //use_count>=4 if file is in more than one tab
//            if (f.use_count()<=3 && f->hasCurves()) {
//                plot->deleteCurvesForDescriptor(f.get());
//            }
//        }
//    }
    t->closeDockWidget();
}

void MainWindow::closeOtherTabs(int index)
{DD;
    if (!topArea) return;
    int count = topArea->dockWidgetsCount();
    if (count<=1) return;

    for (int i = count - 1; i > index; --i)
        closeTab(topArea->dockWidget(i));
    for (int i = index - 1; i >= 0; --i)
        closeTab(topArea->dockWidget(i));
}

void MainWindow::changeCurrentTab(int currentIndex)
{DD;
    if (!topArea) return;

    currentTab = 0;
    if (currentIndex<0) return;

    currentTab = qobject_cast<Tab *>(topArea->dockWidget(currentIndex)->widget());
    updateActions();
}

void MainWindow::editColors()
{DD;
    ColorEditDialog dialog(this);
    dialog.exec();
}

MainWindow::~MainWindow()
{DD;
//    if (currentTab) {
//        setSetting("upperSplitterState",currentTab->saveState());
//        QByteArray treeHeaderState = currentTab->filesTable->header()->saveState();
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

void MainWindow::addPlot()
{
    currentPlot = createPlotArea();
    plotAreas << currentPlot;

    if (!bottomArea)
        bottomArea = m_DockManager->addDockWidget(ads::BottomDockWidgetArea, currentPlot);
    else {
        m_DockManager->addDockWidget(ads::RightDockWidgetArea, currentPlot, bottomArea);
    }

    plotsMenu->addAction(currentPlot->toggleViewAction());
    updateActions();
}

QString MainWindow::getFolderToAdd(bool withSubfolders)
{DD;
    QString directory = App->getSetting("lastDirectory").toString();

    directory = QFileDialog::getExistingDirectory(this,
                                                  withSubfolders?tr("Добавление папки со всеми вложенными папками"):
                                                  tr("Добавление папки"),
                                                  directory,
                                                  QFileDialog::ShowDirsOnly | QFileDialog::ReadOnly);

    if (!directory.isEmpty())
        App->setSetting("lastDirectory", directory);
    return directory;
}

void MainWindow::addFolder() /*SLOT*/
{DD;
    if (auto folder = getFolderToAdd(false /*with subfolders*/); addFolder(folder, false /*with subfolders*/, false /*silent*/))
        currentTab->fileHandler->trackFolder(folder, false);
}

void MainWindow::addFolderWithSubfolders() /*SLOT*/
{DD;
    if (auto folder = getFolderToAdd(true /*with subfolders*/); addFolder(folder, true /*with subfolders*/, false /*silent*/))
        currentTab->fileHandler->trackFolder(folder, true);
}

void MainWindow::addFile()
{DD;
    QString directory = App->getSetting("lastDirectory").toString();

    QFileDialog dialog(this, "Добавить файлы", directory);
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);
    QString defaultSuffix = QFileInfo(directory).suffix();
    if (defaultSuffix.isEmpty()) defaultSuffix = "dfd";
    dialog.setDefaultSuffix(defaultSuffix);
    QSortFilterProxyModel *proxy = new DfdFilterProxy(0, this);
    dialog.setProxyModel(proxy);
    dialog.setFileMode(QFileDialog::ExistingFiles);

    if (dialog.exec()) {
        const QStringList fileNames = dialog.selectedFiles();
        if (fileNames.isEmpty()) return;
        App->setSetting("lastDirectory", fileNames.constFirst());
        if (addFiles(fileNames))
            currentTab->fileHandler->trackFiles(fileNames);
    }
}

bool MainWindow::addFolder(const QString &directory, bool withAllSubfolders, bool silent)
{DD;
    if (directory.isEmpty() || !currentTab) return false;

    QStringList filesToAdd;
    processDir(directory, filesToAdd, withAllSubfolders);
    if (filesToAdd.isEmpty()) return false;

    QStringList toAdd;
    for (const QString &file: qAsConst(filesToAdd)) {
        if (!currentTab->model->contains(file))
            toAdd << file;
    }
    if (toAdd.isEmpty()) return false;

    addFiles(toAdd, silent);
    if (toAdd.size() < filesToAdd.size() && !silent) {
        QMessageBox::information(this,QString("База данных"),
                                 toAdd.isEmpty()?QString("Некоторые файлы уже есть в базе"):
                                                 QString("Только %1 файлов из %2 было добавлено")
                                                 .arg(toAdd.size())
                                                 .arg(filesToAdd.size()));
    }
    return true;
}

void MainWindow::deleteFiles()
{DD;
    if (!currentTab) return;

    const auto indexes = currentTab->model->selected();
    for (int i: indexes) {
        //TODO: Реализовать удаление графиков
//        const F f = tab->model->file(i);
//        //use_count==3 if file is only in one tab, (1 for App, 1 for model, 1 for the prev line)
//        //use_count>=4 if file is in more than one tab
//        if (f.use_count()<=3 && f->hasCurves())
//            plot->deleteCurvesForDescriptor(f.get());
//        tab->fileHandler->untrackFile(f->fileName());
    }
    currentTab->channelModel->clear();

    currentTab->model->deleteSelectedFiles();

    if (currentTab->model->size() == 0)
        currentTab->fileHandler->clear();
}

void MainWindow::deleteChannels() /** SLOT */
{DD;
    QVector<int> channelsToDelete = currentTab->channelModel->selected();
    if (channelsToDelete.isEmpty()) return;

    if (QMessageBox::question(this,"DeepSea Base",
                              QString("Выделенные %1 каналов будут \nудалены из записи. Продолжить?").arg(channelsToDelete.size())
                              )==QMessageBox::Yes) {
        LongOperation op;
        deleteChannels(currentTab->record, channelsToDelete);
        updateChannelsTable(currentTab->record);
    }
}

void MainWindow::deleteChannelsBatch()
{DD;
    QVector<int> channels = currentTab->channelModel->selected();
    if (channels.isEmpty()) return;

    const QList<FileDescriptor*> filesToDelete = currentTab->model->selectedFiles();
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

    updateChannelsTable(currentTab->record);
}

void MainWindow::copyChannels() /** SLOT */
{DD;
    QVector<int> channelsToCopy = currentTab->channelModel->selected();
    if (channelsToCopy.isEmpty()) return;

    if (copyChannels(currentTab->record, channelsToCopy)) {
        updateChannelsTable(currentTab->record);
    }
}

void MainWindow::moveChannels() /** SLOT */
{DD;
    // сначала копируем каналы, затем удаляем
    QVector<int> channelsToMove = currentTab->channelModel->selected();
    if (channelsToMove.isEmpty()) return;

    if (QMessageBox::question(this,"DeepSea Base","Выделенные каналы будут \nудалены из файла. Продолжить?")==QMessageBox::Yes) {
        if (copyChannels(currentTab->record, channelsToMove)) {
            deleteChannels(currentTab->record, channelsToMove);
            updateChannelsTable(currentTab->record);
        }
    }
}

QVector<FileDescriptor*> plottedDescriptors(const QList<Curve*> &curves)
{DD;
    QVector<FileDescriptor*> result;

    for (auto c: curves) {
        if (auto d = c->channel->descriptor(); !result.contains(d))
            result.append(d);
    }

    return result;
}

void MainWindow::deletePlottedChannels() /** SLOT*/
{DD;
    if (QMessageBox::question(this,"DeepSea Base",
                              "Эти каналы будут \nудалены из записей. Продолжить?"
                              )==QMessageBox::Yes) {
        LongOperation op;
        deleteChannels({});

        updateChannelsTable(currentTab->record);
    }
}

void MainWindow::copyPlottedChannels() /** SLOT*/
{DD;
//    auto channels = plot->plottedChannels();
//    if (!channels.isEmpty()) {
//        if (copyChannels(channels))
//            updateChannelsTable(tab->record);
//    }
}

void MainWindow::movePlottedChannels() /** SLOT*/
{DD;
    // сначала копируем каналы, затем удаляем
//    auto channels = plot->plottedChannels();
//    if (!channels.isEmpty()) {
//        if (QMessageBox::question(this,"DeepSea Base","Выделенные каналы будут \nудалены из файлов. Продолжить?")==QMessageBox::Yes) {
//            if (copyChannels(channels)) {
//                deleteChannels(channels);
//                updateChannelsTable(tab->record);
//            }
//        }
//    }
}

void MainWindow::addCorrection()
{DD;
//    if (plot->hasCurves()) {
//        QList<FileDescriptor*> files = tab->model->selectedFiles();

//        CorrectionDialog dialog(plot, files);
//        dialog.exec();

//        updateChannelsTable(tab->record);
//    }
}

void MainWindow::addCorrections()
{DD;
//    static QString pathToExcelFile;
//    pathToExcelFile = QFileDialog::getOpenFileName(this, "Выбрать файл xls с поправками", pathToExcelFile, "*.xls;;*.xlsx");

//    if (pathToExcelFile.isEmpty()) return;

//    QAxObject *excel = 0;

//    if (!excel) excel = new QAxObject("Excel.Application", this);
//    //excel->setProperty("Visible", true);


//    //получаем рабочую книгу
//    QAxObject * workbooks = excel->querySubObject("WorkBooks");
//    workbooks->dynamicCall("Open(QString)", pathToExcelFile);

//    QAxObject * workbook = excel->querySubObject("ActiveWorkBook");
//    if (!workbook) {
//        QMessageBox::critical(this, "DeepSea Database",
//                              "Не удалось открыть текущую рабочую книгу Excel");
//        return;
//    }
//    QAxObject * worksheet = workbook->querySubObject("ActiveSheet");
//    if (!worksheet) {
//        QMessageBox::critical(this, "DeepSea Database",
//                              "Не удалось открыть текущий рабочий лист Excel");
//        return;
//    }

//    int column = 1;
//    while (1) {
//        QAxObject *cells = worksheet->querySubObject("Cells(Int,Int)", 1, column);
//        QString fileName = cells->property("Value").toString();
//        if (fileName.isEmpty()) break;

//        // qDebug()<<fileName;
//        if (QFile(fileName).exists()) {
//            fileName.replace("\\","/");
//            FileDescriptor * dfd = App->find(fileName).get();
//            bool deleteAfter=false;
//            if (!dfd) {
//                dfd = FormatFactory::createDescriptor(fileName);
//                dfd->read();
//                deleteAfter = true;
//            }
//            dfd->populate();

//            int row = 2;
//            bool corrected = false;
//            while (1) {
//                QAxObject *cells1 = worksheet->querySubObject("Cells(Int,Int)", row, column);
//                QString value = cells1->property("Value").toString();
//               // qDebug()<<value;
//                if (value.isEmpty()) break;
//                if (row-2>=dfd->channelsCount()) break;

//                double corr = value.toDouble();
//                if (corr != 0.0) {
//                    corrected = true;
//                    dfd->channel(row-2)->data()->setTemporaryCorrection(corr, 0);
//                    dfd->channel(row-2)->data()->makeCorrectionConstant();
//                    dfd->channel(row-2)->setCorrection(dfd->channel(row-2)->data()->correctionString());
//                    dfd->channel(row-2)->data()->removeCorrection();
//                    dfd->channel(row-2)->setChanged(true);
//                    dfd->channel(row-2)->setDataChanged(true);
//                }
//                row++;
//            }
//            if (corrected) {
//                dfd->setChanged(true);
//                dfd->setDataChanged(true);
//                dfd->write();
//            }
//            if (deleteAfter) delete dfd;
//        }

//        column++;
//    }

//    updateChannelsTable(tab->record);
//    plot->updateAxes();
//    emit updateLegends();
//    plot->replot();


//    delete worksheet;
//    delete workbook;
//    delete workbooks;

//    excel->dynamicCall("Quit()");
//    delete excel;

//    QMessageBox::information(this,"","Готово!");
}

bool MainWindow::deleteChannels(FileDescriptor *file, const QVector<int> &channelsToDelete)
{DD;
//    for (int i=0; i<channelsToDelete.size() - 1; ++i)
//        plot->deleteCurveForChannelIndex(file, channelsToDelete.at(i), false);
//    plot->deleteCurveForChannelIndex(file, channelsToDelete.last(), true);

//    LongOperation op;
//    file->deleteChannels(channelsToDelete);

    return true;
}

bool MainWindow::deleteChannels(const QVector<Channel *> channels)
{DD;
    Q_UNUSED(channels);

//    auto plotted = plottedDescriptors(plot->curves);
//    for (auto d: plotted) {
//        auto list = d->plottedIndexes();
//        deleteChannels(d, list);
//    }
    return true;
}

bool MainWindow::copyChannels(FileDescriptor *source, const QVector<int> &channelsToCopy)
{DD;
    QVector<Channel*> channels;
    for (int i: channelsToCopy) channels << source->channel(i);
    if (!channels.isEmpty()) return copyChannels(channels);
    return false;
}

bool MainWindow::copyChannels(const QVector<Channel *> source)
{DD;
    QString startFile = App->getSetting("startDir").toString();
    QStringList filters = FormatFactory::allFilters();

    QFileDialog dialog(this, "Выбор файла для записи каналов", startFile,
                       filters.join(";;"));
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);

    QStringList suffixes = FormatFactory::allSuffixes(true);


    //проверка каналов на равенство
    const auto type = source.constFirst()->type();
    const auto xStep = source.constFirst()->data()->xStep();
    const auto samplesCount = source.constFirst()->data()->samplesCount();

    const bool typesAreSame = std::all_of(source.cbegin(), source.cend(),
                                          [type](Channel*c){return c->type()==type;});
    const bool stepsAreSame = std::all_of(source.cbegin(), source.cend(),
                                          [xStep](Channel*c){return c->data()->xStep()==xStep;});
    const bool samplesCountAreSame = std::all_of(source.cbegin(), source.cend(),
                                                 [samplesCount](Channel*c){return c->data()->samplesCount()==samplesCount;});


    // если файл - dfd, то мы можем записать его только в dfd файл с таким же типом данных и шагом по x
    // поэтому если они различаются, то насильно записываем каналы в файл uff

    // мы не можем записывать каналы с разным типом/шагом в один файл dfd,
    //поэтому добавляем фильтр
    QSortFilterProxyModel *proxy = new DfdFilterProxy(source.first()->descriptor(), this);
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

    if ((!typesAreSame || !stepsAreSame || !samplesCountAreSame) && filterSuffix.toLower()=="dfd") {
        QMessageBox::critical(0, "Копирование графиков",
                              "Графики несовместимы между собой и не могут быть записаны в файл dfd");
        return false;
    }

    App->setSetting("startDir", file);


    LongOperation op;

    // ИЩЕМ ЭТОТ ФАЙЛ СРЕДИ ДОБАВЛЕННЫХ В БАЗУ

    F destination = App->find(file);
    const bool found = destination.get() != nullptr;
    const bool exists = QFile::exists(file);

    bool isNew = false;
    if (!exists) {//такого файла не существует
        destination = App->addFile(source, file, &isNew);
    }
    else {
        //файл существует, два варианта:
        if (found) {//уже добавлен в базу
            //записываем все изменения данных, если они были
            destination->write();
        }
        else {//еще не добавлен в базу
            destination = App->addFile(file, &isNew);
            destination->read();
        }
        destination->copyChannelsFrom(source);
    }

    if (!destination) {
        qDebug()<<"Неизвестный тип файла"<< file;
        return false;
    }

    if (currentTab) {
        if (found) {
            currentTab->model->updateFile(destination.get());
        }
        else {
            addFile(destination);
            currentTab->fileHandler->trackFiles({file});
        }
    }
    return true;
}

void MainWindow::calculateMean()
{DD;
    if (!currentPlot) return;

    if (currentPlot->curvesCount()<2) return;

//    QList<Channel*> channels;

//    bool dataTypeEqual = true;
//    bool stepsEqual = true; // одинаковый шаг по оси Х
//    bool namesEqual = true; // одинаковые названия осей Y
//    bool oneFile = true; // каналы из одного файла
//    bool writeToSeparateFile = true;
//    bool writeToD94 = false;

//    Channel *firstChannel = plot->curves.first()->channel;
//    channels.append(firstChannel);
//    FileDescriptor *firstDescriptor = firstChannel->descriptor();

//    bool allFilesDfd = firstDescriptor->fileName().toLower().endsWith("dfd");
//    auto firstChannelFileName = firstDescriptor->fileName();

//    for (int i = 1; i<plot->curves.size(); ++i) {
//        Curve *curve = plot->curves.at(i);
//        channels.append(curve->channel);

//        allFilesDfd &= firstChannelFileName.toLower().endsWith("dfd");
//        if (firstChannel->data()->xStep() != curve->channel->data()->xStep())
//            stepsEqual = false;
//        if (firstChannel->yName() != curve->channel->yName())
//            namesEqual = false;
//        if (firstChannelFileName != curve->channel->descriptor()->fileName())
//            oneFile = false;
//        if (!firstDescriptor->dataTypeEquals(curve->channel->descriptor()))
//            dataTypeEqual = false;
//    }
//    if (!dataTypeEqual) {
//        int result = QMessageBox::warning(0, "Среднее графиков",
//                                          "Графики имеют разный тип. Среднее будет записано в файл d94. Продолжить?",
//                                          "Да","Нет");
//        if (result == 1) return;
//        else writeToD94 = true;
//    }
//    if (!namesEqual) {
//        int result = QMessageBox::warning(0, "Среднее графиков",
//                                          "Графики по-видимому имеют разный тип. Среднее будет записано в файл d94. Продолжить?",
//                                          "Да", "Нет");
//        if (result == 1) return;
//        else writeToD94 = true;
//    }
//    if (!stepsEqual) {
//        int result = QMessageBox::warning(0, "Среднее графиков",
//                                          "Графики имеют разный шаг по оси X. Среднее будет записано в файл d94. Продолжить?",
//                                          "Да", "Нет");
//        if (result == 1) return;
//        else writeToD94 = true;
//    }
//    if (oneFile) {
//        QMessageBox box("Среднее графиков",
//                        QString("Графики взяты из одной записи %1.\n").arg(firstChannelFileName)
//                        +
//                        QString("Сохранить среднее в эту запись дополнительным каналом?"),
//                        QMessageBox::Question,
//                        QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel);
//        box.setButtonText(QMessageBox::Yes, "Да, записать в этот файл");
//        box.setButtonText(QMessageBox::No, "Нет, экспортировать в отдельный файл");
//        box.setEscapeButton(QMessageBox::Cancel);

//        int result = box.exec();
//        if (result == QMessageBox::Cancel) return;
//        writeToSeparateFile = (result == QMessageBox::No);
//    }

//    QString meanFileName;
//    F meanFile;

//    if (writeToSeparateFile) {
//        QString meanD = firstChannelFileName;
//        meanD.chop(4);
//        if (writeToD94) meanD.append(".d94");

//        meanD = App->getSetting(writeToD94?"lastMeanUffFile":"lastMeanFile", meanD).toString();

//        QStringList  filters = FormatFactory::allFilters();
//        QStringList suffixes = FormatFactory::allSuffixes(true);

//        QFileDialog dialog(this, "Выбор файла для записи каналов", meanD,
//                           filters.join(";;"));
//        dialog.setOption(QFileDialog::DontUseNativeDialog, true);
//        dialog.setFileMode(QFileDialog::AnyFile);
////        dialog.setDefaultSuffix(writeToUff?"uff":"dfd");

//        if (!writeToD94) {
//            QSortFilterProxyModel *proxy = new DfdFilterProxy(firstDescriptor, this);
//            dialog.setProxyModel(proxy);
//        }

//        QStringList selectedFiles;
//        QString selectedFilter;
//        if (dialog.exec()) {
//            selectedFiles = dialog.selectedFiles();
//            selectedFilter = dialog.selectedNameFilter();
//        }
//        if (selectedFiles.isEmpty()) return;

//        meanFileName = selectedFiles.constFirst();
//        if (meanFileName.isEmpty()) return;

//        QString currentSuffix = QFileInfo(meanFileName).suffix().toLower();
//        QString filterSuffix = suffixes.at(filters.indexOf(selectedFilter));

//        if (currentSuffix != filterSuffix) {
//            //удаляем суффикс, если это суффикс известного нам типа файлов
//            if (suffixes.contains(currentSuffix))
//                meanFileName.chop(currentSuffix.length()+1);

//            meanFileName.append(QString(".%1").arg(filterSuffix));
//        }

//        bool isNew = false;
//        const bool fileExists = QFile::exists(meanFileName);
//        meanFile = App->addFile(meanFileName, &isNew);
//        if (!meanFile) {
//            QMessageBox::critical(this, "Расчет среднего", "Не удалось создать файл неизвестного типа:"
//                                  + meanFileName);
//            qDebug()<<"Не удалось создать файл"<<meanFileName;
//            return;
//        }

//        if (fileExists) {
//            if (isNew) meanFile->read();
//        }
//        else
//            meanFile->fillPreliminary(firstDescriptor);

//        App->setSetting(writeToD94?"lastMeanUffFile":"lastMeanFile", meanFileName);
//    }
//    else {
//        meanFileName = firstChannelFileName;
//        meanFile = App->find(meanFileName);
//    }

//    meanFile->calculateMean(channels);

//    int idx;
//    if (tab->model->contains(meanFile, &idx)) {
//        tab->model->updateFile(idx);
//    }
//    else {
//        addFile(meanFile);
//    }
//    setCurrentAndPlot(meanFile.get(), meanFile->channelsCount()-1);
}

void MainWindow::moveChannelsUp()
{DD;
    moveChannels(true);
}

void MainWindow::moveChannelsDown()
{DD;
    moveChannels(false);
}

void MainWindow::editDescriptions()
{DD;
    if (!currentTab) return;

    QList<FileDescriptor *> records = currentTab->model->selectedFiles();
    if (records.isEmpty()) return;

//    EditDescriptionsDialog dialog(records, this);
//    if (dialog.exec()) {
//        auto descriptions = dialog.descriptions();
//        for (int i=0; i<descriptions.size(); ++i) {
//            tab->model->setDataDescription(i, descriptions.at(i));
//        }
//    }
    DescriptorPropertiesDialog dialog(records, this);
    dialog.exec();
}

void MainWindow::editChannelDescriptions()
{DD;
    if (!currentTab) return;

    QVector<Channel *> selectedChannels = currentTab->channelModel->selectedChannels();
    if (selectedChannels.isEmpty()) return;

    ChannelPropertiesDialog dialog(selectedChannels, this);
    dialog.exec();
    currentTab->channelModel->modelChanged();
//    updateChannelsTable(tab->record);
}

void MainWindow::save()
{DD;
    if (!tabWidget) return;

    for (int i=0; i<tabWidget->count(); ++i) {
        Tab *t = qobject_cast<Tab *>(tabWidget->widget(i));
        if (t)
            t->model->save();
    }
}

void MainWindow::convertMatFiles()
{DD;
    MatlabConverterDialog dialog(this);
    if (dialog.exec()) {
        if (dialog.addFiles()) {
            QStringList files = dialog.getConvertedFiles();
            this->addFiles(files);
            if (currentTab) currentTab->fileHandler->trackFiles(files);
        }
    }
}

void MainWindow::convertTDMSFiles()
{DD;
    TDMSConverterDialog dialog(this);
    if (dialog.exec()) {
        if (dialog.addFiles()) {
            QStringList files = dialog.getConvertedFiles();
            this->addFiles(files);
            if (currentTab) currentTab->fileHandler->trackFiles(files);
        }
    }
}

void MainWindow::convertEsoFiles()
{DD;
    EsoConverterDialog dialog(this);
    if (dialog.exec()) {
        QStringList files = dialog.getConvertedFiles();
        if (files.isEmpty()) return;
        this->addFiles(files);
        if (currentTab) currentTab->fileHandler->trackFiles(files);
    }
}

void MainWindow::saveTimeSegment(const QList<FileDescriptor *> &files, double from, double to)
{DD;
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
{DD;
    if (!currentTab) return;

    QVector<int>  selectedIndexes = currentTab->channelModel->selected();
    if (selectedIndexes.isEmpty()) return;

    QString newYName = QInputDialog::getText(this, "Новая единица измерения", "Введите новую единицу");
    if (newYName.isEmpty()) return;

    currentTab->channelModel->setYName(newYName);
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
    QVector<int> selectedIndexes = currentTab->channelModel->selected();

    const QVector<int> newIndexes = computeIndexes(selectedIndexes, up, currentTab->record->channelsCount());

    if (newIndexes == selectedIndexes) return;

    for (auto &file: currentTab->model->selectedFiles()) {
        file->move(up, selectedIndexes, newIndexes);
    }

    //tab->record->move(up, selectedIndexes, newIndexes);

    updateChannelsTable(currentTab->record);

    currentTab->channelsTable->clearSelection();
    for (int i: newIndexes)
        currentTab->channelsTable->selectionModel()->select(currentTab->channelModel->index(i,0),QItemSelectionModel::Select);
}

void MainWindow::updateChannelsTable(const QModelIndex &current, const QModelIndex &previous)
{DD;
    if (!currentTab || !current.isValid()) return;
    if (current.model() != currentTab->sortModel) return;

    if (previous.isValid()) {
        if ((previous.row()==0 && previous.column() != current.column()) ||
            (previous.row() != current.row())) {
            QModelIndex index = currentTab->sortModel->mapToSource(current);
            updateChannelsTable(currentTab->model->file(index.row()).get());
        }
    }
    else {
        QModelIndex index = currentTab->sortModel->mapToSource(current);
        updateChannelsTable(currentTab->model->file(index.row()).get());
    }



}

void MainWindow::updateChannelsTable(FileDescriptor *descriptor)
{DD;
    QVector<int> plottedChannels = plottedChannelsNumbers;
    if (sergeiMode) {
        currentTab->channelModel->deleteCurves();
    }
    //возвращаем после удаления
    plottedChannelsNumbers = plottedChannels;

    currentTab->record = descriptor;
    currentTab->channelModel->setDescriptor(descriptor);
    if (!descriptor) return;

    if (!descriptor->fileExists()) {
        QMessageBox::warning(this,"Не могу получить список каналов","Такого файла уже нет");
        return;
    }

    currentTab->filePathLabel->setText(descriptor->fileName());

    if (descriptor->channelsCount() == 0) return;

    if (sergeiMode) {
        if (!plottedChannelsNumbers.isEmpty())
            currentTab->channelModel->plotChannels(plottedChannelsNumbers);
    }
    //возвращаем после возможного неполного рисования, чтобы не терялись каналы
    //при переходе к файлам с меньшим количеством каналов
    plottedChannelsNumbers = plottedChannels;
}

void MainWindow::plotAllChannels()
{DD;
    if (currentTab) currentTab->channelModel->plotChannels(false);
}

void MainWindow::plotAllChannelsAtRight()
{DD;
    if (currentTab) currentTab->channelModel->plotChannels(true);
}

void MainWindow::plotChannel(int index)
{DD;
//    QColor col;
//    int idx;
//    tab->model->contains(tab->record, &idx);

//    bool plotOnRight = tab->record->channel(index)->plotted()==2;
//    bool plotted = plot->plotCurve(tab->record->channel(index), &col, plotOnRight, idx+1);

//    if (plotted) {
//        tab->record->channel(index)->setColor(col);
//        tab->record->channel(index)->setPlotted(plotOnRight?2:1);
//    }
//    else {
//        tab->record->channel(index)->setColor(QColor());
//        tab->record->channel(index)->setPlotted(0);
//    }
//    tab->channelModel->onCurveChanged(tab->record->channel(index));
//    tab->model->updateFile(tab->record, MODEL_COLUMN_FILENAME);

//    if (tab->model->selected().size()>1 && QApplication::keyboardModifiers() & Qt::ControlModifier) {
//        QList<FileDescriptor*> selectedFiles = tab->model->selectedFiles();
//        foreach (FileDescriptor *f, selectedFiles) {
//            if (f == tab->record) continue;
//            if (f->channelsCount()<=index) continue;

//            tab->model->contains(f, &idx);
//            plotted = plot->plotCurve(f->channel(index), &col, plotOnRight, idx+1);
//            if (plotted) {
//                f->channel(index)->setColor(col);
//                f->channel(index)->setPlotted(plotOnRight?2:1);
//            }
//            else {
//                f->channel(index)->setPlotted(0);
//                f->channel(index)->setColor(QColor());
//            }
//            tab->model->updateFile(f, MODEL_COLUMN_FILENAME);
//        }
//    }
}

void MainWindow::calculateSpectreRecords(bool useDeepsea)
{DD;
    if (!currentTab) return;

    //QMessageBox::warning(this, "DeepSea Database", "Эта часть программы еще не написана");
    //return;

    QList<FileDescriptor *> records = currentTab->model->selectedFiles({Descriptor::TimeResponse});

    if (records.isEmpty()) {
        QMessageBox::warning(this,QString("DeepSea Base"),
                             QString("Не выделено ни одного файла с исходными временными данными"));
        return;
    }

    if (useDeepsea) {
        CalculateSpectreDialog dialog(records, this);
        if (dialog.exec()) {
            const QStringList files = dialog.getNewFiles();
            addFiles(files);
            currentTab->fileHandler->trackFiles(files);
        }
    }
    else {
        FilesProcessorDialog dialog(records, this);

        if (dialog.exec()) {
            const QStringList files = dialog.getNewFiles();
            addFiles(files);
            currentTab->fileHandler->trackFiles(files);
        }
    }
}

void MainWindow::convertFiles()
{DD;
    if (!currentTab) return;
    QList<FileDescriptor *> records = currentTab->model->selectedFiles();

    ConverterDialog dialog(records, this);
    dialog.exec();

    if (dialog.addFiles()) {
        QStringList files = dialog.getConvertedFiles();
        addFiles(files);
        currentTab->fileHandler->trackFiles(files);
    }
}

void MainWindow::copyToLegend()
{DD;
    if (!currentTab) return;
    const QList<FileDescriptor *> records = currentTab->model->selectedFiles();

    for (FileDescriptor *f: records) {
        f->setLegend(QFileInfo(f->fileName()).completeBaseName());
        currentTab->model->updateFile(f, MODEL_COLUMN_LEGEND);
        emit updateLegends();
    }
}

void MainWindow::calculateThirdOctave()
{DD;
    if (!currentTab) return;
    QList<FileDescriptor *> records = currentTab->model->selectedFiles();
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

    QList<F> toAdd;

    for (auto f: qAsConst(records)) {
        QString thirdOctaveFileName = createUniqueFileName("", f->fileName(), "3oct", "dfd", false);
        F newFile = App->addFile(thirdOctaveFileName);
        newFile->fillPreliminary(f);
        newFile->calculateThirdOctave(f);

        int idx;
        if (currentTab->model->contains(thirdOctaveFileName, &idx)) {
            currentTab->model->updateFile(idx);
        }
        else {
            addFile(newFile);
        }
    }
}

void MainWindow::calculateMovingAvg()
{DD;
//    if (plot->curves.size()<1) return;

//    int windowSize = App->getSetting("movingAvgSize",3).toInt();
//    bool ok;
//    windowSize = QInputDialog::getInt(this,"Скользящее среднее","Выберите величину окна усреднения",windowSize,
//                                      3,15,2,&ok);
//    if (ok)
//        App->setSetting("movingAvgSize",windowSize);
//    else
//        return;

//    QList<Channel*> channels;

//    bool oneFile = true; // каналы из одного файла
//    bool writeToSeparateFile = true;

//    Curve *firstCurve = plot->curves.constFirst();
//    channels.append(firstCurve->channel);
//    const QString firstName = firstCurve->channel->descriptor()->fileName();

//    bool allFilesDfd = firstName.toLower().endsWith("dfd") ||
//                       firstName.toLower().endsWith("d94");

//    for (int i = 1; i<plot->curves.size(); ++i) {
//        Curve *curve = plot->curves.at(i);
//        channels.append(curve->channel);
//        const QString curveName = curve->channel->descriptor()->fileName();

//        allFilesDfd &= (curveName.toLower().endsWith("dfd") ||
//                        curveName.toLower().endsWith("d94"));
//        if (firstName != curveName)
//            oneFile = false;
//    }
//    if (oneFile) {
//        QMessageBox box("Скользящее среднее каналов", QString("Каналы взяты из одной записи %1.\n").arg(firstName)
//                        +
//                        QString("Сохранить скользящее среднее в эту запись дополнительными каналами?"),
//                        QMessageBox::Question,
//                        QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel);
//        box.setButtonText(QMessageBox::Yes, "Да, записать в этот файл");
//        box.setButtonText(QMessageBox::No, "Нет, экспортировать в отдельный файл");
//        box.setEscapeButton(QMessageBox::Cancel);

//        int result = box.exec();
//        if (result == QMessageBox::Cancel) return;
//        writeToSeparateFile = (result == QMessageBox::No);
//    }

//    QString avgFileName;
//    F avg;

//    if (writeToSeparateFile) {
//        QString avgD = firstName;
//        avgD.chop(4);

//        avgD = App->getSetting("lastMovingAvgFile", avgD).toString();

//        QStringList filters = FormatFactory::allFilters();
//        QStringList suffixes = FormatFactory::allSuffixes(true);

//        QFileDialog dialog(this, "Выбор файла для записи каналов", avgD,
//                           filters.join(";;"));
//        dialog.setOption(QFileDialog::DontUseNativeDialog, true);
//        dialog.setFileMode(QFileDialog::AnyFile);

//        if (allFilesDfd) {
//            QSortFilterProxyModel *proxy = new DfdFilterProxy(firstCurve->channel->descriptor(), this);
//            dialog.setProxyModel(proxy);
//        }

//        QString selectedFilter;
//        QStringList selectedFiles;
//        if (dialog.exec()) {
//            selectedFiles = dialog.selectedFiles();
//            selectedFilter = dialog.selectedNameFilter();
//        }
//        if (selectedFiles.isEmpty()) return;

//        avgFileName = selectedFiles.constFirst();
//        if (avgFileName.isEmpty()) return;
//        App->setSetting("lastMovingAvgFile", avgFileName);

//        //добавляем суффикс
//        QString currentSuffix = QFileInfo(avgFileName).suffix().toLower();
//        QString filterSuffix = suffixes.at(filters.indexOf(selectedFilter));
//        if (currentSuffix != filterSuffix) {
//            //удаляем суффикс, если это суффикс известного нам типа файлов
//            if (suffixes.contains(currentSuffix))
//                avgFileName.chop(currentSuffix.length()+1);

//            avgFileName.append(QString(".%1").arg(filterSuffix));
//        }

//        bool isNew = false;
//        avg = App->addFile(avgFileName, &isNew);

//        if (!avg) {
//            qDebug() << "Не удалось создать файл" << avgFileName;
//            return;
//        }

//        if (QFileInfo(avgFileName).exists()) {
//            if (isNew) avg->read();
//        }
//        else
//            avg->fillPreliminary(firstCurve->channel->descriptor());
//    }
//    else {
//        avgFileName = firstCurve->channel->descriptor()->fileName();
//        avg = App->find(avgFileName);
//    }

//    avg->calculateMovingAvg(channels,windowSize);

//    int idx;
//    if (tab->model->contains(avgFileName, &idx)) {
//        tab->model->updateFile(idx);
//    }
//    else {
//        addFile(avg);
//    }
//    setCurrentAndPlot(avg.get(), avg->channelsCount()-1);
}

//соединяется с channelModel
void MainWindow::deleteCurve(int index) /*slot*/
{DD;
//    //не удаляем, если фиксирована
//    if (Curve *c = plot->plotted(tab->record->channel(index)))
//        if (c->fixed) return;

//    //удаляем кривую с графика
//    plot->deleteCurveForChannelIndex(tab->record, index);

//    if (tab->model->selected().size()>1 && QApplication::keyboardModifiers() & Qt::ControlModifier) {
//        auto selectedFiles = tab->model->selectedFiles();
//        for (FileDescriptor *f: qAsConst(selectedFiles)) {
//            if (f == tab->record) continue;
//            if (f->channelsCount()<=index) continue;

//            //не удаляем, если фиксирована
//            if (Curve *c = plot->plotted(f->channel(index))) {
//                if (c->fixed) continue;
//            }

//            plot->deleteCurveForChannelIndex(f, index);
//        }
//    }
//    //cycled.clear();

//    updatePlottedChannelsNumbers();
}

void MainWindow::rescanBase()
{DD;
//    // first we delete all curves affected
//    plot->deleteAllCurves(true);

//    Tab *oldTab = tab;

//    for (int i=0; i<tabWidget->count(); ++i) {
//        if (Tab *t = dynamic_cast<Tab*>(tabWidget->widget(i))) {
//            // next we clear all tab and populate it with folders anew
//            t->channelModel->clear();
//            t->model->clear();
//            t->filePathLabel->clear();
//            tab = t;

//            for (auto folder: qAsConst(t->fileHandler->files)) {
//                addFolder(folder.first, folder.second == FileHandler::FolderWithSubfolders, false);
//            }
//        }
//    }
//    tab = oldTab;
}

void MainWindow::addFile(F descriptor)
{DD;
    if (!currentTab) return;
    if (!descriptor) return;

    addDescriptors(QList<F>()<<descriptor);
}

void MainWindow::setCurrentAndPlot(FileDescriptor *d, int channelIndex)
{DD;
    if (currentTab) {
        if (currentTab->model->contains(d)) {
            updateChannelsTable(d);

            currentTab->channelModel->plotChannels(QVector<int>()<<channelIndex);
        }
    }
}

void MainWindow::updatePlottedChannelsNumbers()
{DD;
    plottedChannelsNumbers.clear();

    if (sergeiMode) {
        for (int i=0, count = currentTab->record->channelsCount(); i<count; ++i)
            if (currentTab->record->channel(i)->plotted()>0)
                plottedChannelsNumbers << i;
    }
}

void MainWindow::previousOrNextDescriptor(bool previous) /*private*/
{DD;
    if (!currentTab || !currentTab->record) return;

    //проверяем, есть ли в табе другие записи
    if (currentTab->model->size() < 2) return;

    bool mode = sergeiMode;
    sergeiMode = true;
    updatePlottedChannelsNumbers();

    QModelIndex current = currentTab->filesTable->selectionModel()->currentIndex();
    int row = current.row();
    QModelIndex index;
    if (previous) {
        if (row == 0)
            index = currentTab->filesTable->model()->index(currentTab->filesTable->model()->rowCount()-1,current.column());
        else
            index = currentTab->filesTable->model()->index(row-1,current.column());
    }
    else {
        if (row == currentTab->filesTable->model()->rowCount()-1)
            index = currentTab->filesTable->model()->index(0,current.column());
        else
            index = currentTab->filesTable->model()->index(row+1,current.column());
    }
    if (index.isValid())
        currentTab->filesTable->selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Current);
    sergeiMode = mode;
}

void MainWindow::previousDescriptor()
{DD;
    previousOrNextDescriptor(true);
}

void MainWindow::nextDescriptor()
{DD;
    previousOrNextDescriptor(false);
}

void MainWindow::arbitraryDescriptor()
{DD;
    if (!currentTab || !currentTab->record) return;

    //проверяем, есть ли в табе другие записи
    if (currentTab->model->size() < 2) return;

    sergeiMode = !sergeiMode;
    updatePlottedChannelsNumbers();
}

void MainWindow::cycleChannelsUp()
{DD;
    cycleChannelsUpOrDown(true);
}

void MainWindow::cycleChannelsDown()
{DD;
    cycleChannelsUpOrDown(false);
}

void MainWindow::cycleChannelsUpOrDown(bool up) /*private*/
{DD;
    if (!currentTab || !currentTab->record) return;

    bool mode = sergeiMode;
    sergeiMode = true;
    updatePlottedChannelsNumbers();

    QVector<int> plotted = plottedChannelsNumbers;

    if (cycled.size() > plotted.size())
        cycled.clear();
    if (cycled.size() < plotted.size())
        qDebug()<<"size mismatch: cycled"<<cycled.size()<<"plotted"<<plotted.size();

    if (plotted.isEmpty()) cycled.clear(); //контроль очистки графика

    if (cycled.isEmpty()) {
        cycled = plotted;
        //удаляем фиксированные каналы
        for (int i=cycled.size()-1; i>=0; --i) {
            if (Channel *ch = currentTab->record->channel(cycled[i]); ch->plotted()>0 && ch->curve->fixed)
                cycled.remove(i);
        }
    }
    if (!cycled.isEmpty()) {
        currentTab->channelModel->deleteCurves();
        if (up) {
            for (int i=0; i<cycled.size(); ++i) {
                if (cycled[i] == 0) cycled[i] = currentTab->record->channelsCount()-1;
                else cycled[i]=cycled[i]-1;
            }
        }
        else {
            for (int i=cycled.size()-1; i>=0; --i) {
                if (cycled[i] == currentTab->record->channelsCount()-1) cycled[i] = 0;
                else cycled[i]=cycled[i]+1;
            }
        }
        currentTab->channelModel->plotChannels(cycled);
    }
    sergeiMode = mode;
}

void MainWindow::exportChannelsToWav()
{DD;
    if (!currentTab || !currentTab->record) return;
    //проверяем тип файла
    if (!currentTab->record->isSourceFile()) return;

    QVector<int> toExport = currentTab->channelModel->selected();
    if (toExport.isEmpty()) return;

    WavExportDialog dialog(currentTab->record, toExport, this);
    dialog.exec();
}

void MainWindow::updateActions()
{DD;
    if (!currentTab || !currentTab->model ||!currentTab->channelModel) return;

    const int selectedChannelsCount = currentTab->channelModel->selected().size();
    const int selectedFilesCount = currentTab->model->selected().size();
    const int channelsCount = currentTab->channelModel->channelsCount;
    const int filesCount = currentTab->model->size();
    const bool hasCurves = currentPlot ? currentPlot->curvesCount():false;
    const bool spectrogram = currentPlot ? currentPlot->type()==PlotType::Spectrogram:false;

    saveAct->setEnabled(currentTab->model->changed());

    renameAct->setDisabled(selectedFilesCount==0);
    delFilesAct->setDisabled(selectedFilesCount==0);
    plotAllChannelsAct->setDisabled(selectedFilesCount==0);
    plotAllChannelsOnRightAct->setDisabled(selectedFilesCount==0);
    plotSelectedChannelsAct->setDisabled(selectedChannelsCount==0);
    editChannelDescriptionsAct->setDisabled(selectedChannelsCount==0);
    //exportChannelsToWavAct;
    const auto timeFiles = currentTab->model->selectedFiles({Descriptor::TimeResponse});
    calculateSpectreAct->setDisabled(timeFiles.isEmpty());
    calculateSpectreDeepSeaAct->setDisabled(timeFiles.isEmpty());

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
                currentTab->model->selectedFiles(types).isEmpty());
    rescanBaseAct->setEnabled(filesCount>0);
    //QAction *switchCursorAct;
    //trackingCursorAct->setEnabled(!spectrogram);

    //QAction *editColorsAct;

    if (currentPlot) meanAct->setDisabled(currentPlot->curvesCount()<2);
    movingAvgAct->setEnabled(hasCurves && !spectrogram);
    //QAction *interactionModeAct;
    addCorrectionAct->setEnabled(hasCurves && !spectrogram);
    addCorrectionsAct->setEnabled(hasCurves && !spectrogram);
    deleteChannelsAct->setDisabled(selectedChannelsCount==0);
    deleteChannelsBatchAct->setDisabled(selectedChannelsCount==0);
    copyChannelsAct->setDisabled(selectedChannelsCount==0);
    moveChannelsAct->setDisabled(selectedChannelsCount==0);
    copyPlottedChannelsAct->setEnabled(hasCurves);
    movePlottedChannelsAct->setEnabled(hasCurves);
    deletePlottedChannelsAct->setEnabled(hasCurves);

    moveChannelsDownAct->setEnabled(selectedChannelsCount > 0 && selectedChannelsCount < channelsCount);
    moveChannelsUpAct->setEnabled(selectedChannelsCount > 0 && selectedChannelsCount < channelsCount);
    editDescriptionsAct->setDisabled(selectedFilesCount==0);

    exportToExcelAct->setEnabled(hasCurves && !spectrogram);
    exportChannelsToWavAct->setEnabled(!timeFiles.isEmpty() && selectedChannelsCount>0);

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
    editYNameAct->setDisabled(selectedChannelsCount==0);
    if (currentPlot) currentPlot->updateActions(filesCount, channelsCount);
}

void MainWindow::renameDescriptor()
{DD;
    if (!currentTab) return;

    auto indexes = currentTab->model->selected();
    if (indexes.isEmpty()) return;

    F file = currentTab->model->file(indexes.first());
    QFileInfo fi(file->fileName());
    QString newName = QInputDialog::getText(this, "Переименование файла",
                                            "Введите новое имя файла",
                                            QLineEdit::Normal,
                                            fi.fileName());
    if (newName.isEmpty() || newName == fi.fileName()) return;

    if (!file->rename(newName)) QMessageBox::warning(this,"DeepSea Base", "Не удалось переименовать файл");
}

void MainWindow::exportToExcel()
{DD;
    if (currentPlot) currentPlot->exportToExcel(false, false);
}

void MainWindow::exportToExcelFull()
{DD;
    if (currentPlot) currentPlot->exportToExcel(true, false);
}

void MainWindow::exportToExcelData()
{DD;
    if (currentPlot) currentPlot->exportToExcel(false, true);
}

//QStringList twoStringDescription(const DescriptionList &list)
//{
//    QStringList result;
//    if (list.size()>0) result << descriptionEntryToString(list.constFirst()); else result << "";
//    if (list.size()>1) result << descriptionEntryToString(list.at(1)); else result << "";
//    return result;
//}

void MainWindow::onCurveColorChanged(Curve *curve)
{DD;
    if (currentTab->record == curve->channel->descriptor()) {
        currentTab->channelModel->onCurveChanged(curve->channel);
//        updateChannelsTable(curve->channel->descriptor());
    }
}

void MainWindow::onCurveDeleted(Channel *channel)
{DD;
    channel->setPlotted(0);
    channel->setColor(QColor());

    currentTab->model->invalidateCurve(channel);
    if (currentTab->record == channel->descriptor())
        currentTab->channelModel->onCurveChanged(channel);
}

void MainWindow::addDescriptors(const QList<F> &files, bool silent)
{DD;
    if (currentTab) currentTab->model->addFiles(files, silent);
}

bool MainWindow::addFiles(const QStringList &files, bool silent)
{DD;
    if (files.isEmpty()) return false;
    if (!currentTab) return false;
    LongOperation op;

    QList<F> items;

    for (const QString &fileName: files) {
        if (fileName.isEmpty()) continue;

        if (currentTab->model->contains(fileName))
            continue;

        emit loading(fileName);

        bool isNew = false;
        F file = App->addFile(fileName, &isNew);
        if (file) {
            if (isNew) file->read();
            items << file;
        }
    }
    addDescriptors(items, silent);
    return !items.isEmpty();
}

void MainWindow::closeEvent(QCloseEvent *event)
{DD;
    if (closeRequested())
        event->accept();
    else
        event->ignore();
}

bool MainWindow::closeRequested()
{DD;
    //определяем, были ли изменения
    bool changed = false;
    if (tabWidget) {
        for (int i=0; i<tabWidget->count(); ++i) {
            if (Tab *t = qobject_cast<Tab *>(tabWidget->widget(i)); t->model->changed()) {
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
        else if (msgBox.clickedButton() == discB) {
            for (int i=0; i<tabWidget->count(); ++i) {
                if (Tab *t = qobject_cast<Tab *>(tabWidget->widget(i))) {
                    t->model->discardChanges();
                }
            }
        }
    }

    if (currentTab) {
        App->setSetting("upperSplitterState",currentTab->saveState());
        QByteArray treeHeaderState = currentTab->filesTable->header()->saveState();
        App->setSetting("treeHeaderState", treeHeaderState);
    }

    if (tabWidget) {
        QVariantMap map;
        for (int i=0; i<tabWidget->count(); ++i) {
            if (Tab *t = qobject_cast<Tab *>(tabWidget->widget(i))) {
                if (auto folders = t->fileHandler->fileNames(); !folders.isEmpty())
                    map.insert(tabWidget->tabText(i), folders);
                currentTab->filterHeader->clear();
            }
        }

        App->setSetting("folders1", map);

        for (int i= tabWidget->count()-1; i>=0; --i) {
            QWidget *w = tabWidget->widget(i);
            tabWidget->removeTab(i);
            w->deleteLater();
        }
    }

    return true;
}
