#include "mainwindow.h"

#include <QtWidgets>
#include <QtConcurrent/QtConcurrentRun>

#include "deepseaprocessordialog.h"
#include "filesprocessordialog.h"
#include "sortabletreewidgetitem.h"
#include "headerview.h"
#include "plot/plot.h"
#include "coloreditdialog.h"
#include "plot/curve.h"
#include "model.h"
#include "sortfiltermodel.h"
#include "filterheaderview.h"
#include "wavexportdialog.h"
#include "methods/averaging.h"

#include <ActiveQt/ActiveQt>
#include "logging.h"

#include "editdescriptionsdialog.h"
#include "converters/matlabconvertor.h"
#include "converters/matlabconverterdialog.h"
#include "converters/esoconverterdialog.h"
#include "converters/anaconverterdialog.h"
#include "converters/converterdialog.h"
#include "timeslicer.h"
#include <QTime>
#include "channeltablemodel.h"
#include "htmldelegate.h"
#include "longoperation.h"
#include "filestable.h"
#include "channelstable.h"

#include "fileformats/abstractformatfactory.h"
#include "descriptorpropertiesdialog.h"
#include "channelpropertiesdialog.h"
#include "tab.h"
#include "filehandler.h"
#include "filehandlerdialog.h"
#include "plot/plotarea.h"
#include "plot/plotmodel.h"
#include "stepitemdelegate.h"
#include "dfdfilterproxy.h"

#include "DockManager.h"
#include "DockAreaWidget.h"
#include "DockWidget.h"
#include "DockAreaTitleBar.h"
#include "DockAreaTabBar.h"
#include "DockComponentsFactory.h"
#include "DockWidgetTab.h"
#include "tabdockfactory.h"
#include "plotdockfactory.h"
#include "plugins/convertplugin.h"
#include "settings.h"
#include "methods/calculations.h"
#include "settingsdialog.h"
#include "version.h"
#include "plot/qcpplot.h"

