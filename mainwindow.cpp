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

#include <ActiveQt/ActiveQt>

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

void maybeAppend(const QString &s, QStringList &list)
{
    if (!list.contains(s)) list.append(s);
}

void processDir(const QString &file, QStringList &files, bool includeSubfolders)
{
    if (QFileInfo(file).isDir()) {
        QFileInfoList dirLst = QDir(file).entryInfoList(QStringList()<<"*.dfd"<<"*.DFD",
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
    : QMainWindow(parent), tab(0), record(0)
{
    setWindowTitle(tr("DeepSea Database 1.2.1"));
    setAcceptDrops(true);

    plot = new Plot(this);
    connect(plot, SIGNAL(fileCreated(QString,bool)), SLOT(addFile(QString,bool)));
    connect(plot, SIGNAL(fileChanged(QString, bool)), SLOT(updateFile(QString,bool)));
    connect(plot, SIGNAL(curveChanged(Curve*)), SLOT(onCurveColorChanged(Curve*)));

    addFolderAct = new QAction(qApp->style()->standardIcon(QStyle::SP_DialogOpenButton),
                               tr("Добавить папку"),this);
    addFolderAct->setShortcut(tr("Ctrl+O"));
    connect(addFolderAct, SIGNAL(triggered()), SLOT(addFolder()));

    delFilesAct = new QAction(QString("Удалить записи"), this);
    delFilesAct->setShortcut(Qt::Key_Delete);
    delFilesAct->setShortcutContext(Qt::WidgetShortcut);
    connect(delFilesAct, SIGNAL(triggered()), SLOT(deleteFiles()));

    plotAllChannelsAct = new QAction(QString("Построить все каналы"), this);
    connect(plotAllChannelsAct, SIGNAL(triggered()), SLOT(plotAllChannels()));

    convertAct = new QAction(QString("Конвертировать в..."), this);
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

    meanAct = new QAction(QString("Вывести среднее"), this);
    meanAct->setIcon(QIcon(":/icons/mean.png"));
    connect(meanAct, SIGNAL(triggered()), plot, SLOT(calculateMean()));

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
    addCorrectionAct->setEnabled(false);
    connect(addCorrectionAct, &QAction::triggered, [=](){
        CorrectionDialog dialog(plot);
        dialog.exec();
    });


    QMenu *fileMenu = menuBar()->addMenu(tr("Файл"));
    fileMenu->addAction(addFolderAct);

    QMenu *recordsMenu = menuBar()->addMenu(QString("Записи"));
    recordsMenu->addAction(delFilesAct);
    recordsMenu->addAction(rescanBaseAct);

    QMenu *settingsMenu = menuBar()->addMenu(QString("Настройки"));
    settingsMenu->addAction(editColorsAct);

    QToolBar *toolBar = new QToolBar(this);
    toolBar->setIconSize(QSize(24,24));
    toolBar->setOrientation(Qt::Vertical);
    toolBar->addAction(clearPlotAct);
    toolBar->addAction(savePlotAct);
    toolBar->addAction(copyToClipboardAct);
    toolBar->addAction(printPlotAct);
    toolBar->addAction(exportToExcelAct);
    toolBar->addAction(switchCursorAct);
    toolBar->addSeparator();
    toolBar->addAction(meanAct);
    toolBar->addAction(interactionModeAct);
    toolBar->addAction(addCorrectionAct);

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
    plotsLayout->addWidget(toolBar,0,0);
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
{
    tab = new Tab(this);
    tab->setOrientation(Qt::Horizontal);

    tab->tree = new QTreeWidget(this);
    tab->tree->setRootIsDecorated(false);
    tab->tree->setContextMenuPolicy(Qt::ActionsContextMenu);
    tab->tree->setSelectionMode(QAbstractItemView::ContiguousSelection);
    tab->tree->setDragEnabled(true);
    tab->tree->setDragDropMode(QAbstractItemView::InternalMove);
   // tab->tree->setDefaultDropAction(Qt::MoveAction);
    tab->tree->addAction(addFolderAct);
    tab->tree->addAction(delFilesAct);
    tab->tree->addAction(plotAllChannelsAct);
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


    tab->filePathLabel = new QLabel(this);
    tab->filePathLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);




    QWidget *treeWidget = new QWidget(this);
    QGridLayout *treeLayout = new QGridLayout;
    treeLayout->addWidget(tab->tree,0,0);
    treeWidget->setLayout(treeLayout);

    QWidget *channelsWidget = new QWidget(this);
    QGridLayout *channelsLayout = new QGridLayout;
    channelsLayout->addWidget(tab->filePathLabel,0,0);
    channelsLayout->addWidget(tab->channelsTable,1,0);
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
            addFolder(folder);
        }
    }
}

QString MainWindow::getNewSheetName()
{
    return QDateTime::currentDateTime().toString();
}


void MainWindow::createNewTab()
{
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
{
    if (tabWidget->count()==1) return;
    int index=i;
    if (i<0) index = tabWidget->currentIndex();

    tabWidget->setCurrentIndex(index);

    QVector<int> indexes;
    for (int row = 0; row < tab->tree->topLevelItemCount(); ++row)
        indexes << row;
    deleteFiles(indexes);

    QWidget *w = tabWidget->widget(index);
    tabWidget->removeTab(index);
    w->deleteLater();
}

void MainWindow::closeOtherTabs(int index)
{
    int count=tabWidget->count();
    if (count<=1) return;

    for (int i = count - 1; i > index; --i)
        closeTab(i);
    for (int i = index - 1; i >= 0; --i)
        closeTab(i);
}

void MainWindow::renameTab(int i)
{
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
{
    if (currentIndex<0) return;

    Tab *sp = qobject_cast<Tab *>(tabWidget->currentWidget());
    if (sp) {
        tab = sp;
    }
    else
        tab = 0;
}

void MainWindow::onTabTextChanged()
{
    QString s = tabWidget->tabText(tabWidget->currentIndex());
    tabWidget->setTabText(tabWidget->currentIndex(),s);
}

void MainWindow::editColors()
{
    ColorEditDialog dialog(this);
    dialog.exec();
}

MainWindow::~MainWindow()
{
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
    if (!map.isEmpty())
        setSetting("folders", map);

    ColorSelector::instance()->drop();
}

bool checkForContains(Tab *tab, const QString &file, int *index = 0)
{
    for (int i=0; i<tab->tree->topLevelItemCount(); ++i) {
        SortableTreeWidgetItem *item = dynamic_cast<SortableTreeWidgetItem *>(tab->tree->topLevelItem(i));
        if (item) {
            if (item->dfd->dfdFileName == file) {
                if (index) *index = i;
                return true;
            }
        }
    }

    if (index) *index = -1;
    return false;
}

void MainWindow::addFolder()
{
    QString directory = getSetting("lastDirectory").toString();

    directory = QFileDialog::getExistingDirectory(this,
                                                  tr("Add folder"),
                                                  directory,
                                                  QFileDialog::ShowDirsOnly | QFileDialog::ReadOnly);

    if (directory.isEmpty()) return;
    setSetting("lastDirectory", directory);
    addFolder(directory);
}

void MainWindow::addFolder(const QString &directory)
{
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
    if (toAdd.size() < filesToAdd.size()) {
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
{
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

void MainWindow::updateChannelsHeaderState()
{
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
{
    Q_UNUSED(previous)
    if (!tab) return;
    if (!current /*|| current==previous*/) return;

    SortableTreeWidgetItem *item = dynamic_cast<SortableTreeWidgetItem *>(current);
    if (!item) {
        record = 0;
        return;
    }

    updateChannelsTable(item->dfd);


}

void MainWindow::updateChannelsTable(DfdFileDescriptor *dfd)
{
    tab->channelsTable->blockSignals(true);
    tab->channelsTable->setRowCount(0);
    tab->channelsTable->blockSignals(false);

    if (!QFileInfo(dfd->dfdFileName).exists()
        || !QFileInfo(dfd->rawFileName).exists()) {
        QMessageBox::warning(this,"Не могу получить список каналов","Такого файла уже нет");
        return;
    }

    record = dfd;
    tab->filePathLabel->setText(record->dfdFileName);

    int chanCount = record->channels.size();
    if (chanCount == 0) return;

    QStringList headers = record->channels[0]->getInfoHeaders();

    tab->channelsTable->blockSignals(true);
    tab->channelsTable->clear();
    tab->channelsTable->setColumnCount(headers.size());
    tab->channelsTable->setHorizontalHeaderLabels(headers);
    tab->channelsTable->setRowCount(0);
    tab->channelsTable->setRowCount(chanCount);

    QFont boldFont = tab->channelsTable->font();
    boldFont.setBold(true);

    for (int i=0; i<chanCount; ++i) {
        Channel *ch = record->channels[i];
        QStringList data = ch->getInfoData();
        Qt::CheckState state = ch->checkState;
        for (int col=0; col<headers.size(); ++col) {
            QTableWidgetItem *ti = new QTableWidgetItem(data.at(col));
            if (col==0) {
                ti->setCheckState(state);
                if (state==Qt::Checked) ti->setFont(boldFont);
                if (ch->color.isValid()) {
                    ti->setTextColor(Qt::white);
                    ti->setBackgroundColor(ch->color);
                }
            }
            tab->channelsTable->setItem(i,col,ti);
        }
    }
    updateChannelsHeaderState();
    tab->channelsTable->blockSignals(false);
}

void MainWindow::maybePlotChannel(int currentRow, int currentColumn, int previousRow, int previousColumn)
{
    Q_UNUSED(previousColumn);
    if (currentRow<0 || currentColumn<0 || currentRow==previousRow) return;

    Channel *ch = record->channels.at(currentRow);

    if (!ch->populated)
        ch->populateData();

    plot->plotChannel(record, currentRow, false, 0);
}

bool allUnchecked(const QList<Channel *> &channels)
{
    foreach (Channel *c, channels) {
        if (c->checkState==Qt::Checked) return false;
    }
    return true;
}

void MainWindow::maybePlotChannel(QTableWidgetItem *item)
{
    if (!tab) return;
    if (!item) return;
    int column = item->column();
    if (column!=0) return;

    Qt::CheckState state = item->checkState();
    Channel *ch = record->channels[item->row()];

    QFont oldFont = tab->channelsTable->font();
    QFont boldFont = oldFont;
    boldFont.setBold(true);

    if (state == Qt::Checked && !ch->populated)
        ch->populateData();

    tab->channelsTable->blockSignals(true);
    bool plotted = true;

    if (state == Qt::Checked) {
        QColor col;
        plotted = plot->plotChannel(record, item->row(), true, &col);
        if (plotted) {
            item->setFont(boldFont);
            ch->checkState = Qt::Checked;
            ch->color = col;
            item->setBackgroundColor(col);
            item->setTextColor(Qt::white);
        }
        else {
            item->setCheckState(Qt::Unchecked);
            ch->checkState = Qt::Unchecked;
            ch->color = QColor();
            item->setTextColor(Qt::black);
        }
    }
    else if (state == Qt::Unchecked) {
        plot->deleteGraph(record, item->row());
        item->setFont(tab->channelsTable->font());
        ch->checkState = Qt::Unchecked;
        item->setBackgroundColor(Qt::white);
        item->setTextColor(Qt::black);
        ch->color = QColor();
    }
    tab->channelsTable->blockSignals(false);

    tab->tree->currentItem()->setFont(1, allUnchecked(record->channels)?oldFont:boldFont);

    if (tab->tableHeader->isSectionCheckable(column))
        updateChannelsHeaderState();
}

void MainWindow::plotAllChannels()
{
    if (!tab) return;
    for (int i=0; i<tab->channelsTable->rowCount(); ++i) {
        tab->channelsTable->item(i,0)->setCheckState(Qt::Checked);
    }
}

void MainWindow::convertRecords()
{
    if (!tab) return;
    QList<DfdFileDescriptor *> records;
    for (int i=0; i<tab->tree->topLevelItemCount(); ++i) {
        if (tab->tree->topLevelItem(i)->isSelected()) {
            SortableTreeWidgetItem *item = dynamic_cast<SortableTreeWidgetItem *>(tab->tree->topLevelItem(i));
            if (!item) continue;

            if (item->dfd->DataType>=SourceData && item->dfd->DataType<=15) {
                // only convert source files
                records << item->dfd;
            }
        }
    }
    if (records.isEmpty()) return;

    ConvertDialog dialog(&records, this);

    if (dialog.exec()) {

        QStringList newFiles = dialog.getNewFiles();
        for (int i=newFiles.size()-1; i>=0; --i) {
            if (checkForContains(tab, newFiles.at(i)))
                newFiles.removeAt(i);
        }

        addFiles(newFiles);
    }
}

void MainWindow::headerToggled(int column, Qt::CheckState state)
{
    if (!tab || column<0 || column >= tab->channelsTable->columnCount()) return;

    if (state == Qt::PartiallyChecked) return;
    for (int i=0; i<tab->channelsTable->rowCount(); ++i)
        tab->channelsTable->item(i,column)->setCheckState(state);
}

void MainWindow::clearPlot()
{
    plot->deleteGraphs();
    for (int index = 0; index<tabWidget->count(); ++index) {
        Tab *t = qobject_cast<Tab*>(tabWidget->widget(index));
        if (t) {
            for (int i=0; i<t->tree->topLevelItemCount(); ++i) {
                DfdFileDescriptor *dfd = dynamic_cast<SortableTreeWidgetItem *>(t->tree->topLevelItem(i))->dfd;
                QList<Channel *> list = dfd->channels;
                foreach (Channel *c, list) {
                    c->checkState = Qt::Unchecked;
                    c->color = QColor();
                }
                t->tree->topLevelItem(i)->setFont(1, t->tree->font());
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
{
    if (!item || column!=9) return;

    SortableTreeWidgetItem *i = dynamic_cast<SortableTreeWidgetItem *>(item);
    if (i) {
        if (!QFileInfo(i->dfd->dfdFileName).exists()
            || !QFileInfo(i->dfd->rawFileName).exists()) {
            QMessageBox::warning(this,"Не могу изменить легенду","Такого файла уже нет");
            return;
        }
        i->dfd->setLegend(item->text(column));
        plot->updateLegends();
    }
}

void MainWindow::rescanBase()
{
    if (!tab) return;

    // first we delete all graphs affected
    for (int i=0; i<tab->tree->topLevelItemCount(); ++i) {
        SortableTreeWidgetItem *item = dynamic_cast<SortableTreeWidgetItem *>(tab->tree->topLevelItem(i));
        if (item) {
            plot->deleteGraphs(item->dfd->DFDGUID);
        }
    }

    // next we clear all tab and populate it with folders anew
    tab->tree->clear();
    tab->channelsTable->setRowCount(0);

    tab->filePathLabel->clear();

    QStringList folders = tab->folders;
    foreach (const QString &folder, folders)
        addFolder(folder);

    QMessageBox::information(this, "База данных", "В базе данных все записи \"живые\"!");
}

QVariant MainWindow::getSetting(const QString &key, const QVariant &defValue)
{
    QSettings se("Alex Novichkov","DeepSea Database");
    return se.value(key, defValue);
}

void MainWindow::setSetting(const QString &key, const QVariant &value)
{
    QSettings se("Alex Novichkov","DeepSea Database");
    se.setValue(key, value);
}

void MainWindow::addFile(const QString &fileName, bool plot)
{
    if (!tab) return;

    int index = -1;
    checkForContains(tab, fileName, &index);
    // такого файла еще не было
    if (index < 0) {
        int count = tab->tree->topLevelItemCount();
        addFiles(QStringList()<<fileName);

        if (tab->tree->topLevelItemCount()>count) {
            tab->tree->setCurrentItem(tab->tree->topLevelItem(count));

        }
    }
    else {
        // перечитываем данные файла
        QString pos = tab->tree->topLevelItem(index)->text(0);
        deleteFiles(QVector<int>()<<index);

        DfdFileDescriptor *dfd = new DfdFileDescriptor(fileName);
        dfd->read();

        QTreeWidgetItem *item =
                new SortableTreeWidgetItem(dfd,
                                           QStringList()
                                           << pos // QString("№") 0
                                           << QFileInfo(dfd->dfdFileName).completeBaseName() //QString("Файл") 1
                                           << QDateTime(dfd->Date,dfd->Time).toString(dateTimeFormat) // QString("Дата") 2
                                           << dataTypeDescription(dfd->DataType) // QString("Тип") 3
                                           << QString::number(dfd->NumInd * dfd->XStep) // QString("Размер") 4
                                           << dfd->XName // QString("Ось Х") 5
                                           << QString::number(dfd->XStep) // QString("Шаг") 6
                                           << QString::number(dfd->NumChans) // QString("Каналы")); 7
                                           << dfd->description()
                                           << dfd->legend()
                                           );
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
        tab->tree->insertTopLevelItem(index, item);

        tab->tree->setCurrentItem(0);
        tab->tree->setCurrentItem(tab->tree->topLevelItem(index));
    }
    if (plot)
        headerToggled(0, Qt::Checked);
}

void MainWindow::updateFile(const QString &fileName, bool plot)
{
    if (!tab) return;

    int index = -1;
    checkForContains(tab, fileName, &index);
    // такого файла еще не было
    if (index >= 0) {
        // перечитываем данные файла
        SortableTreeWidgetItem *item = dynamic_cast<SortableTreeWidgetItem *>(tab->tree->topLevelItem(index));
        if (!item) return;

        DfdFileDescriptor *dfd = item->dfd;
        item->setText(2, QDateTime(dfd->Date,dfd->Time).toString(dateTimeFormat));
        item->setText(3, dataTypeDescription(dfd->DataType));
        item->setText(7, QString::number(dfd->NumChans));

        tab->tree->setCurrentItem(0);
        tab->tree->setCurrentItem(tab->tree->topLevelItem(index));
    }
    if (plot)
        tab->channelsTable->item(tab->channelsTable->rowCount()-1, 0)->setCheckState(Qt::Checked);
}

void setLineColor(QAxObject *obj, int color)
{
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
{
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
{
    static QAxObject *excel = 0;

    if (!plot->hasGraphs()) {
        QMessageBox::warning(this, "Графиков нет", "Нечего тут экспортировать!");
        return;
    }

    if (!excel) excel = new QAxObject("Excel.Application", this);
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
    QString newSheetName = getNewSheetName();
    QAxObject * worksheet = workbook->querySubObject("ActiveSheet");
    //worksheet->setProperty("Name", newSheetName);

    // экспортируем данные графиков на лист
     QList<Curve *> curves = plot->curves();

     // проверяем, все ли каналы из одного файла
     DfdFileDescriptor *dfd = curves.at(0)->dfd;
     bool allChannelsFromOneFile = true;
     for (int i=1; i<curves.size(); ++i) {
         if (curves.at(i)->dfd != dfd) {
             allChannelsFromOneFile = false;
             break;
         }
     }

     //проверяем, все ли каналы имеют одинаковое разрешение по х
     bool allChannelsHaveSameXStep = true;

     if (!allChannelsFromOneFile) {
         for (int i=1; i<curves.size(); ++i) {
             if (qAbs(curves.at(i)->dfd->XStep - dfd->XStep) >= 1e-10) {
                 allChannelsHaveSameXStep = false;
                 break;
             }
         }
     }

     // ищем максимальное количество отсчетов
     // и максимальный шаг
     quint32 maxInd = dfd->NumInd;
     double maxStep = dfd->XStep;

     for (int i=1; i<curves.size(); ++i) {
         if (curves.at(i)->dfd->NumInd > maxInd) {
             maxInd = curves.at(i)->dfd->NumInd;
         }
         if (qAbs(curves.at(i)->dfd->XStep - maxStep) > 1e-10) {
             maxStep = curves.at(i)->dfd->XStep;
         }
     }


     // записываем название файла
     if (allChannelsFromOneFile) {
         QAxObject *cells = worksheet->querySubObject("Cells(Int,Int)", 1, 1);
         if (cells) cells->setProperty("Value", dfd->dfdFileName);
         delete cells;
     }
     else {
         for (int i=0; i<curves.size(); ++i) {
             Curve *curve = curves.at(i);
             QAxObject *cells = allChannelsHaveSameXStep ? worksheet->querySubObject("Cells(Int,Int)", 1, 2+i)
                                                         : worksheet->querySubObject("Cells(Int,Int)", 1, 2+i*2);
             cells->setProperty("Value", curve->dfd->dfdFileName);
             delete cells;
         }
     }

     // записываем название канала
     for (int i=0; i<curves.size(); ++i) {
         Curve *curve = curves.at(i);
         QAxObject *cells = allChannelsHaveSameXStep ? worksheet->querySubObject("Cells(Int,Int)", 2, 2+i)
                                                     : worksheet->querySubObject("Cells(Int,Int)", 2, 2+i*2);
         cells->setProperty("Value", curve->title().text());
         delete cells;
     }

     // если все каналы имеют одинаковый шаг по х, то в первый столбец записываем
     // данные х
     // если каналы имеют разный шаг по х, то для каждого канала отдельно записываем
     // по два столбца
     if (allChannelsHaveSameXStep) {
         for (uint i = 0; i < maxInd; ++i) {
             QAxObject *cells = worksheet->querySubObject("Cells(Int,Int)", 3+i, 1);
             cells->setProperty("Value", dfd->XBegin + i*dfd->XStep);
             delete cells;
         }
     }

     // записываем данные
     if (allChannelsHaveSameXStep) {
         const quint32 numRows = maxInd;
         const quint32 numCols = curves.size();
         QAxObject* Cell1 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 3, 2);
         QAxObject* Cell2 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 3 + numRows - 1, 2 + numCols - 1);
         QAxObject* range = worksheet->querySubObject("Range(const QVariant&,const QVariant&)", Cell1->asVariant(), Cell2->asVariant() );
         QList<QVariant> cellsList;
         QList<QVariant> rowsList;
         for (uint i = 0; i < numRows; i++) {
             cellsList.clear();
             for (uint j = 0; j < numCols; j++)
                 cellsList << ((curves.at(j)->dfd->NumInd < maxInd) ? 0 : curves.at(j)->dfd->channels.at(curves.at(j)->channel)->yValues[i]);
             rowsList << QVariant(cellsList);
         }
         range->setProperty("Value", QVariant(rowsList) );

         // выделяем диапазон, чтобы он автоматически использовался для построения диаграммы
         Cell1 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 2, 1);
         Cell2 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 2 + numRows, 1 + numCols);
         range = worksheet->querySubObject("Range(const QVariant&,const QVariant&)", Cell1->asVariant(), Cell2->asVariant() );
         range->dynamicCall("Select (void)");

         delete range;
         delete Cell1;
         delete Cell2;
     }
     else {
         for (int i=0; i<curves.size(); ++i) {
             Curve *curve = curves.at(i);
             Channel *channel = curve->dfd->channels.at(curve->channel);

             QAxObject* Cell1 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 3, 1+i*2);
             QAxObject* Cell2 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 3 + curve->dfd->NumInd-1, 2 + i*2);
             QAxObject* range = worksheet->querySubObject("Range(const QVariant&,const QVariant&)", Cell1->asVariant(), Cell2->asVariant() );

             QList<QVariant> cellsList;
             QList<QVariant> rowsList;
             for (uint j = 0; j < curve->dfd->NumInd; j++) {
                 cellsList.clear();
                 cellsList << (curve->dfd->XBegin + j*curve->dfd->XStep) << channel->yValues[j];
                 rowsList << QVariant(cellsList);
             }
             range->setProperty("Value", QVariant(rowsList) );

             delete Cell1;
             delete Cell2;
             delete range;
         }
     }

     QAxObject *charts = workbook->querySubObject("Charts");
     charts->dynamicCall("Add()");
     QAxObject *chart = workbook->querySubObject("ActiveChart");
//     chart->dynamicCall("Select()");
//     chart->setProperty("Name", "Диагр."+newSheetName);
     chart->setProperty("ChartType", 75);

     QAxObject * series = chart->querySubObject("SeriesCollection");

     // отдельно строить кривые нужно, только если у нас много пар столбцов с данными
     if (!allChannelsHaveSameXStep) {
         for (int i=0; i<curves.size(); ++i) {
             Curve *curve = curves.at(i);
             QAxObject * serie = series->querySubObject("Item (int)", i+1);

             QAxObject* Cell1 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 3, 1+i*2);
             QAxObject* Cell2 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 3 + curve->dfd->NumInd-1, 1 + i*2);
             QAxObject * xvalues = worksheet->querySubObject("Range(const QVariant&,const QVariant&)", Cell1->asVariant(), Cell2->asVariant());
             serie->setProperty("XValues", xvalues->asVariant());

             delete Cell1;
             delete Cell2;

             Cell1 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 3, 2+i*2);
             Cell2 = worksheet->querySubObject("Cells(QVariant&,QVariant&)", 3 + curve->dfd->NumInd-1, 2 + i*2);
             QAxObject * yvalues = worksheet->querySubObject("Range(const QVariant&,const QVariant&)", Cell1->asVariant(), Cell2->asVariant());
             serie->setProperty("Values", yvalues->asVariant());

             serie->setProperty("Name", curve->dfd->channels.at(curve->channel)->ChanName);

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
     xAxis->setProperty("MaximumScale", maxInd * maxStep);

     QAxObject *yAxis = chart->querySubObject("Axes(const QVariant&)", 2);
     setAxis(yAxis, "Уровень, дБ");

     // рамка вокруг графика
     QAxObject *plotArea = chart->querySubObject("PlotArea");
     setLineColor(plotArea, 13);
     delete plotArea;

     // цвета графиков
     for (int i = 0; i< curves.size(); ++i) {
         QAxObject * serie = series->querySubObject("Item(int)", i+1);

         QAxObject *format = serie->querySubObject("Format");
         QAxObject *formatLine = format->querySubObject("Line");
         formatLine->setProperty("Weight", 1);

         QAxObject *formatLineForeColor = formatLine->querySubObject("ForeColor");
         formatLineForeColor->setProperty("RGB", curves.at(i)->pen().color().rgb());

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


//    QFile file1("chartArea.html");
//    file1.open(QIODevice::WriteOnly | QIODevice::Text);
//    QTextStream out(&file1);
//    out << chartArea->generateDocumentation();
//    file1.close();

     delete series;
     delete chart;
     delete charts;
     delete worksheet;
     delete worksheets;
     delete workbook;
     delete workbooks;
}

void MainWindow::onCurveColorChanged(Curve *curve)
{
    if (record == curve->dfd)
        updateChannelsTable(curve->dfd);
}

void MainWindow::addFiles(const QStringList &files)
{
    if (!tab) return;

    QList<QTreeWidgetItem *> items;

    int pos = tab->tree->topLevelItemCount();

    for (int i=0; i<files.size(); ++i) {
        QString file = files[i];
        DfdFileDescriptor *dfd = new DfdFileDescriptor(file);
        dfd->read();

        QTreeWidgetItem *item =
                new SortableTreeWidgetItem(dfd,
                                           QStringList()
                                           << QString::number(++pos) // QString("№") 0
                                           << QFileInfo(dfd->dfdFileName).completeBaseName() //QString("Файл") 1
                                           << QDateTime(dfd->Date,dfd->Time).toString(dateTimeFormat) // QString("Дата") 2
                                           << dataTypeDescription(dfd->DataType) // QString("Тип") 3
                                           << QString::number(dfd->NumInd * dfd->XStep) // QString("Размер") 4
                                           << dfd->XName // QString("Ось Х") 5
                                           << QString::number(dfd->XStep) // QString("Шаг") 6
                                           << QString::number(dfd->NumChans) // QString("Каналы")); 7
                                           << dfd->description()
                                           << dfd->legend()
                                           );
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
        items << item;
    }
    tab->tree->setSortingEnabled(false);
    tab->tree->addTopLevelItems(items);
//    tab->tree->sortItems(0,Qt::AscendingOrder);
    tab->tree->setSortingEnabled(true);
}

void MainWindow::deleteFiles(const QVector<int> &indexes)
{
    if (!tab) return;
    bool taken = false;

    for (int i=indexes.size()-1; i>=0; --i) {
        SortableTreeWidgetItem *item = dynamic_cast<SortableTreeWidgetItem *>(tab->tree->topLevelItem(indexes.at(i)));
        if (item) {
            plot->deleteGraphs(item->dfd->DFDGUID);
        }
        delete tab->tree->takeTopLevelItem(indexes.at(i));
        taken = true;
    }
    if (taken) {
        tab->channelsTable->setRowCount(0);
    }
}