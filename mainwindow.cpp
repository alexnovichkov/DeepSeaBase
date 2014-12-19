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
{
    if (QFileInfo(fileName).suffix().toLower()=="dfd") return new DfdFileDescriptor(fileName);
    if (QFileInfo(fileName).suffix().toLower()=="uff") return new UffFileDescriptor(fileName);
    if (QFileInfo(fileName).suffix().toLower()=="unv") return new UffFileDescriptor(fileName);
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
    setWindowTitle(tr("DeepSea Database 1.3.6.2"));
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


    delFilesAct = new QAction(QString("Удалить записи"), this);
    delFilesAct->setShortcut(Qt::Key_Delete);
    delFilesAct->setShortcutContext(Qt::WidgetShortcut);
    connect(delFilesAct, SIGNAL(triggered()), SLOT(deleteFiles()));

    plotAllChannelsAct = new QAction(QString("Построить все каналы"), this);
    connect(plotAllChannelsAct, SIGNAL(triggered()), SLOT(plotAllChannels()));

//    removeChannelsPlotsAct = new QAction(QString("Очистить графики каналов"), this);
//    connect(removeChannelsPlotsAct, &QAction::triggered, [=](){
//        plot->deleteGraphs();
//        for (int index = 0; index<tabWidget->count(); ++index) {
//            Tab *t = qobject_cast<Tab*>(tabWidget->widget(index));
//            if (t) {
//                QFont boldFont = t->tree->font();
//                boldFont.setBold(true);
//                for (int i=0; i<t->tree->topLevelItemCount(); ++i) {
//                    if (t->tree->topLevelItem(i)->font(1) == boldFont) {
//                        DfdFileDescriptor *dfd = dynamic_cast<SortableTreeWidgetItem *>(t->tree->topLevelItem(i))->dfd;
//                        QList<Channel *> list = dfd->channels;
//                        foreach (Channel *c, list) {
//                            c->checkState = Qt::Unchecked;
//                            c->color = QColor();
//                        }
//                        t->tree->topLevelItem(i)->setFont(1, t->tree->font());
//                    }
//                }
//                t->channelsTable->blockSignals(true);
//                for (int i=0; i<t->channelsTable->rowCount(); ++i) {
//                    t->channelsTable->item(i,0)->setCheckState(Qt::Unchecked);
//                    t->channelsTable->item(i,0)->setFont(t->channelsTable->font());
//                    t->channelsTable->item(i,0)->setBackgroundColor(Qt::white);
//                    t->channelsTable->item(i,0)->setTextColor(Qt::black);
//                }
//                t->channelsTable->blockSignals(false);
//            }
//        }
//    });

    QAction *plotHelpAct = new QAction("Справка", this);
    connect(plotHelpAct, &QAction::triggered, [=](){
        QString info = "<b>Управление графиком</b><br>"
                       "<u>Внутри графика:</u><br>"
                       "Правая кнопка мыши: двигать график по плоскости<br>"
                       "Колесико мыши: увеличить/уменьшить масштаб<br>"
                       "Колесико мыши + Ctrl: увеличить/уменьшить масштаб по горизонтали<br>"
                       "Колесико мыши + Shift: увеличить/уменьшить масштаб по вертикали<br>"
                       "Левая кнопка мыши: масштабирование по выделению<br>"
                       "Backspace или двойной щелчок: вернуть масштаб к исходному<br>"
                       "Backspace + Ctrl: вернуть масштаб к исходному по горизонтали<br>"
                       "Backspace + Shift: вернуть масштаб к исходному по вертикали<br>"
                       "Клавиша H: показать/спрятать подписи осей<br>"
                       "<u>На осях:</u><br>"
                       "Левая кнопка мыши: увеличить/уменьшить масштаб на оси"
                       ""
                       ;
        QMessageBox::information(this,"DeepSea Base", info);
    });

    convertAct = new QAction(QString("Конвертировать в..."), this);
    //convertAct->setIcon();
    connect(convertAct, SIGNAL(triggered()), SLOT(convertRecords()));


    clearPlotAct  = new QAction(QString("Очистить график"), this);
    clearPlotAct->setIcon(QIcon(":/icons/cross.png"));
    connect(clearPlotAct, SIGNAL(triggered()), SLOT(clearPlot()));

    savePlotAct = new QAction(QString("Сохранить график..."), this);
    savePlotAct->setIcon(qApp->style()->standardIcon(QStyle::SP_DialogSaveButton));
    connect(savePlotAct, SIGNAL(triggered()), plot, SLOT(savePlot()));

    rescanBaseAct = new QAction(QString("Пересканировать базу"), this);
    rescanBaseAct->setIcon(qApp->style()->standardIcon(QStyle::SP_BrowserReload));
    connect(rescanBaseAct, SIGNAL(triggered()), this, SLOT(rescanBase()));

    switchCursorAct = new QAction(QString("Показать/скрыть курсор"), this);
    switchCursorAct->setIcon(QIcon(":/icons/cursor.png"));
    switchCursorAct->setCheckable(true);
    bool pickerEnabled = MainWindow::getSetting("pickerEnabled", true).toBool();
    switchCursorAct->setChecked(pickerEnabled);
    connect(switchCursorAct, SIGNAL(triggered()), plot, SLOT(switchCursor()));

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
    exportToExcelAct->setMenu(excelMenu);

    meanAct = new QAction(QString("Вывести среднее"), this);
    meanAct->setIcon(QIcon(":/icons/mean.png"));
    connect(meanAct, SIGNAL(triggered()), this, SLOT(calculateMean()));

    editColorsAct = new QAction(QString("Изменить цвета графиков"), this);
    editColorsAct->setIcon(QIcon(":/icons/colors.png"));
    connect(editColorsAct, SIGNAL(triggered()), this, SLOT(editColors()));

    interactionModeAct = new QAction(QString("Включить режим изменения данных"), this);
    interactionModeAct->setIcon(QIcon(":/icons/data.png"));
    interactionModeAct->setCheckable(true);
    connect(interactionModeAct, &QAction::triggered, [=](){
        bool b = plot->switchInteractionMode();
        addCorrectionAct->setEnabled(b);
    });

    addCorrectionAct = new QAction("Добавить поправку...", this);
    addCorrectionAct->setIcon(QIcon(":/icons/correction.png"));
    connect(addCorrectionAct, SIGNAL(triggered()), SLOT(addCorrection()));

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


    mainToolBar->addWidget(new QLabel("Записи:"));
    mainToolBar->addAction(addFolderAct);
    mainToolBar->addAction(addFileAct);
    mainToolBar->addAction(convertAct);
    mainToolBar->addAction(editDescriptionsAct);

    mainToolBar->addSeparator();
    mainToolBar->addWidget(new QLabel("  Каналы:"));
    QMenu *channelsMenu = new QMenu("Операции", this);
    channelsMenu->addAction(copyChannelsAct);
    channelsMenu->addAction(moveChannelsAct);

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
    mainToolBar->addAction(exportToExcelAct);
    mainToolBar->addAction(switchCursorAct);
//    mainToolBar->addSeparator();

    mainToolBar->addAction(interactionModeAct);

    mainToolBar->addAction(plotHelpAct);


    QMenu *fileMenu = menuBar()->addMenu(tr("Файл"));
    fileMenu->addAction(addFolderAct);
    fileMenu->addAction(addFileAct);

    QMenu *recordsMenu = menuBar()->addMenu(QString("Записи"));
    recordsMenu->addAction(delFilesAct);
    recordsMenu->addAction(rescanBaseAct);

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


    QWidget *plotsWidget = new QWidget(this);
    QGridLayout *plotsLayout = new QGridLayout;
//    plotsLayout->addWidget(toolBar,0,0);
    plotsLayout->addWidget(plot,0,1);
    plotsWidget->setLayout(plotsLayout);

    splitter = new QSplitter(Qt::Vertical, this);
    splitter->addWidget(tabWidget);
    splitter->addWidget(plotsWidget);

    QByteArray mainSplitterState = getSetting("mainSplitterState").toByteArray();
    if (!mainSplitterState.isEmpty())
        splitter->restoreState(mainSplitterState);

    setCentralWidget(splitter);



    QVariantMap v = getSetting("folders").toMap();
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
//    tab->tree->addAction(removeChannelsPlotsAct);
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
    connect(tab->tree,SIGNAL(itemChanged(QTreeWidgetItem*,int)), SLOT(recordLegendChanged(QTreeWidgetItem*, int)));
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
            //QDesktopServices::openUrl(dir.canonicalPath());
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

    foreach (const QString &folder, folders) {
        if (QFileInfo(folder).exists()) {
            tab->folders.append(folder);
            addFolder(folder, true);
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
    setSetting("folders", map);

    ColorSelector::instance()->drop();
}

bool checkForContains(Tab *tab, const QString &file, int *index = 0)
{DD;
    for (int i=0; i<tab->tree->topLevelItemCount(); ++i) {
        SortableTreeWidgetItem *item = dynamic_cast<SortableTreeWidgetItem *>(tab->tree->topLevelItem(i));
        if (item) {
           // qDebug()<<item->fileDescriptor->fileName();
            if (item->fileDescriptor->fileName() == file) {
                if (index) *index = i;
                return true;
            }
        }
    }

    if (index) *index = -1;
    return false;
}

void MainWindow::addFolder()
{DD;
    QString directory = getSetting("lastDirectory").toString();

    directory = QFileDialog::getExistingDirectory(this,
                                                  tr("Add folder"),
                                                  directory,
                                                  QFileDialog::ShowDirsOnly | QFileDialog::ReadOnly);

    if (directory.isEmpty()) return;
    setSetting("lastDirectory", directory);
    addFolder(directory);
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
    dialog.setFileMode(QFileDialog::AnyFile);


    QStringList fileNames;
    if (dialog.exec()) {
        fileNames = dialog.selectedFiles();
    }
    if (fileNames.isEmpty()) return;

    setSetting("lastDirectory", fileNames.first());
    addFiles(fileNames);
}

void MainWindow::addFolder(const QString &directory, bool silent)
{DD;
    if (directory.isEmpty()) return;

    if (!tab) return;

    QStringList filesToAdd;
    processDir(directory, filesToAdd, true);

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
    if (!toAdd.isEmpty())
        if (!tab->folders.contains(directory))
            tab->folders.append(directory);
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
{
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

    if (QMessageBox::question(this,"DeepSea Base","Выделенные каналы будут \nудалены из файлов. Продолжить?")==QMessageBox::Yes) {
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

    if (QMessageBox::question(this,"DeepSea Base","Выделенные каналы будут \nудалены из файлов. Продолжить?")==QMessageBox::Yes) {
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
{
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
    if (dialog.exec()) {
        selectedFiles = dialog.selectedFiles();
    }
    if (selectedFiles.isEmpty()) return false;

    QString file = selectedFiles.first();
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

        dfd->write();
        dfd->writeRawFile();
        addFile(dfd);
    }
    else {
        dfd->copyChannelsFrom(channelsToCopy);

        dfd->write();
        dfd->writeRawFile();

        updateFile(dfd);
    }
    return true;
}

void MainWindow::calculateMean()
{DD;
    const int graphsSize = plot->graphsCount();
    if (graphsSize<2) return;

    QMultiHash<FileDescriptor *, int> channels;

    bool dataTypeEqual = true;
    bool stepsEqual = true; // одинаковый шаг по оси Х
    bool namesEqual = true; // одинаковые названия осей Y
    bool oneFile = true; // каналы из одного файла
    bool writeToSeparateFile = true;
    bool writeToUff = false;

    Curve *firstCurve = plot->graphs.first();
    channels.insert(firstCurve->descriptor, firstCurve->channelIndex);

    bool allFilesDfd = firstCurve->descriptor->fileName().toLower().endsWith("dfd");

    for (int i = 1; i<graphsSize; ++i) {
        Curve *curve = plot->graphs.at(i);
        channels.insert(curve->descriptor, curve->channelIndex);

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
        }


        const Qt::CheckState state = item->checkState();


        QFont oldFont = tab->channelsTable->font();
        QFont boldFont = oldFont;
        boldFont.setBold(true);

//        if (state == Qt::Checked && !ch->populated())
//            ch->populate();

        tab->channelsTable->blockSignals(true);
        bool plotted = true;

        if (state == Qt::Checked) {
            QColor col;
            plotted = plot->plotChannel(tab->record, item->row(), &col);
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
        }
        else if (state == Qt::Unchecked) {
            plot->deleteGraph(tab->record, item->row());
            item->setFont(tab->channelsTable->font());
            ch->setCheckState(Qt::Unchecked);
            item->setBackgroundColor(Qt::white);
            item->setTextColor(Qt::black);
            ch->setColor(QColor());
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

void MainWindow::convertRecords()
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

    ConvertDialog dialog(&records, this);

    if (dialog.exec()) {
        QStringList newFiles = dialog.getNewFiles();
        addFiles(newFiles);
    }
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
            t->channelsTable->blockSignals(false);
        }
    }
}

void MainWindow::recordLegendChanged(QTreeWidgetItem *item, int column)
{DD;
    if (column==9) {
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

//    if (column==8) {// описание
//        SortableTreeWidgetItem *i = dynamic_cast<SortableTreeWidgetItem *>(item);
//        if (i) {

//        }
//    }
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

    tab->filePathLabel->clear();

    QStringList folders = tab->folders;
    foreach (const QString &folder, folders)
        addFolder(folder, true);

    QMessageBox::information(this, "База данных", "В базе данных все записи \"живые\"!");
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

    int count = tab->tree->topLevelItemCount();
    addFiles(QList<FileDescriptor *>()<<descriptor);
    tab->tree->setCurrentItem(tab->tree->topLevelItem(count));
}

bool MainWindow::findDescriptor(FileDescriptor *d)
{
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
{
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
{
    exportToExcel(true);
}

QStringList twoStringDescription(const DescriptionList &list)
{
    QStringList result;
    if (list.size()>0) result << descriptionEntryToString(list.first()); else result << "";
    if (list.size()>1) result << descriptionEntryToString(list.at(1)); else result << "";
    return result;
}

void MainWindow::exportToExcel(bool fullRange)
{
    static QAxObject *excel = 0;

    if (!plot->hasGraphs()) {
        QMessageBox::warning(this, "Графиков нет", "Постройте хотя бы один график!");
        return;
    }

    if (!excel) excel = new QAxObject("Excel.Application", this);
    //qDebug()<<excel->generateDocumentation();
    //QList<QVariant> params; params << 0;
    //excel->dynamicCall("ActivateMicrosoftApp()", params);

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
    QString newSheetName = QDateTime::currentDateTime().toString();
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
     for (int i=1; i<curves.size(); ++i) {
         if (qAbs(curves.at(i)->channel->xStep() - channel->xStep()) >= 1e-10) {
             allChannelsHaveSameXStep = false;
             break;
         }
     }

     // ищем максимальное количество отсчетов
     // и максимальный шаг
     quint32 maxInd = channel->samplesCount();
     double maxStep = channel->xStep();
     double minX = channel->xBegin();
     double maxX = channel->xMaxInitial();

     Range range = plot->xRange();

     for (int i=1; i<curves.size(); ++i) {
         Channel *ch = curves.at(i)->channel;
         if (ch->samplesCount() > maxInd)
             maxInd = ch->samplesCount();
         if (ch->xStep() > maxStep)
             maxStep = ch->xStep();
         if (ch->xBegin()< minX)
             minX = ch->xBegin();
         if (ch->xMaxInitial() > maxX)
             maxX = ch->xMaxInitial();
     }
     if (minX >= range.min && maxX <= range.max) fullRange = true;

     bool exportPlots = true;
     if (maxInd > 32000 && fullRange) {
         QMessageBox::warning(this, "Слишком много данных",
                              "В одном или всех каналах число отсчетов превышает 32000.\n"
                              "Будут экспортированы только данные, но не графики");
         exportPlots = false;
     }


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
         quint32 numRows = maxInd;
         const quint32 numCols = curves.size();

         QList<QVariant> cellsList;
         QList<QVariant> rowsList;
         for (uint i = 0; i < maxInd; ++i) {
             double val = channel->xBegin() + i*channel->xStep();
             if (!fullRange && (val < range.min || val > range.max) ) continue;

             cellsList.clear();
             cellsList << val;
             for (uint j = 0; j < numCols; ++j) {
                 cellsList << ((curves.at(j)->channel->samplesCount() < maxInd) ? 0 : curves.at(j)->channel->yValues()[i]);
             }
             rowsList << QVariant(cellsList);
         }
         numRows = rowsList.size();

         QAxObject* Cell1 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 5, 1);
         QAxObject* Cell2 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 5 + numRows - 1, 1 + numCols);
         QAxObject* range = worksheet->querySubObject("Range(const QVariant&,const QVariant&)", Cell1->asVariant(), Cell2->asVariant() );

         range->setProperty("Value", QVariant(rowsList) );

         // выделяем диапазон, чтобы он автоматически использовался для построения диаграммы
         Cell1 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 2, 1);
         Cell2 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 2 + numRows+2, 1 + numCols);
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

             quint32 numRows = ch->samplesCount();
             QList<QVariant> cellsList;
             QList<QVariant> rowsList;
             for (uint j = 0; j < ch->samplesCount(); j++) {
                 double val = ch->xBegin() + j*ch->xStep();
                 if (!fullRange && (val < range.min || val > range.max) ) continue;

                 cellsList.clear();
                 cellsList << (ch->xBegin() + j*ch->xStep()) << ch->yValues()[j];
                 rowsList << QVariant(cellsList);
             }
             numRows = rowsList.size();
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
         //     chart->setProperty("Name", "Диагр."+newSheetName);
         chart->setProperty("ChartType", 75);

         QAxObject * series = chart->querySubObject("SeriesCollection");

         // отдельно строить кривые нужно, только если у нас много пар столбцов с данными
         if (!allChannelsHaveSameXStep) {
             Q_ASSERT(selectedSamples.size() == curves.size());
             for (int i=0; i<curves.size(); ++i) {
                 Curve *curve = curves.at(i);
                 QAxObject * serie = series->querySubObject("Item (int)", i+1);

                 QAxObject* Cell1 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 5, 1+i*2);
                 QAxObject* Cell2 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 5 + selectedSamples.at(i)-1, 1 + i*2);
                 QAxObject * xvalues = worksheet->querySubObject("Range(const QVariant&,const QVariant&)", Cell1->asVariant(), Cell2->asVariant());
                 serie->setProperty("XValues", xvalues->asVariant());

                 delete Cell1;
                 delete Cell2;

                 Cell1 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 5, 2+i*2);
                 Cell2 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 5 + selectedSamples.at(i)-1, 2 + i*2);
                 QAxObject * yvalues = worksheet->querySubObject("Range(const QVariant&,const QVariant&)", Cell1->asVariant(), Cell2->asVariant());
                 serie->setProperty("Values", yvalues->asVariant());

                 serie->setProperty("Name", curve->channel->name());

                 delete xvalues;
                 delete yvalues;
                 delete Cell1;
                 delete Cell2;
                 delete serie;
             }
             // удаляем лишние графики
             while (curves.size() < series->property("Count").toInt()) {
                 QAxObject * serie = series->querySubObject("Item (int)", curves.size()+1);
                 serie->dynamicCall("Delete()");
                 delete serie;
             }
         }

         // добавляем подписи осей
         QAxObject *xAxis = chart->querySubObject("Axes(const QVariant&)", 1);
         setAxis(xAxis, "Частота, Гц");
         xAxis->setProperty("MaximumScale", range.max);
         xAxis->setProperty("MinimumScale", int(range.min/10)*10);

         QAxObject *yAxis = chart->querySubObject("Axes(const QVariant&)", 2);
         setAxis(yAxis, "Уровень, дБ");
         yAxis->setProperty("CrossesAt", -1000);

         // рамка вокруг графика
         QAxObject *plotArea = chart->querySubObject("PlotArea");
         setLineColor(plotArea, 13);
         delete plotArea;

         // цвета графиков
         for (int i = 0; i< curves.size(); ++i) {
             Curve *curve = curves.at(i);
             QAxObject * serie = series->querySubObject("Item(int)", i+1);

             QAxObject *format = serie->querySubObject("Format");
             QAxObject *formatLine = format->querySubObject("Line");
             formatLine->setProperty("Weight", 1);

             QAxObject *formatLineForeColor = formatLine->querySubObject("ForeColor");
             formatLineForeColor->setProperty("RGB", curves.at(i)->pen().color().rgb());

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
             delete serie;
         }

         QAxObject *chartArea = chart->querySubObject("ChartArea");
         chartArea->querySubObject("Format")->querySubObject("Line")->setProperty("Visible", 0);
         delete chartArea;
         delete yAxis;
         delete xAxis;

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
    if (tab->record == curve->descriptor)
        updateChannelsTable(curve->descriptor);
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

void MainWindow::addFiles(const QStringList &files)
{DD;
    if (!tab) return;

    QList<FileDescriptor *> items;

    for (int i=0; i<files.size(); ++i) {
        QString file = files[i];

        FileDescriptor *dfd = findDescriptor(file);
        if (!dfd) {
            dfd = createDescriptor(file);
            dfd->read();
        }
        if (dfd) {
            items << dfd;
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

        delete tab->tree->takeTopLevelItem(indexes.at(i));
        if (!findDescriptor(d)) delete d;
        taken = true;
    }
    if (taken) {
        tab->channelsTable->setRowCount(0);
    }
}