void checkVersions()
{
    QDir distrDir("\\\\192.168.1.6\\Disk 2\\Distributives\\Work\\УЗКС ФП\\DeepSea Database");
    auto distrList = distrDir.entryInfoList(QDir::Files);

    std::string thisVersion = DEEPSEABASE_VERSION;

    //определяем максимальную версию, выложенную в папке
    for (auto dist: distrList) {
        //DeepSeaBaseInstall-2.1.6-12.07.2023-x64
        QString name = dist.fileName();
        if (!name.startsWith("DeepSeaBaseInstall-")) continue;
        std::string version = name.split("-").at(1).toStdString();
        auto c = CompareVersions(version, thisVersion);
        if (c > 0) {
            thisVersion = version;
        }
    }

    auto c = CompareVersions(thisVersion, DEEPSEABASE_VERSION);
    if (c > 0) {
        QMessageBox::information(0, "DeepSea Database", QString("В папке \"%1\" уже лежит более новая версия программы - %2")
                                 .arg(distrDir.path())
                                 .arg(QString::fromStdString(thisVersion)));
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{DD;
    setWindowTitle(tr("DeepSea Database ")+DEEPSEABASE_VERSION);

    setDocumentMode(true);

    ads::CDockManager::setConfigFlags(ads::CDockManager::DefaultNonOpaqueConfig);
    ads::CDockManager::setConfigFlag(ads::CDockManager::DockAreaHasCloseButton, false);
    ads::CDockManager::setConfigFlag(ads::CDockManager::DockAreaHasUndockButton, false);
    ads::CDockManager::setConfigFlag(ads::CDockManager::FocusHighlighting, true);
    ads::CDockManager::setConfigFlag(ads::CDockManager::EqualSplitOnInsertion, true);
    ads::CDockManager::setConfigFlag(ads::CDockManager::XmlCompressionEnabled, false);

    m_DockManager = new ads::CDockManager(this);

    auto state = m_DockManager->saveState();
    QFile adsStyle("ads.css");
    if (adsStyle.open(QFile::ReadOnly | QFile::Text)) {
        m_DockManager->setStyleSheet(adsStyle.readAll());
    }
    else {
        LOG(DEBUG) << QString("Не удалось открыть файл стиля для ADS.");
    }
    m_DockManager->restoreState(state);

    connect(m_DockManager, &ads::CDockManager::focusedDockWidgetChanged, this, &MainWindow::onFocusedDockWidgetChanged);
    connect(m_DockManager, &ads::CDockManager::dockWidgetAboutToBeRemoved, this, &MainWindow::dockWidgetAboutToBeRemoved);

    createActions();

    App->loadPlugins();

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
    mainToolBar->addAction(addPlotAreaAct);

    QWidget *fillerWidget = new QWidget(this);
    fillerWidget->setContentsMargins(0,0,0,0);
    fillerWidget->setSizePolicy(QSizePolicy::Expanding, fillerWidget->sizePolicy().verticalPolicy());
    mainToolBar->addWidget(fillerWidget);
    mainToolBar->addAction(plotHelpAct);


    QMenu *fileMenu = menuBar()->addMenu(tr("Записи"));
    fileMenu->addAction(addFolderAct);
    fileMenu->addAction(addFileAct);
    fileMenu->addAction(delFilesAct);
    fileMenu->addAction(rescanBaseAct);
    fileMenu->addAction(saveAct);
    fileMenu->addAction(renameAct);

    QMenu *convertMenu = menuBar()->addMenu(QString("Конвертеры"));
    convertMenu->addAction(convertAct);
    convertMenu->addAction(convertMatFilesAct);
    convertMenu->addAction(convertEsoFilesAct);
    convertMenu->addAction(convertAnaFilesAct);
    createConvertPluginsMenu(convertMenu);

    QMenu *settingsMenu = menuBar()->addMenu(QString("Настройки"));
    settingsMenu->addAction(settingsAct);
    settingsMenu->addAction(editColorsAct);
    settingsMenu->addAction(aboutAct);

    tabsMenu = menuBar()->addMenu("Вкладки");
    tabsMenu->addAction(addTabAct);

    plotsMenu = menuBar()->addMenu("Графики");
    plotsMenu->addAction(addPlotAreaAct);

    addPlotArea();

    QVariantMap v = se->getSetting("folders1").toMap();

    if (v.isEmpty())
        createNewTab();
    else {
        for (const auto [key,val]: asKeyValueRange(v)) {
            createTab(key, val.toStringList());
            tabsNames << key;
        }
    }

    auto splitterSizes = se->fromList(se->getSetting("centralSplitter").toList());
    if (!splitterSizes.isEmpty()) {
        m_DockManager->setSplitterSizes(topArea, splitterSizes);
    }

    updateActions();

    QtConcurrent::run(checkVersions);
}

void MainWindow::dockWidgetAboutToBeRemoved(ads::CDockWidget* dockWidget)
{
    if (auto tab = qobject_cast<Tab *>(dockWidget->widget())) {
        if (currentTab == tab) currentTab = nullptr;
    }
    else if (auto plot = qobject_cast<PlotArea*>(dockWidget)) {
        if (currentPlot == plot) currentPlot = nullptr;
    }
}

void MainWindow::createActions()
{DD;

    settingsAct = new QAction("Настройки", this);
    connect(settingsAct, &QAction::triggered, [this](){
        SettingsDialog dialog(this);
        if (dialog.exec()) {

        }
    });
    addTabAct = new QAction("Новая вкладка", this);
    addTabAct->setShortcut(QKeySequence::AddTab);
    connect(addTabAct, &QAction::triggered, this, &MainWindow::createNewTab);

    QIcon newPlotIcon(":/icons/newPlot.png");
    newPlotIcon.addFile(":/icons/newPlot24.png");
    addPlotAreaAct = new QAction(newPlotIcon, "Добавить график справа", this);
    connect(addPlotAreaAct, SIGNAL(triggered()), this, SLOT(addPlotArea()));

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
    //moveChannelsUpAct->setShortcut(tr("Ctrl+Up"));
    connect(moveChannelsUpAct, SIGNAL(triggered()), SLOT(moveChannelsUp()));

    QIcon moveChannelsDownIcon(":/icons/move_down.png");
    moveChannelsDownIcon.addFile(":/icons/move_down_24.png");
    moveChannelsDownAct = new QAction(moveChannelsDownIcon, "Сдвинуть каналы вниз", this);
    //moveChannelsDownAct->setShortcut(tr("Ctrl+Down"));
    connect(moveChannelsDownAct, SIGNAL(triggered()), SLOT(moveChannelsDown()));

    ctrlUpAct = new QAction(this);
    ctrlUpAct->setShortcut(Qt::CTRL+Qt::Key_Up);
    ctrlUpAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(ctrlUpAct, &QAction::triggered, this, &MainWindow::ctrlUpTriggered);
    addAction(ctrlUpAct);

    ctrlDownAct = new QAction(this);
    ctrlDownAct->setShortcut(Qt::CTRL+Qt::Key_Down);
    ctrlDownAct->setShortcutContext(Qt::ApplicationShortcut);
    connect(ctrlDownAct, &QAction::triggered, this, &MainWindow::ctrlDownTriggered);
    addAction(ctrlDownAct);

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

    exportChannelsToWavAct = new QAction(QString("Экспортировать в WAV"), this);
    exportChannelsToWavAct->setIcon(QIcon(":/icons/wav.ico"));
    connect(exportChannelsToWavAct, SIGNAL(triggered(bool)), SLOT(exportChannelsToWav()));

    plotHelpAct = new QAction(QIcon(":/icons/help.png"), "Справка", this);
    connect(plotHelpAct, &QAction::triggered, [](){QDesktopServices::openUrl(QUrl("help.html"));});

    aboutAct = new QAction("О программе", this);
    connect(aboutAct, &QAction::triggered, [](){
        QString version = QString("DeepSea Database - версия %1\n").arg(DEEPSEABASE_VERSION);
        version.append(sizeof(int*) == sizeof(qint32) ? "32-bit версия" : "64-bit версия");
        QMessageBox::about(nullptr, tr("О программе"), version);
    });

    QIcon calculateSpectreIcon(":/icons/function.png");
    calculateSpectreIcon.addFile(":/icons/function16.png");
    calculateSpectreAct = new QAction(calculateSpectreIcon, QString("Рассчитать спектры..."), this);
    connect(calculateSpectreAct, &QAction::triggered, [=](){
        calculateSpectreRecords();
    });

    calculateSpectreAct1 = new QAction(calculateSpectreIcon, QString("Рассчитать спектры..."), this);
    connect(calculateSpectreAct1, &QAction::triggered, [=](){
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

    rescanBaseAct = new QAction(QString("Пересканировать базу"), this);
    rescanBaseAct->setIcon(QIcon(":/icons/revert.png"));
    rescanBaseAct->setShortcut(QKeySequence::Refresh);
    connect(rescanBaseAct, SIGNAL(triggered()), this, SLOT(rescanBase()));

    exportToExcelAct = new QAction(QString("Экспортировать в Excel"), this);
    exportToExcelAct->setIcon(QIcon(":/icons/excel.png"));
    connect(exportToExcelAct, SIGNAL(triggered()), this, SLOT(exportToExcel()));
    QMenu *excelMenu = new QMenu(this);
    exportToExcelFullAct = new QAction("Экспортировать в Excel весь диапазон", this);
    connect(exportToExcelFullAct, SIGNAL(triggered()), this, SLOT(exportToExcelFull()));
    excelMenu->addAction(exportToExcelFullAct);
    exportToExcelOnlyDataAct = new QAction("Экспортировать в Excel только данные", this);
    connect(exportToExcelOnlyDataAct, SIGNAL(triggered()), this, SLOT(exportToExcelData()));
    excelMenu->addAction(exportToExcelOnlyDataAct);
    exportToClipboardAct = new QAction("Копировать данные в буфер обмена", this);
    connect(exportToClipboardAct, SIGNAL(triggered()), this, SLOT(exportToClipboard()));
    excelMenu->addAction(exportToClipboardAct);
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

    deleteChannelsOneAct = new QAction("Удалить выделенные каналы...",this);
    deleteChannelsOneAct->setIcon(QIcon(":/icons/remove.png"));
    connect(deleteChannelsOneAct, SIGNAL(triggered()), this, SLOT(deleteChannels()));

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

    convertEsoFilesAct = new QAction("Конвертировать файлы ESO...", this);
    connect(convertEsoFilesAct,SIGNAL(triggered()),SLOT(convertEsoFiles()));

    convertAnaFilesAct = new QAction("Конвертировать файлы ANA/ANP...", this);
    connect(convertAnaFilesAct,SIGNAL(triggered()),SLOT(convertAnaFiles()));
}

void MainWindow::createConvertPluginsMenu(QMenu *menu)
{DD;
    if (!App->convertPlugins.isEmpty())
        menu->addSeparator();

    QSignalMapper *mapper = new QSignalMapper(this);
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
    connect(mapper, &QSignalMapper::mappedString, this, &MainWindow::onPluginTriggered);
#else
    connect(mapper, SIGNAL(mapped(QString)), this, SLOT(onPluginTriggered(QString)));
#endif
    for (int it = 0; it < App->convertPlugins.size(); ++it) {
        QJsonObject metaData = App->convertPlugins.at(it);
        QString pluginInterface=metaData.value(("interface")).toString();
        if (pluginInterface=="IConvertPlugin") {
            QIcon icon(metaData.value("icon").toString());
            QString text = metaData.value("text").toString();
            QString tooltip = metaData.value("description").toString();
            if (text.isEmpty()) text = tooltip;
            QString key = metaData.value("path").toString();
            QAction *a = new QAction(icon, text, this);
            a->setProperty("id", it);
            a->setToolTip(tooltip);
            connect(a, SIGNAL(triggered()), mapper, SLOT(map()));
            mapper->setMapping(a, key);
            menu->insertAction(0, a);
        }
    }
}

void MainWindow::createTab(const QString &name, const QStringList &folders)
{DD;
    auto newTab = new Tab(this);
    currentTab = newTab;

    newTab->filesTable->addAction(delFilesAct);
    newTab->filesTable->addAction(saveAct);

    newTab->addParentAction("addFolder", addFolderAct);
    newTab->addParentAction("addFile", addFileAct);
    newTab->addParentAction("deleteFiles", delFilesAct);
    newTab->addParentAction("calculateSpectre", calculateSpectreAct1);
    newTab->addParentAction("calculateSpectreDeepSea", calculateSpectreDeepSeaAct);
    newTab->addParentAction("convert", convertAct);
    newTab->addParentAction("rename", renameAct);

    newTab->channelsTable->addAction("exportWav", exportChannelsToWavAct);
    newTab->channelsTable->addAction("moveUp", moveChannelsUpAct);
    newTab->channelsTable->addAction("moveDown", moveChannelsDownAct);
    newTab->channelsTable->addAction("delete", deleteChannelsOneAct);
    newTab->channelsTable->addAction("deleteBatch", deleteChannelsBatchAct);
    newTab->channelsTable->addAction("copy", copyChannelsAct);
    newTab->channelsTable->addAction("move", moveChannelsAct);

    //сигнал updateLegends() передается всем имеющимся графикам
    connect(newTab->model, SIGNAL(legendsChanged()), this, SIGNAL(updateLegends()));
    connect(newTab->channelModel, SIGNAL(legendsChanged()), this, SIGNAL(updateLegends()));
    connect(newTab->model, &Model::plotNeedsUpdate, this, &MainWindow::updatePlotAreas);

    //пересылается в графики для передвижения по записям в режиме Сергея
    connect(newTab, &Tab::descriptorChanged, this, &MainWindow::descriptorChanged);
    connect(newTab, &Tab::needPlotChannels, this, qOverload<bool,const QVector<Channel*> &,bool>(&MainWindow::onChannelsDropped));
    connect(newTab, &Tab::needUnplotChannel, this, &MainWindow::unplotChannel);

    ads::CDockComponentsFactory::setFactory(new TabDockFactory(this));
    auto dockWidget = new ads::CDockWidget(name);
    dockWidget->setWidget(newTab);
    dockWidget->setFeature(ads::CDockWidget::DockWidgetFloatable, false);
    dockWidget->setFeature(ads::CDockWidget::DockWidgetDeleteOnClose, true);
    dockWidget->setFeature(ads::CDockWidget::DockWidgetFocusable, true);
    dockWidget->setFeature(ads::CDockWidget::CustomCloseHandling, true);

    if (!topArea)
        topArea = m_DockManager->addDockWidget(ads::TopDockWidgetArea, dockWidget);
    else
        m_DockManager->addDockWidget(ads::CenterDockWidgetArea, dockWidget, topArea);

    connect(dockWidget, &ads::CDockWidget::closeRequested, [=](){
        closeTab(dockWidget);
    });
    tabsMenu->addAction(dockWidget->toggleViewAction());

    connect(newTab->fileHandler, qOverload<const QString &,bool,bool>(&FileHandler::fileAdded),
            this, qOverload<const QString &,bool,bool>(&MainWindow::addFolder));
    newTab->fileHandler->setFileNames(folders);
    LOG(INFO) << QString("Создана вкладка ")<<name;
}

void MainWindow::createNewTab()
{DD;
    int sequenceNumber = 1;
    QString name = tr("Вкладка %1").arg(sequenceNumber);
    while (tabsNames.contains(name)) {
        sequenceNumber++;
        name = tr("Вкладка %1").arg(sequenceNumber);
    }
    createTab(name, QStringList());
    tabsNames << name;
}

void MainWindow::closeTab(ads::CDockWidget *t)
{DD;
    if (!t) return;
    auto tab = dynamic_cast<Tab*>(t->widget());
    if (!tab) return;

    int tabsLeft = 0;
    auto m = m_DockManager->dockWidgetsMap().values();
    for (const auto &w : m) {
        if (PlotArea *area = dynamic_cast<PlotArea*>(w)) {
            for (int i=0; i<tab->model->size(); ++i) {
                const F f = tab->model->file(i);
                //use_count==2 if file is only in one tab, (1 for model, 1 for the prev line)
                //use_count>=3 if file is in more than one tab
                if (f.use_count()<=2) {
                    area->deleteCurvesForDescriptor(f.get());
                }
            }
        }
        else if (dynamic_cast<Tab*>(w->widget()))
            tabsLeft++;
    }
    currentTab = nullptr;
    auto name = t->tabWidget()->text();

    t->closeDockWidget();

    //этог была последняя вкладка
    if (tabsLeft==1) topArea = nullptr;
    LOG(INFO) << QString("Закрыта вкладка ")<<name;
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

//    auto dockManagerState = m_DockManager->saveState();
//    LOG(DEBUG) << dockManagerState;

    se->setSetting("centralSplitter", se->toList(m_DockManager->splitterSizes(topArea)));

    //вручную удаляем все панели с графиками, чтобы корректно удалились кривые. Иначе
    //dockManager не гарантирует последовательность удаления панелей, и при удалении
    //графика после вкладки с файлами программа падала
    const auto m = m_DockManager->dockWidgetsMap().values();
    for (const auto &w: m) {
        if (PlotArea *area = dynamic_cast<PlotArea*>(w)) {
            area->closeDockWidget();
        }
    }
    const auto m1 = m_DockManager->floatingWidgets();
    for (const auto &container: m1) {
        for (auto w: container->dockWidgets()) {
            if (PlotArea *area = dynamic_cast<PlotArea*>(w)) {
                area->closeDockWidget();
            }
        }
    }

}

PlotArea *MainWindow::createPlotArea()
{DD;
    PlotDockFactory * defaultFactory = new PlotDockFactory();
    defaultFactory->setReceiver(this);
    ads::CDockComponentsFactory::setFactory(defaultFactory);

    static int plotIndex = 0;
    plotIndex++;

    PlotArea *area = new PlotArea(plotIndex, this);
    //это соединение позволяет обновить сразу все вкладки графиков
    connect(this, SIGNAL(updateLegends()), area, SIGNAL(updateLegends()));

    connect(area, &PlotArea::needPlotChannels, this, qOverload<bool,const QVector<Channel*> &>(&MainWindow::onChannelsDropped));
    connect(area, &PlotArea::curvesCountChanged, this, &MainWindow::updateActions);
    connect(area, &PlotArea::channelPlotted, this, &MainWindow::onChannelChanged);
    connect(area, &PlotArea::curveDeleted, this, &MainWindow::onChannelChanged);
    connect(area, &ads::CDockWidget::closeRequested, [=](){closePlot(area);});
    connect(area, &PlotArea::descriptorRequested, this, &MainWindow::setDescriptor);
    connect(area, &PlotArea::saveHorizontalSlice, this, &MainWindow::saveHorizontalSlice);
    connect(area, &PlotArea::saveVerticalSlice, this, &MainWindow::saveVerticalSlice);
    connect(area, &PlotArea::saveTimeSegment, this, &MainWindow::saveTimeSegment);
    connect(this, &MainWindow::descriptorChanged, area, &PlotArea::replotDescriptor);
    connect(this, &MainWindow::updatePlotAreas, area, &PlotArea::updatePlot);

    plotsMenu->addAction(area->toggleViewAction());
    //в текущей вкладке графика еще нет самого графика
    setCurrentPlot(nullptr);
    updateActions();
    return area;
}

void MainWindow::setCurrentPlot(QCPPlot *plot)
{
    const auto m = m_DockManager->dockWidgetsMap().values();
    for (const auto &w: m) {
        if (Tab *t = qobject_cast<Tab *>(w->widget())) {
            t->setCurrentPlot(plot);
        }
    }
}

void MainWindow::addPlotArea()
{DD;
    auto p = createPlotArea();

    if (!bottomArea)
        bottomArea = m_DockManager->addDockWidget(ads::BottomDockWidgetArea, p);
    else
        m_DockManager->addDockWidget(ads::RightDockWidgetArea, p, bottomArea);
    LOG(INFO) << QString("Добавлен график ")<< p->tabWidget()->text();
    currentPlot = p;
}

void MainWindow::addPlotTabbed()
{DD;
    if (!currentPlot) addPlotArea();
    else {
        auto area = currentPlot->dockAreaWidget();
        currentPlot = createPlotArea();
        m_DockManager->addDockWidgetTabToArea(currentPlot, area);
    }
    LOG(INFO) << QString("Добавлен график ")<< currentPlot->tabWidget()->text();
}

void MainWindow::closePlot(ads::CDockWidget *t)
{DD;
    if (!t) return;

    currentPlot = nullptr;
    setCurrentPlot(nullptr);
    auto name = t->tabWidget()->text();

    t->closeDockWidget();

    //проверяем, остались ли не плавающие графики
    bool plotsLeft = false;
    const auto m = m_DockManager->dockWidgetsMap().values();
    for (const auto &w: m) {
        if (dynamic_cast<PlotArea*>(w)) {
            plotsLeft = true;
            break;
        }
    }
    if (!plotsLeft) bottomArea = nullptr;
    LOG(INFO) << QString("Закрыт график ")<<name;
}

void MainWindow::closeOtherPlots(int index)
{DD;
    if (!currentPlot) return;

    int count = currentPlot->dockAreaWidget()->dockWidgetsCount();
    if (count<=1) return;

    for (int i = count - 1; i > index; --i)
        closePlot(currentPlot->dockAreaWidget()->dockWidget(i));
    for (int i = index - 1; i >= 0; --i)
        closePlot(currentPlot->dockAreaWidget()->dockWidget(i));
}

QString MainWindow::getFolderToAdd(bool withSubfolders)
{DD;
    QString directory = se->getSetting("lastDirectory").toString();

    directory = QFileDialog::getExistingDirectory(this,
                                                  withSubfolders?tr("Добавление папки со всеми вложенными папками"):
                                                  tr("Добавление папки"),
                                                  directory,
                                                  QFileDialog::ShowDirsOnly | QFileDialog::ReadOnly);

    if (!directory.isEmpty())
        se->setSetting("lastDirectory", directory);
    return directory;
}

void MainWindow::addFolder() /*SLOT*/
{DD;
    if (!currentTab) return;
    if (auto folder = getFolderToAdd(false /*with subfolders*/); addFolder(folder, false /*with subfolders*/, false /*silent*/)) {
        currentTab->fileHandler->trackFolder(folder, false);
        LOG(INFO) << QString("Добавлена папка ")<<folder;
        saveTabsState();
    }
}

void MainWindow::addFolderWithSubfolders() /*SLOT*/
{DD;
    if (!currentTab) return;
    if (auto folder = getFolderToAdd(true /*with subfolders*/); addFolder(folder, true /*with subfolders*/, false /*silent*/)) {
        currentTab->fileHandler->trackFolder(folder, true);
        LOG(INFO) << QString("Добавлена папка ")<<folder;
        saveTabsState();
    }
}

void MainWindow::addFile()
{DD;
    if (!currentTab) return;
    QString directory = se->getSetting("lastDirectory").toString();

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
        se->setSetting("lastDirectory", fileNames.constFirst());
        if (addFiles(fileNames)) {
            currentTab->fileHandler->trackFiles(fileNames);
            saveTabsState();
        }
    }
}

bool MainWindow::addFolder(const QString &directory, bool withAllSubfolders, bool silent)
{DD;
    if (directory.isEmpty() || !currentTab) return false;

    QStringList filesToAdd;
    processDir(directory, filesToAdd, withAllSubfolders, App->formatFactory->allSuffixes());
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
                                                 QString("Только %1 файлов из %2 было добавлено")
                                                 .arg(toAdd.size())
                                                 .arg(filesToAdd.size()));
        LOG(WARNING) << QString("Только %1 файлов из %2 было добавлено")
                        .arg(toAdd.size())
                        .arg(filesToAdd.size());
    }
    return true;
}

void MainWindow::deleteFiles()
{DD;
    if (!currentTab) return;

    const auto m = m_DockManager->dockWidgetsMap().values();
    for (const auto &w: m) {
        if (PlotArea *area = dynamic_cast<PlotArea*>(w)) {
            const auto indexes = currentTab->model->selected();
            for (int i: indexes) {
                const F f = currentTab->model->file(i);
                //use_count==2 if file is only in one tab, (1 for model, 1 for the prev line)
                //use_count>=3 if file is in more than one tab
                if (f.use_count()<=2) {
                    area->deleteCurvesForDescriptor(f.get());
                }
                currentTab->fileHandler->untrackFile(f->fileName());
            }
        }
    }

    currentTab->channelModel->clear();

    currentTab->model->deleteSelectedFiles();

    if (currentTab->model->size() == 0)
        currentTab->fileHandler->clear();
    saveTabsState();
}

void MainWindow::deleteChannels() /** SLOT */
{DD;
    if (!currentTab) return;

    QVector<int> channelsToDelete = currentTab->channelModel->selected();
    if (channelsToDelete.isEmpty()) return;

    if (QMessageBox::question(this,"DeepSea Base",
                              QString("Выделенные %1 каналов будут \nудалены из записи. Продолжить?").arg(channelsToDelete.size())
                              )==QMessageBox::Yes) {
        LongOperation op;
        deleteChannels(currentTab->record, channelsToDelete);
        currentTab->updateChannelsTable(currentTab->record);
    }
}

void MainWindow::deleteChannelsBatch()
{DD;
    if (!currentTab) return;
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

    currentTab->updateChannelsTable(currentTab->record);
}

void MainWindow::copyChannels() /** SLOT */
{DD;
    if (!currentTab) return;

    QVector<int> channelsToCopy = currentTab->channelModel->selected();
    if (channelsToCopy.isEmpty()) return;

    if (copyChannels(currentTab->record, channelsToCopy)) {
        currentTab->updateChannelsTable(currentTab->record);
    }
}

void MainWindow::moveChannels() /** SLOT */
{DD;
    if (!currentTab) return;

    // сначала копируем каналы, затем удаляем
    QVector<int> channelsToMove = currentTab->channelModel->selected();
    if (channelsToMove.isEmpty()) return;

    if (QMessageBox::question(this,"DeepSea Base","Выделенные каналы будут \nудалены из файла. Продолжить?")==QMessageBox::Yes) {
        if (copyChannels(currentTab->record, channelsToMove)) {
            deleteChannels(currentTab->record, channelsToMove);
            currentTab->updateChannelsTable(currentTab->record);
        }
    }
}

void MainWindow::deletePlottedChannels() /** SLOT*/
{DD;
    if (QMessageBox::question(this,"DeepSea Base",
                              "Построенные каналы будут \nудалены из записей. Продолжить?"
                              )==QMessageBox::Yes) {
        LongOperation op;
        deletePlotted();

        if (currentTab) currentTab->updateChannelsTable(currentTab->record);
    }
}

void MainWindow::copyPlottedChannels() /** SLOT*/
{DD;
    if (!currentPlot) return;
    auto channels = currentPlot->plottedChannels();
    if (!channels.isEmpty()) {
        if (copyChannels(channels))
            if (currentTab) currentTab->updateChannelsTable(currentTab->record);
    }
}

void MainWindow::movePlottedChannels() /** SLOT*/
{DD;
    // сначала копируем каналы, затем удаляем
    if (!currentPlot) return;
    auto channels = currentPlot->plottedChannels();
    if (!channels.isEmpty()) {
        if (QMessageBox::question(this,"DeepSea Base","Построенные каналы будут \nудалены из файлов. Продолжить?")==QMessageBox::Yes) {
            if (copyChannels(channels)) {
                deletePlotted();
                if (currentTab) currentTab->updateChannelsTable(currentTab->record);
            }
        }
    }
}

void MainWindow::addCorrection()
{DD;
    if (!currentPlot || !currentTab) return;
    currentPlot->addCorrection(currentTab->model->selectedFiles());
    currentTab->updateChannelsTable(currentTab->record);
}

void MainWindow::addCorrections()
{DD;
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

        // LOG(DEBUG)<<fileName;
        if (QFile(fileName).exists()) {
            fileName.replace("\\","/");
            FileDescriptor * dfd = App->find(fileName).get();
            bool deleteAfter=false;
            if (!dfd) {
                dfd = App->formatFactory->createDescriptor(fileName);
                dfd->read();
                deleteAfter = true;
            }
            dfd->populate();

            int row = 2;
            bool corrected = false;
            while (1) {
                QAxObject *cells1 = worksheet->querySubObject("Cells(Int,Int)", row, column);
                QString value = cells1->property("Value").toString();
               // LOG(DEBUG)<<value;
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
            }
            if (deleteAfter) delete dfd;
        }

        column++;
    }

    if (currentTab) currentTab->updateChannelsTable(currentTab->record);
    updatePlotAreas();

    delete worksheet;
    delete workbook;
    delete workbooks;

    excel->dynamicCall("Quit()");
    delete excel;

    QMessageBox::information(this,"","Готово!");
}

bool MainWindow::deleteChannels(FileDescriptor *file, const QVector<int> &channelsToDelete)
{DD;
    LongOperation op;
    const auto m = m_DockManager->dockWidgetsMap();
    for (auto w: m.values()) {
        if (auto area = dynamic_cast<PlotArea*>(w)) {
            area->deleteCurvesForDescriptor(file, channelsToDelete);
        }
    }

    file->deleteChannels(channelsToDelete);

    return true;
}

//так как у нас есть только список каналов, которые могут быть из разных записей,
//то сначала определяем, из каких они записей. Затем для каждой записи получаем
//список индексов, и тогда уже удаляем.
bool MainWindow::deletePlotted()
{DD;
    if (!currentPlot) return false;

    auto plotted = currentPlot->plottedDescriptors();

    for (const auto & [d, indexes]: asKeyValueRange(plotted)) {
        if (!deleteChannels(d, indexes)) return false;
    }
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
    QString startFile = se->getSetting("startDir").toString();
    QStringList filters = App->formatFactory->allFilters();

    QFileDialog dialog(this, "Выбор файла для записи каналов", startFile,
                       filters.join(";;"));
    dialog.setOption(QFileDialog::DontUseNativeDialog, true);

    QStringList suffixes = App->formatFactory->allSuffixes(true);


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
        QMessageBox::critical(0, "Копирование каналов",
                              "Каналы несовместимы между собой и не могут быть записаны в файл dfd");
        return false;
    }

    se->setSetting("startDir", file);


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
        DebugPrint(file);
        LOG(ERROR)<<QString("Неизвестный тип файла")<< file;
        return false;
    }

    if (currentTab) {
        if (found) {
            currentTab->model->updateFile(destination.get());
        }
        else {
            addFile(destination);
            currentTab->fileHandler->trackFiles({file});
            saveTabsState();
        }
    }
    return true;
}



void MainWindow::calculateMean()
{DD;
    if (!currentPlot) return;
    if (currentPlot->curvesCount()<2) return;

    QVector<Channel*> channels = currentPlot->plottedChannels();

    bool dataTypeEqual = true;
    bool stepsEqual = true; // одинаковый шаг по оси Х
    bool namesEqual = true; // одинаковые названия осей Y
    bool oneFile = true; // каналы из одного файла
    bool xMinDifferent = false;
    bool writeToSeparateFile = true;
    bool writeToD94 = false;

    Channel *firstChannel = channels.first();
    FileDescriptor *firstDescriptor = firstChannel->descriptor();

    bool allFilesDfd = firstDescriptor->fileName().toLower().endsWith("dfd");
    auto firstChannelFileName = firstDescriptor->fileName();

    for (auto channel: channels) {
        allFilesDfd &= firstChannelFileName.toLower().endsWith("dfd");
        if (firstChannel->data()->xStep() != channel->data()->xStep())
            stepsEqual = false;
        if (firstChannel->yName() != channel->yName())
            namesEqual = false;
        if (firstChannelFileName != channel->descriptor()->fileName())
            oneFile = false;
        if (!firstDescriptor->dataTypeEquals(channel->descriptor()))
            dataTypeEqual = false;
        if (firstChannel->data()->xMin() != channel->data()->xMin())
            xMinDifferent = true;
    }
    if (!dataTypeEqual) {
        int result = QMessageBox::warning(0, "Среднее графиков",
                                          "Графики имеют разный тип. Среднее будет записано в файл d94. Продолжить?",
                                          "Да","Нет");
        if (result == 1) return;
        else writeToD94 = true;
    }
    if (!namesEqual) {
        int result = QMessageBox::warning(0, "Среднее графиков",
                                          "Графики по-видимому имеют разный тип. Среднее будет записано в файл d94. Продолжить?",
                                          "Да", "Нет");
        if (result == 1) return;
        else writeToD94 = true;
    }
    if (!stepsEqual) {
        QMessageBox::critical(0, "Среднее графиков", "Графики имеют разный шаг по оси X. Прерываю");
        return;

//        int result = QMessageBox::warning(0, "Среднее графиков",
//                                          "Графики имеют разный шаг по оси X. Среднее будет записано в файл d94. Продолжить?",
//                                          "Да", "Нет");
//        if (result == 1) return;
//        else writeToD94 = true;
    }
    if (xMinDifferent) {
        QMessageBox::critical(0, "Среднее графиков", "Графики имеют разное начальное значение по оси X. Прерываю");
        return;
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
        if (writeToD94) meanD.append(".d94");

        meanD = se->getSetting(writeToD94?"lastMeanUffFile":"lastMeanFile", meanD).toString();

        QStringList  filters = App->formatFactory->allFilters();
        QStringList suffixes = App->formatFactory->allSuffixes(true);

        QFileDialog dialog(this, "Выбор файла для записи каналов", meanD,
                           filters.join(";;"));
        dialog.setOption(QFileDialog::DontUseNativeDialog, true);
        dialog.setFileMode(QFileDialog::AnyFile);

        if (!writeToD94) {
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
            LOG(ERROR)<<QString("Не удалось создать файл")<<meanFileName;
            return;
        }

        if (fileExists) {
            if (isNew) meanFile->read();
        }
        else
            meanFile->fillPreliminary(firstDescriptor);

        se->setSetting(writeToD94?"lastMeanUffFile":"lastMeanFile", meanFileName);
    }
    else {
        meanFileName = firstChannelFileName;
        meanFile = App->find(meanFileName);
    }

    ::calculateMean(meanFile.get(), channels.toList());

    int idx;
    if (currentTab && currentTab->model->contains(meanFile, &idx)) {
        currentTab->model->updateFile(idx);
        currentTab->updateChannelsTable(currentTab->record);
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
}

void MainWindow::save()
{DD;
    const auto m = m_DockManager->dockWidgetsMap().values();
    for (const auto &w: m) {
        if (Tab *t = dynamic_cast<Tab*>(w->widget())) {
            t->model->save();
        }
    }
}

void MainWindow::convertMatFiles()
{DD;
    MatlabConverterDialog dialog(this);
    connect(&dialog, &MatlabConverterDialog::filesConverted, [=](const QStringList &files){
        this->addFiles(files);
        if (currentTab) currentTab->fileHandler->trackFiles(files);
        saveTabsState();
    });
    dialog.exec();
}

void MainWindow::onPluginTriggered(const QString &pluginKey)
{DD;
    IConvertPlugin *plugin = loadedPlugins.value(pluginKey, nullptr);
    if (!plugin) {
        QPluginLoader loader(pluginKey);
        QObject *o = loader.instance();
        if (o) {
            plugin = qobject_cast<IConvertPlugin *>(o);
            loadedPlugins.insert(pluginKey, plugin);
        }
        else {
            QMessageBox::critical(this, "Ошибка загрузки плагина",
                                  loader.errorString());
        }
    }

    if (plugin) {
        QStringList files = plugin->getConvertedFiles(App->formatFactory.get());
        if (plugin->addFiles()) {
            this->addFiles(files);
            if (currentTab) currentTab->fileHandler->trackFiles(files);
            saveTabsState();
        }
    }
}

void MainWindow::saveTabsState()
{
    QVariantMap map;
    const auto m = m_DockManager->dockWidgetsMap().values();
    for (const auto &w: m) {
        if (Tab *t = qobject_cast<Tab *>(w->widget())) {
            if (auto folders = t->fileHandler->fileNames(); !folders.isEmpty())
                map.insert(w->windowTitle(), folders);
            if (currentTab) currentTab->filterHeader->clear();
        }
    }
    se->setSetting("folders1", map);
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

void MainWindow::convertAnaFiles()
{
    AnaConverterDialog dialog(this);
    if (dialog.exec()) {
        QStringList files = dialog.getConvertedFiles();
        if (files.isEmpty()) return;
        this->addFiles(files);
        if (currentTab) currentTab->fileHandler->trackFiles(files);
    }
}

void MainWindow::saveTimeSegment(const QVector<FileDescriptor *> &files, double from, double to)
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
    if (!currentTab) return;
    //if (!currentTab->channelsTable->hasFocus()) return;
    QVector<int> selectedIndexes = currentTab->channelModel->selected();

    const QVector<int> newIndexes = computeIndexes(selectedIndexes, up, currentTab->record->channelsCount());

    if (newIndexes == selectedIndexes) return;

    for (auto &file: currentTab->model->selectedFiles()) {
        file->move(up, selectedIndexes, newIndexes);
    }

    //tab->record->move(up, selectedIndexes, newIndexes);

    currentTab->updateChannelsTable(currentTab->record);

    currentTab->channelsTable->clearSelection();
    for (int i: newIndexes)
        currentTab->channelsTable->selectionModel()->select(currentTab->channelModel->index(i,0),QItemSelectionModel::Select);
}

void MainWindow::ctrlUpTriggered(bool b)
{DD;
    Q_UNUSED(b);
    if (dynamic_cast<ChannelsTable*>(qApp->focusWidget())) {
        moveChannelsUp();
    }
    else {
        setDescriptor(-1, false);
    }
}

void MainWindow::ctrlDownTriggered(bool b)
{DD;
    Q_UNUSED(b);
    if (dynamic_cast<ChannelsTable*>(qApp->focusWidget())) {
        moveChannelsDown();
    }
    else {
        setDescriptor(1, false);
    }
}

void MainWindow::unplotChannel(Channel* channel, bool allChannels)
{
    if (!currentPlot || !currentPlot->plot()) return;
    const int channelIndex = channel->index();
    if (allChannels && currentTab) {
        const QList<FileDescriptor*> selectedFiles = currentTab->model->selectedFiles();
        for (auto descriptor: selectedFiles) currentPlot->plot()->deleteCurveForChannelIndex(descriptor, channelIndex);
    }
    else {
        currentPlot->plot()->deleteCurveForChannelIndex(channel->descriptor(), channelIndex);
    }
}

//Возможно, добавляем к списку каналы из других записей, а затем строим графики
void MainWindow::onChannelsDropped(bool plotOnLeft, const QVector<Channel *> &channels)
{DD;
    if (QApplication::keyboardModifiers() & Qt::ControlModifier)
        onChannelsDropped(plotOnLeft, channels, true);
    else
        onChannelsDropped(plotOnLeft, channels, false);
}

void MainWindow::onChannelsDropped(bool plotOnLeft, const QVector<Channel *> &channels, bool plotAll)
{DD;
    //определяем список каналов, которые нужно построить
    QVector<Channel *> toPlot;// = channels;
    if (currentTab && currentTab->model->selected().size()>1 && plotAll) {
        const QList<FileDescriptor*> selectedFiles = currentTab->model->selectedFiles();
        for (const auto f: selectedFiles) {
//            if (f == currentTab->record) continue;
            for (const auto &ch: channels) {
                int index = ch->index();
                if (auto c = f->channel(index)) toPlot << c;
            }
        }
    }
    else
        toPlot = channels;

    if (toPlot.isEmpty()) return;

    //определяем график, на котором будем строить каналы
    QCPPlot* p = nullptr;
    if (currentPlot) {
        p = currentPlot->plot();
        auto type = PlotArea::getPlotType(toPlot);

        //графика нет - создаем график
        if (!p) {
            currentPlot->addPlot(type);
        }
        else {
            if (p->model()->isEmpty()) {
                if (p->type() != type) {
                    currentPlot->addPlot(type);
                }
            }
            else {
                if (!PlotArea::plotTypesCompatible(p->type(), type)) {
                    //кривые есть - выводим сообщение
                    QMessageBox::warning(this, QString("Не могу построить канал"),
                                         QString("Тип графика не подходит.\nСначала очистите график."));
                    return;
                }
            }
        }
        p = currentPlot->plot();
        //в текущей вкладке графика еще нет самого графика
        setCurrentPlot(currentPlot->plot());
    }
    if (p) {//график существует
        {//Строим первую кривую и обновляем график, чтобы высота легенды определилась правильно
            int index=-1;
            if (currentTab) currentTab->model->contains(toPlot.first()->descriptor(),&index);
            p->plotChannel(toPlot.first(), plotOnLeft, index+1);
        }
        qApp->processEvents();
        //Строим остальные кривые
        for (int i=1; i<toPlot.size(); ++i) {
            auto ch = toPlot.at(i);
            int index=-1;
            if (currentTab) currentTab->model->contains(ch->descriptor(),&index);
            p->plotChannel(ch, plotOnLeft, index+1);
        }
    }
    else LOG(ERROR) << QString("Во вкладке не создан подходящий график");
}

void MainWindow::calculateSpectreRecords(bool useDeepsea)
{DD;
    if (!currentTab) return;

    QList<FileDescriptor *> records = currentTab->model->selectedFiles({Descriptor::TimeResponse}, false);

    if (records.isEmpty()) {
        QMessageBox::warning(this,QString("DeepSea Base"),
                             QString("Не выделено ни одного файла с исходными временными данными"));
        return;
    }

    if (useDeepsea) {
        DeepSeaProcessorDialog dialog(records, this);
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
        ::calculateThirdOctave(newFile.get(), f);

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
    if (!currentPlot) return;
    if (currentPlot->curvesCount()==0) return;

    int windowSize = se->getSetting("movingAvgSize",3).toInt();
    bool ok;
    windowSize = QInputDialog::getInt(this,"Скользящее среднее","Выберите величину окна усреднения",windowSize,
                                      3,15,2,&ok);
    if (ok)
        se->setSetting("movingAvgSize",windowSize);
    else
        return;

    auto channels = currentPlot->plottedChannels();

    bool oneFile = true; // каналы из одного файла
    bool writeToSeparateFile = true;

    auto firstChannel = channels.first();
    const QString firstName = firstChannel->descriptor()->fileName();

    bool allFilesDfd = firstName.toLower().endsWith("dfd") ||
                       firstName.toLower().endsWith("d94");

    for (auto channel: channels) {
        const QString curveName = channel->descriptor()->fileName();

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

        avgD = se->getSetting("lastMovingAvgFile", avgD).toString();

        QStringList filters = App->formatFactory->allFilters();
        QStringList suffixes = App->formatFactory->allSuffixes(true);

        QFileDialog dialog(this, "Выбор файла для записи каналов", avgD,
                           filters.join(";;"));
        dialog.setOption(QFileDialog::DontUseNativeDialog, true);
        dialog.setFileMode(QFileDialog::AnyFile);

        if (allFilesDfd) {
            QSortFilterProxyModel *proxy = new DfdFilterProxy(firstChannel->descriptor(), this);
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
        se->setSetting("lastMovingAvgFile", avgFileName);

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
            LOG(ERROR) << QString("Не удалось создать файл") << avgFileName;
            return;
        }

        if (QFileInfo(avgFileName).exists()) {
            if (isNew) avg->read();
        }
        else
            avg->fillPreliminary(firstChannel->descriptor());
    }
    else {
        avgFileName = firstChannel->descriptor()->fileName();
        avg = App->find(avgFileName);
    }

    ::calculateMovingAvg(avg.get(), channels.toList(),windowSize);

    int idx;
    if (currentTab && currentTab->model->contains(avgFileName, &idx)) {
        currentTab->model->updateFile(idx);
    }
    else {
        addFile(avg);
    }
    setCurrentAndPlot(avg.get(), avg->channelsCount()-1);
}

//сохраняет спектр из спектрограммы
void MainWindow::saveHorizontalSlice(const QVector<double>& zValues)
{DD;
    if (!currentPlot) return;

    auto firstChannel = currentPlot->firstVisible();
    if (!firstChannel) return;

    const QString firstName = firstChannel->descriptor()->fileName();

    QString title = zValues.size()>1 ? "Сохранение спектров" : "Сохранение спектра";
    QString message = zValues.size()>1 ? "Сохранить спектры в эту запись дополнительными каналами?" :
                                   "Сохранить спектр в эту запись дополнительным каналом?";
    QMessageBox box(title, message,
                    QMessageBox::Question,
                    QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel);
    box.setButtonText(QMessageBox::Yes, "Да, записать в этот файл");
    box.setButtonText(QMessageBox::No, "Нет, экспортировать в отдельный файл");
    box.setEscapeButton(QMessageBox::Cancel);

    int result = box.exec();
    if (result == QMessageBox::Cancel) return;
    bool writeToSeparateFile = (result == QMessageBox::No);

    QString spectreFileName;
    F spectre;

    if (writeToSeparateFile) {
        QString spectreD = firstName;
        spectreD.chop(4);

        spectreD = se->getSetting("lastSpectreFile", spectreD).toString();

        QStringList filters = App->formatFactory->allFilters();
        QStringList suffixes = App->formatFactory->allSuffixes(true);

        QFileDialog dialog(this, "Выбор файла для записи каналов", spectreD,
                           filters.join(";;"));
        dialog.setOption(QFileDialog::DontUseNativeDialog, true);
        dialog.setFileMode(QFileDialog::AnyFile);

        QSortFilterProxyModel *proxy = new DfdFilterProxy(firstChannel->descriptor(), this);
        dialog.setProxyModel(proxy);

        QString selectedFilter;
        QStringList selectedFiles;
        if (dialog.exec()) {
            selectedFiles = dialog.selectedFiles();
            selectedFilter = dialog.selectedNameFilter();
        }
        if (selectedFiles.isEmpty()) return;

        spectreFileName = selectedFiles.constFirst();
        if (spectreFileName.isEmpty()) return;
        se->setSetting("lastSpectreFile", spectreFileName);

        //добавляем суффикс
        QString currentSuffix = QFileInfo(spectreFileName).suffix().toLower();
        QString filterSuffix = suffixes.at(filters.indexOf(selectedFilter));
        if (currentSuffix != filterSuffix) {
            //удаляем суффикс, если это суффикс известного нам типа файлов
            if (suffixes.contains(currentSuffix))
                spectreFileName.chop(currentSuffix.length()+1);

            spectreFileName.append(QString(".%1").arg(filterSuffix));
        }

        bool isNew = false;
        spectre = App->addFile(spectreFileName, &isNew);

        if (!spectre) {
            LOG(ERROR) << QString("Не удалось создать файл") << spectreFileName;
            return;
        }

        if (QFileInfo(spectreFileName).exists()) {
            if (isNew) spectre->read();
        }
        else
            spectre->fillPreliminary(firstChannel->descriptor());
    }
    else {
        spectreFileName = firstChannel->descriptor()->fileName();
        spectre = App->find(spectreFileName);
    }

    auto file = spectre.get();

    for (double zValue: zValues)
        ::saveSpectre(file, firstChannel, zValue);

    file->setChanged(true);
    file->setDataChanged(true);
    file->write();

    int idx;
    if (currentTab && currentTab->model->contains(spectreFileName, &idx)) {
        currentTab->model->updateFile(idx);
        currentTab->updateChannelsTable(currentTab->record);
    }
    else {
        addFile(spectre);
    }
}

void MainWindow::saveVerticalSlice(const QVector<double> &frequencies)
{DD;
    if (!currentPlot) return;

    auto firstChannel = currentPlot->firstVisible();
    if (!firstChannel) return;

    const QString firstName = firstChannel->descriptor()->fileName();

    QString title = frequencies.size()>1 ? "Сохранение проходных характеристик" : "Сохранение проходной характеристики";
    QString message = frequencies.size()>1 ? "Сохранить проходные в эту запись дополнительными каналами?" :
                                   "Сохранить проходную в эту запись дополнительным каналом?";
    QMessageBox box(title, message,
                    QMessageBox::Question,
                    QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel);
    box.setButtonText(QMessageBox::Yes, "Да, записать в этот файл");
    box.setButtonText(QMessageBox::No, "Нет, экспортировать в отдельный файл");
    box.setEscapeButton(QMessageBox::Cancel);

    int result = box.exec();
    if (result == QMessageBox::Cancel) return;
    bool writeToSeparateFile = (result == QMessageBox::No);

    QString throughFileName;
    F through;

    if (writeToSeparateFile) {
        QString throughD = firstName;
        throughD.chop(4);

        throughD = se->getSetting("lastThroughFile", throughD).toString();

        QStringList filters = App->formatFactory->allFilters();
        QStringList suffixes = App->formatFactory->allSuffixes(true);

        QFileDialog dialog(this, "Выбор файла для записи каналов", throughD,
                           filters.join(";;"));
        dialog.setOption(QFileDialog::DontUseNativeDialog, true);
        dialog.setFileMode(QFileDialog::AnyFile);

        QSortFilterProxyModel *proxy = new DfdFilterProxy(firstChannel->descriptor(), this);
        dialog.setProxyModel(proxy);

        QString selectedFilter;
        QStringList selectedFiles;
        if (dialog.exec()) {
            selectedFiles = dialog.selectedFiles();
            selectedFilter = dialog.selectedNameFilter();
        }
        if (selectedFiles.isEmpty()) return;

        throughFileName = selectedFiles.constFirst();
        if (throughFileName.isEmpty()) return;
        se->setSetting("lastThroughFile", throughFileName);

        //добавляем суффикс
        QString currentSuffix = QFileInfo(throughFileName).suffix().toLower();
        QString filterSuffix = suffixes.at(filters.indexOf(selectedFilter));
        if (currentSuffix != filterSuffix) {
            //удаляем суффикс, если это суффикс известного нам типа файлов
            if (suffixes.contains(currentSuffix))
                throughFileName.chop(currentSuffix.length()+1);

            throughFileName.append(QString(".%1").arg(filterSuffix));
        }

        bool isNew = false;
        through = App->addFile(throughFileName, &isNew);

        if (!through) {
            QMessageBox::critical(this, "Сохранение проходной характеристики",
                                  QString("Не удалось создать файл %1").arg(throughFileName));
            return;
        }

        if (QFileInfo(throughFileName).exists()) {
            if (isNew) through->read();
        }
        else
            through->fillPreliminary(firstChannel->descriptor());
    }
    else {
        throughFileName = firstChannel->descriptor()->fileName();
        through = App->find(throughFileName);
    }

    auto file = through.get();

    for (double xValue: frequencies)
        ::saveThrough(file, firstChannel, xValue);

    file->setChanged(true);
    file->setDataChanged(true);
    file->write();

    int idx;
    if (currentTab && currentTab->model->contains(throughFileName, &idx)) {
        currentTab->model->updateFile(idx);
        currentTab->updateChannelsTable(currentTab->record);
    }
    else {
        addFile(through);
    }
}

void MainWindow::rescanBase()
{DD;
    const auto m = m_DockManager->dockWidgetsMap();

    //определяем, были ли изменения
    bool changed = false;
    for (const auto &w: m.values()) {
        if (Tab *t = qobject_cast<Tab *>(w->widget()); t && t->model->changed()) {
            changed = true;
            break;
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
        QPushButton *discB=msgBox.addButton(tr("Нет, сбросить изменения"),QMessageBox::NoRole);
        discB->setIcon(qApp->style()->standardIcon(QStyle::SP_DialogDiscardButton));
        QPushButton *cancB=msgBox.addButton(QMessageBox::Cancel);
        cancB->setIcon(qApp->style()->standardIcon(QStyle::SP_DialogCancelButton));
        msgBox.setDefaultButton(saveB);
        msgBox.setEscapeButton(cancB);
        msgBox.setWindowModality(Qt::WindowModal);
        msgBox.exec();

        if (msgBox.clickedButton() == cancB) return;
        if (msgBox.clickedButton() == discB) {
            for (const auto &w: m) {
                if (Tab *t = qobject_cast<Tab *>(w->widget()))
                    t->model->discardChanges();
            }
        }
    }

    // first we delete all curves affected
    for (auto w: m.values()) {
        if (auto area = dynamic_cast<PlotArea*>(w))
            area->deleteAllCurves();
    }

    Tab *oldTab = currentTab;
    // then we refill all the tabs with files
    for (auto w: m.values()) {
        if (auto t = dynamic_cast<Tab*>(w->widget())) {
            t->channelModel->clear();
            t->model->clear();
            t->filePathLabel->clear();
            currentTab = t;

            for (auto folder: qAsConst(t->fileHandler->files)) {
                addFolder(folder.path, folder.type == FileHandler::FolderWithSubfolders, false);
            }
        }
    }

    currentTab = oldTab;
}

void MainWindow::addFile(F descriptor)
{DD;
    if (!currentTab) return;
    if (!descriptor) return;

    addDescriptors(QList<F>()<<descriptor);
}

//TODO: проверить необходимость updateChannelsTable(d);
void MainWindow::setCurrentAndPlot(FileDescriptor *d, int index)
{DD;
    if (currentTab && currentTab->model->contains(d)) {
        currentTab->updateChannelsTable(d);
        onChannelsDropped(true, {d->channel(index)});
    }
}

//Этот метод ищет нужную запись по direction и перемещает на неё выделение
void MainWindow::setDescriptor(int direction, bool checked) /*private*/
{DD;
    if (!currentTab) return;
    //проверяем, есть ли в табе другие записи
    if (currentTab->model->size() < 2) return;
    if (!currentPlot) return;
    if (!currentPlot->plot()) return;

    //проверяем, в какой вкладке находится та запись, для которой построены графики
    FileDescriptor* d = nullptr;
    //ищем запись первой не фиксированной кривой
    //(фиксированная кривая может быть из другой записи)

    auto c = currentPlot->plot()->model()->firstOf([](Curve *c){return !c->fixed;});
    if (c) d = c->channel->descriptor();

    if (!d) return;

    int index = -1;
    currentTab->model->contains(d, &index);

    if (index<0) return; //мы в другой вкладке

    //перемещаем фокус на другую запись
    QModelIndex current = currentTab->sortModel->mapFromSource(currentTab->model->index(index, MODEL_COLUMN_FILENAME));
    QModelIndex modelIndex;
    index = current.row();
    if (direction==-1) {
        if (index == 0)
            modelIndex = currentTab->sortModel->index(currentTab->sortModel->rowCount()-1, current.column());
        else
            modelIndex = currentTab->sortModel->index(index-1, current.column());
    }
    else if (direction==1) {
        if (index == currentTab->sortModel->rowCount()-1)
            modelIndex = currentTab->sortModel->index(0, current.column());
        else
            modelIndex = currentTab->sortModel->index(index+1, current.column());
    }
    else if (direction==0) {
        currentPlot->plot()->sergeiMode = checked;
    }

    //ниже - только для file up/file down
    if (modelIndex.isValid()) {
        //это вызывает Tab::updateChannelsTable(FileDescriptor *descriptor)
        //который посылает сигнал в MainWindow, который в свою очередь вызывает currentPlot->replotDescriptor
        currentPlot->plot()->sergeiMode = true;
        currentTab->filesTable->selectionModel()->setCurrentIndex(modelIndex, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Current);
        currentPlot->plot()->sergeiMode = false;
    }
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
    const int curvesCount = currentPlot ? currentPlot->curvesCount():0;
    bool spectrogram = false;
    if (currentPlot && currentPlot->plot())
        spectrogram = currentPlot->plot()->type() == Enums::PlotType::Spectrogram;

    saveAct->setEnabled(currentTab->model->changed());

    renameAct->setDisabled(selectedFilesCount==0);
    delFilesAct->setDisabled(selectedFilesCount==0);
    editChannelDescriptionsAct->setDisabled(selectedChannelsCount==0);
    const auto timeFiles = currentTab->model->selectedFiles({Descriptor::TimeResponse}, false);
    calculateSpectreAct->setDisabled(timeFiles.isEmpty());
    calculateSpectreAct1->setDisabled(timeFiles.isEmpty());
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
                currentTab->model->selectedFiles(types, false).isEmpty());
    rescanBaseAct->setEnabled(filesCount>0);

    meanAct->setDisabled(curvesCount<2);
    movingAvgAct->setEnabled(curvesCount>0 && !spectrogram);
    addCorrectionAct->setEnabled(curvesCount>0 && !spectrogram);
    addCorrectionsAct->setEnabled(curvesCount>0 && !spectrogram);
    deleteChannelsAct->setDisabled(selectedChannelsCount==0);
    deleteChannelsOneAct->setDisabled(selectedChannelsCount==0);
    deleteChannelsBatchAct->setDisabled(selectedChannelsCount==0);
    copyChannelsAct->setDisabled(selectedChannelsCount==0);
    moveChannelsAct->setDisabled(selectedChannelsCount==0);
    copyPlottedChannelsAct->setEnabled(curvesCount>0);
    movePlottedChannelsAct->setEnabled(curvesCount>0);
    deletePlottedChannelsAct->setEnabled(curvesCount>0);

    moveChannelsDownAct->setEnabled(selectedChannelsCount > 0 && selectedChannelsCount < channelsCount);
    moveChannelsUpAct->setEnabled(selectedChannelsCount > 0 && selectedChannelsCount < channelsCount);
    editDescriptionsAct->setDisabled(selectedFilesCount==0);

    exportToExcelAct->setEnabled(curvesCount>0 /*&& !spectrogram*/);
    exportToExcelFullAct->setEnabled(curvesCount>0);
    exportToExcelOnlyDataAct->setEnabled(curvesCount>0);
    exportToClipboardAct->setEnabled(curvesCount>0);
    exportChannelsToWavAct->setEnabled(!timeFiles.isEmpty() && selectedChannelsCount>0);

    if (currentPlot) currentPlot->updateActions(filesCount, channelsCount);
    currentTab->updateActions();
}

void MainWindow::onFocusedDockWidgetChanged(ads::CDockWidget *old, ads::CDockWidget *now)
{DD;
    Q_UNUSED(old);
    if (!now) return;
    if (auto tab = qobject_cast<Tab *>(now->widget())) {
        currentTab = tab;
        ctrlUpTargetsChannels = true;
        if (currentPlot) setCurrentPlot(currentPlot->plot());
    }
    else if (auto plot = qobject_cast<PlotArea*>(now)) {
        currentPlot = plot;
        ctrlUpTargetsChannels = false;
        setCurrentPlot(currentPlot->plot());
    }
    updateActions();
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

void MainWindow::exportToClipboard()
{
    if (currentPlot) currentPlot->exportToClipboard();
}

void MainWindow::onChannelChanged(Channel *ch)
{DD;
    if (!currentTab || !ch) return;
    currentTab->model->invalidateCurve(ch);
    if (currentTab->record == ch->descriptor())
            currentTab->channelModel->onChannelChanged(ch);
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
    //удаляем временные файлы
    if (se->getSetting("deleteTemporaryFiles", true).toBool()) {
        temporaryFiles->deleteAll();
    }

    //определяем, были ли изменения
    bool changed = false;
    const auto m = m_DockManager->dockWidgetsMap().values();
    for (const auto &w: m) {
        if (Tab *t = qobject_cast<Tab *>(w->widget()); t && t->model->changed()) {
            changed = true;
            break;
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
            for (const auto &w: m) {
                if (Tab *t = qobject_cast<Tab *>(w->widget()))
                    t->model->discardChanges();
            }
        }
    }

    if (currentTab) {
        se->setSetting("upperSplitterState",currentTab->saveState());
        QByteArray treeHeaderState = currentTab->filesTable->header()->saveState();
        se->setSetting("treeHeaderState", treeHeaderState);
    }

    QVariantMap map;
    for (const auto &w: m) {
        if (Tab *t = qobject_cast<Tab *>(w->widget())) {
            if (auto folders = t->fileHandler->fileNames(); !folders.isEmpty())
                map.insert(w->windowTitle(), folders);
            if (currentTab) currentTab->filterHeader->clear();
        }
    }
    se->setSetting("folders1", map);

    return true;
}
