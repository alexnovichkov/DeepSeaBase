#include "mainwindow.h"

#include <QtWidgets>

#include "convertdialog.h"
#include "sortabletreewidgetitem.h"
#include "checkableheaderview.h"
#include "plot.h"
#include "tabwidget.h"



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
    : QMainWindow(parent), record(0)
{
    setWindowTitle(tr("DeepSea Database"));
    setAcceptDrops(true);

    plot = new Plot(this);

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

    QMenu *fileMenu = menuBar()->addMenu(tr("Файл"));
    fileMenu->addAction(addFolderAct);

    QMenu *recordsMenu = menuBar()->addMenu(QString("Записи"));
    recordsMenu->addAction(delFilesAct);
    recordsMenu->addAction(rescanBaseAct);

    QToolBar *toolBar = new QToolBar(this);
    toolBar->setOrientation(Qt::Vertical);
    toolBar->addAction(clearPlotAct);
    toolBar->addAction(savePlotAct);
    toolBar->addAction(copyToClipboardAct);
    toolBar->addAction(printPlotAct);
    toolBar->addAction(switchCursorAct);

    tabWidget = new TabWidget(this);
    connect(tabWidget,SIGNAL(newTab()),this, SLOT(createNewTab()));
    connect(tabWidget,SIGNAL(closeTab(int)),this, SLOT(closeTab(int)));
    connect(tabWidget,SIGNAL(closeOtherTabs(int)), this, SLOT(closeOtherTabs(int)));
    connect(tabWidget,SIGNAL(renameTab(int)),this, SLOT(renameTab(int)));
    connect(tabWidget,SIGNAL(currentChanged(int)),SLOT(changeCurrentTab(int)));
    connect(tabWidget,SIGNAL(tabTextChanged(QString)),SLOT(onTabTextChanged()));

    //createNewTab();



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



    QVariantMap v = getSetting("files").toMap();
    if (v.isEmpty())
        createNewTab();
    else {
        QMapIterator<QString, QVariant> it(v);
        while (it.hasNext()) {
            it.next();
            createTab(it.key(), it.value().toList());
            tabsNames << it.key();
        }
    }
}

void MainWindow::createTab(const QString &name, const QVariantList &files)
{
    QTreeWidget *tree = new QTreeWidget(this);
    tree->setRootIsDecorated(false);
    tree->setContextMenuPolicy(Qt::ActionsContextMenu);
    tree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    tree->addAction(addFolderAct);
    tree->addAction(delFilesAct);
    tree->addAction(plotAllChannelsAct);
    tree->addAction(convertAct);
    tree->setHeaderLabels(QStringList()
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
    tree->header()->setStretchLastSection(false);
    tree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    tree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    tree->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    tree->header()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    tree->header()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    tree->header()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    tree->header()->setSectionResizeMode(7, QHeaderView::ResizeToContents);

    QByteArray treeHeaderState = getSetting("treeHeaderState").toByteArray();
    if (!treeHeaderState.isEmpty())
        tree->header()->restoreState(treeHeaderState);


    connect(tree, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),SLOT(updateChannelsTable(QTreeWidgetItem*,QTreeWidgetItem*)));
    connect(tree,SIGNAL(itemChanged(QTreeWidgetItem*,int)), SLOT(recordLegendChanged(QTreeWidgetItem*, int)));
    tree->sortByColumn(0, Qt::AscendingOrder);
//    tree->setSortingEnabled(true);

    this->tree = tree;

    QTableWidget *channelsTable = new QTableWidget(0,6,this);
    this->channelsTable = channelsTable;


    CheckableHeaderView *tableHeader = new CheckableHeaderView(Qt::Horizontal, channelsTable);
    this->tableHeader = tableHeader;
    channelsTable->setHorizontalHeader(tableHeader);
    tableHeader->setCheckState(0,Qt::Checked);
    tableHeader->setCheckable(0,true);
    tableHeader->setCheckState(0,Qt::Unchecked);
    connect(tableHeader,SIGNAL(toggled(int,Qt::CheckState)),this,SLOT(headerToggled(int,Qt::CheckState)));

    channelsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    channelsTable->horizontalHeader()->setStretchLastSection(false);
    //connect(channelsTable, SIGNAL(currentCellChanged(int,int,int,int)),this,SLOT(maybePlotChannel(int,int,int,int)));
    connect(channelsTable, SIGNAL(itemChanged(QTableWidgetItem*)), SLOT(maybePlotChannel(QTableWidgetItem*)));


    QLabel *filePathLabel = new QLabel(this);
    filePathLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    this->filePathLabel = filePathLabel;



    QWidget *treeWidget = new QWidget(this);
    QGridLayout *treeLayout = new QGridLayout;
    treeLayout->addWidget(tree,0,0);
    treeWidget->setLayout(treeLayout);

    QWidget *channelsWidget = new QWidget(this);
    QGridLayout *channelsLayout = new QGridLayout;
    channelsLayout->addWidget(filePathLabel,0,0);
    channelsLayout->addWidget(channelsTable,1,0);
    channelsWidget->setLayout(channelsLayout);

    Tab *upperSplitter = new Tab(this);
    upperSplitter->setOrientation(Qt::Horizontal);
    upperSplitter->addWidget(treeWidget);
    upperSplitter->addWidget(channelsWidget);

    upperSplitter->tree = tree;
    upperSplitter->channelsTable = channelsTable;
    upperSplitter->filePathLabel = filePathLabel;
    upperSplitter->tableHeader = tableHeader;


    QByteArray upperSplitterState = getSetting("upperSplitterState").toByteArray();
    if (!upperSplitterState.isEmpty())
        upperSplitter->restoreState(upperSplitterState);

    int i = tabWidget->addTab(upperSplitter, name);
    tabWidget->setCurrentIndex(i);

    foreach (const QVariant &item, files) {
        QStringList rec = item.toStringList();
        if (QFileInfo(rec.first()).exists())
            upperSplitter->files.append(rec);
    }

    if (!upperSplitter->files.isEmpty())
        addFiles(upperSplitter->files, false);
}


void MainWindow::createNewTab()
{
    static int sequenceNumber = 1;
    QString name = tr("Вкладка %1").arg(sequenceNumber);
    while (tabsNames.contains(name)) {
        sequenceNumber++;
        name = tr("Вкладка %1").arg(sequenceNumber);
    }
    createTab(name, QVariantList());
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
    for (int row = 0; row < tree->topLevelItemCount(); ++row)
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
        tree = sp->tree;
        channelsTable = sp->channelsTable;
        filePathLabel = sp->filePathLabel;
        tableHeader = sp->tableHeader;
        alreadyAddedFiles = &sp->files;
    }
}

void MainWindow::onTabTextChanged()
{
    QString s = tabWidget->tabText(tabWidget->currentIndex());
    tabWidget->setTabText(tabWidget->currentIndex(),s);
}

MainWindow::~MainWindow()
{
    setSetting("mainSplitterState",splitter->saveState());

    if (tabWidget->currentWidget()) {
        QSplitter *upperSplitter = qobject_cast<QSplitter *>(tabWidget->currentWidget());
        if (upperSplitter)
            setSetting("upperSplitterState",upperSplitter->saveState());
    }

    QVariantMap legends;

    QVariantMap map;
    for (int i=0; i<tabWidget->count(); ++i) {
        Tab *t = qobject_cast<Tab *>(tabWidget->widget(i));
        if (t) {
            QVariantList list;
            foreach(const QStringList &item, t->files)
                list << QVariant(item);
            map.insert(tabWidget->tabText(i), list);

            for (int i=0; i<t->tree->topLevelItemCount(); ++i) {
                SortableTreeWidgetItem *item =
                        dynamic_cast<SortableTreeWidgetItem *>(t->tree->topLevelItem(i));
                if (item) {
                    if (!item->text(9).isEmpty())
                        legends.insert(item->dfd->DFDGUID, item->text(9));
                }
            }
        }
    }
    setSetting("files", map);
    setSetting("legends", legends);

    QByteArray treeHeaderState = tree->header()->saveState();
    setSetting("treeHeaderState", treeHeaderState);
}

bool checkForContains(QList<QStringList> *list, const QString &file)
{
    foreach (const QStringList &item, *list) {
        if (item.first() == file) return true;
    }
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

    QStringList filesToAdd;
    processDir(directory, filesToAdd, true);

    QStringList toAdd;
    foreach (const QString &file, filesToAdd) {
        if (!checkForContains(alreadyAddedFiles, file))
            toAdd << file;
    }

    addFiles(toAdd, true);
    if (toAdd.size() < filesToAdd.size()) {
        QMessageBox::information(this,QString("База данных"),
                                 toAdd.isEmpty()?QString("Эти файлы уже есть в базе"):
                                                 QString("Только %1 файлов из %2 было добавлено")
                                                 .arg(toAdd.size())
                                                 .arg(filesToAdd.size()));
    }
}

void MainWindow::addExistingFiles()
{
    addFiles(*alreadyAddedFiles, false);
}

void MainWindow::deleteFiles()
{
    QVector<int> list;
    for (int i=0; i<tree->topLevelItemCount(); ++i) {
        if (tree->topLevelItem(i)->isSelected()) {
            list << i;
        }
    }
    if (!list.isEmpty())
        deleteFiles(list);
}

void MainWindow::updateChannelsHeaderState()
{
    if (!channelsTable || !tableHeader) return;
    int checked=0;
    const int column = 0;
    for (int i=0; i<channelsTable->rowCount(); ++i) {
        if (channelsTable->item(i, column) && channelsTable->item(i,column)->checkState()==Qt::Checked)
            checked++;
    }

    if (checked==0)
        tableHeader->setCheckState(column, Qt::Unchecked);
    else if (checked==channelsTable->rowCount())
        tableHeader->setCheckState(column, Qt::Checked);
    else
        tableHeader->setCheckState(column, Qt::PartiallyChecked);
}

void MainWindow::rescanDeadRecords()
{
    QHash<QString, DfdFileDescriptor *> allRecords;
    QVector<int> list;

    DrivesDialog dialog(this);
    if (dialog.exec()) {
        QApplication::setOverrideCursor(Qt::WaitCursor);
        QStringList drives = dialog.drives;
        if (drives.isEmpty()) return;

        QStringList filesToAdd;

        foreach (const QString &drive, drives) {
            processDir(drive, filesToAdd, true);
        }

        if (!filesToAdd.isEmpty()) {
            foreach (const QString &file, filesToAdd) {
                DfdFileDescriptor *dfd = new DfdFileDescriptor(file);
                dfd->read();
                allRecords.insert(dfd->DFDGUID, dfd);
            }

            for (int i = 0; i<tree->topLevelItemCount(); ++i) {
                SortableTreeWidgetItem *item = dynamic_cast<SortableTreeWidgetItem *>(tree->topLevelItem(i));
                if (item) {
                    if (!QFileInfo(item->dfd->dfdFileName).exists()) {
                        // или заменяем на актуальный, или удаляем из таблицы
                        if (allRecords.contains(item->dfd->DFDGUID)) {
                            delete item->dfd;
                            item->dfd = allRecords.value(item->dfd->DFDGUID);
                            allRecords.remove(item->dfd->DFDGUID);
                            //обновляем текст записи
                            item->setText(1, QFileInfo(item->dfd->dfdFileName).completeBaseName());
                            item->setText(2, QDateTime(item->dfd->Date,item->dfd->Time).toString(dateTimeFormat));
                            item->setText(3, dataTypeDescription(item->dfd->DataType));
                            item->setText(4, QString::number(item->dfd->NumInd * item->dfd->XStep));
                            item->setText(5, item->dfd->XName);
                            item->setText(6, QString::number(item->dfd->XStep));
                            item->setText(7, QString::number(item->dfd->NumChans));
                            item->setText(8, item->dfd->description());
                            item->setText(9, item->dfd->legend());
                            (*alreadyAddedFiles)[i][0] = item->dfd->dfdFileName;
                            (*alreadyAddedFiles)[i][1] = item->dfd->DFDGUID;
                        }
                        else {
                            list << i;
                        }
                    }
                }
            }

        }
        QApplication::restoreOverrideCursor();
    }
    if (!list.isEmpty()) {
        deleteFiles(list);
        QMessageBox::information(this, "База данных", QString("Удалено %1 записей").arg(list.size()));
    }
    qDeleteAll(allRecords.values());
}

void MainWindow::removeDeadRecords()
{
    QVector<int> list;

    for (int i = 0; i<tree->topLevelItemCount(); ++i) {
        SortableTreeWidgetItem *item = dynamic_cast<SortableTreeWidgetItem *>(tree->topLevelItem(i));
        if (item) {
            if (!QFileInfo(item->dfd->dfdFileName).exists()) {
                list << i;
            }
        }
    }

    if (!list.isEmpty())
        deleteFiles(list);
}

void MainWindow::updateChannelsTable(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    Q_UNUSED(previous)
    if (!current /*|| current==previous*/) return;

    SortableTreeWidgetItem *item = dynamic_cast<SortableTreeWidgetItem *>(current);

    record = item?item->dfd:0;
    filePathLabel->setText(record->dfdFileName);

    int chanCount = record->channels.size();
    if (chanCount == 0) return;

    QStringList headers = record->channels[0]->getHeaders();

    channelsTable->blockSignals(true);
    channelsTable->clear();
    channelsTable->setColumnCount(headers.size());
    channelsTable->setHorizontalHeaderLabels(headers);
    channelsTable->setRowCount(0);
    channelsTable->setRowCount(chanCount);

    QFont boldFont = channelsTable->font();
    boldFont.setBold(true);

    for (int i=0; i<chanCount; ++i) {
        QStringList data = record->channels[i]->getData();
        Qt::CheckState state = record->channels[i]->checkState;
        for (int col=0; col<headers.size(); ++col) {
            QTableWidgetItem *ti = new QTableWidgetItem(data.at(col));
            if (col==0) {
                ti->setCheckState(state);
                if (state==Qt::Checked) ti->setFont(boldFont);
            }
            channelsTable->setItem(i,col,ti);
        }
    }
    updateChannelsHeaderState();
    //channelsTable->resizeColumnsToContents();
    channelsTable->blockSignals(false);
}

void MainWindow::maybePlotChannel(int currentRow, int currentColumn, int previousRow, int previousColumn)
{
    Q_UNUSED(previousColumn);
    if (currentRow<0 || currentColumn<0 || currentRow==previousRow) return;

    Channel *ch = record->channels.at(currentRow);

    if (!ch->populated)
        ch->populateData();

    plot->plotChannel(record, currentRow, false);
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
    if (!channelsTable || !tree || !tableHeader) return;
    if (!item) return;
    int column = item->column();
    if (column!=0) return;

    Qt::CheckState state = item->checkState();
    Channel *ch = record->channels[item->row()];

    QFont oldFont = channelsTable->font();
    QFont boldFont = oldFont;
    boldFont.setBold(true);

    if (state == Qt::Checked && !ch->populated)
        ch->populateData();

    channelsTable->blockSignals(true);
    bool plotted = true;

    if (state == Qt::Checked) {
        plotted = plot->plotChannel(record, item->row(), true);
        if (plotted) {
            item->setFont(boldFont);
            ch->checkState = Qt::Checked;
        }
        else {
            item->setCheckState(Qt::Unchecked);
            ch->checkState = Qt::Unchecked;
        }
    }
    else if (state == Qt::Unchecked) {
        plot->deleteGraph(record, item->row());
        item->setFont(channelsTable->font());
        ch->checkState = Qt::Unchecked;
    }
    channelsTable->blockSignals(false);

    tree->currentItem()->setFont(1, allUnchecked(record->channels)?oldFont:boldFont);

    if (tableHeader->isSectionCheckable(column))
        updateChannelsHeaderState();
}

//void MainWindow::graphClicked(QCPAbstractPlottable *plottable)
//{
//    Q_UNUSED(plottable);
//    statusBar->showMessage(QString("Clicked on graph '%1'.").arg(plottable->name()), 1000);
//}

void MainWindow::plotSelectionChanged()
{
//    // synchronize selection of graphs with selection of corresponding legend items:
//    for (int i=0; i<plot->graphCount(); ++i)
//    {
//        QCPGraph *graph = plot->graph(i);
//        QCPPlottableLegendItem *item = plot->legend->itemWithPlottable(graph);
//        if (item->selected() || graph->selected())
//        {
//            item->setSelected(true);
//            graph->setSelected(true);
//        }
//    }
}

void MainWindow::plotAllChannels()
{
    for (int i=0; i<channelsTable->rowCount(); ++i) {
        channelsTable->item(i,0)->setCheckState(Qt::Checked);
    }
}

void MainWindow::convertRecords()
{
    QList<DfdFileDescriptor *> records;
    for (int i=0; i<tree->topLevelItemCount(); ++i) {
        if (tree->topLevelItem(i)->isSelected()) {
            SortableTreeWidgetItem *item = dynamic_cast<SortableTreeWidgetItem *>(tree->topLevelItem(i));
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
            if (checkForContains(alreadyAddedFiles, newFiles.at(i)))
                newFiles.removeAt(i);
        }

        addFiles(newFiles, true);
    }
}

void MainWindow::headerToggled(int column, Qt::CheckState state)
{
    if (column<0 || column >= channelsTable->columnCount()) return;

    if (state == Qt::PartiallyChecked) return;
    for (int i=0; i<channelsTable->rowCount(); ++i)
        channelsTable->item(i,column)->setCheckState(state);
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
                foreach (Channel *c, list) c->checkState = Qt::Unchecked;
                t->tree->topLevelItem(i)->setFont(1, t->tree->font());
            }
            t->channelsTable->blockSignals(true);
            for (int i=0; i<t->channelsTable->rowCount(); ++i) {
                t->channelsTable->item(i,0)->setCheckState(Qt::Unchecked);
                t->channelsTable->item(i,0)->setFont(t->channelsTable->font());
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
        i->dfd->setLegend(item->text(column));
        plot->updateLegends();
    }
}

void MainWindow::rescanBase()
{
    for (int i=0; i<tree->topLevelItemCount(); ++i) {
        SortableTreeWidgetItem *item = dynamic_cast<SortableTreeWidgetItem *>(tree->topLevelItem(i));
        if (item) {
            if (!QFileInfo(item->dfd->dfdFileName).exists()) {
                QMessageBox msgBox;
                msgBox.setIcon(QMessageBox::Warning);
                msgBox.setText("Найдены мертвые записи:\n"
                               +item->dfd->dfdFileName);
                msgBox.setInformativeText("Пересканировать жесткие диски?");
                QPushButton *rescanButton = msgBox.addButton(tr("Да, пересканировать"), QMessageBox::ActionRole);
                QPushButton *removeButton = msgBox.addButton(tr("Нет, только удалить из списка"), QMessageBox::NoRole);
                msgBox.addButton(QMessageBox::Abort);

                msgBox.exec();

                if (msgBox.clickedButton() == rescanButton) {
                    rescanDeadRecords();
                }
                else if (msgBox.clickedButton() == removeButton) {
                    removeDeadRecords();
                }
                return;
            }
        }
    }
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

void MainWindow::addFiles(const QStringList &files, bool addToDatabase)
{
    QList<QStringList> list;
    foreach (const QString &file, files)
        list << (QStringList()<<file<<"");
    addFiles(list, addToDatabase);
}

void MainWindow::addFiles(QList<QStringList> &files, bool addToDatabase)
{
    QList<QTreeWidgetItem *> items;
    QList<DfdFileDescriptor *> dfds;
    int pos = tree->topLevelItemCount();

    for (int i=0; i<files.size(); ++i) {
        QStringList file = files[i];
        DfdFileDescriptor *dfd = new DfdFileDescriptor(file.first());
        dfd->read();
        dfds << dfd;
        if (dfd->DFDGUID.isEmpty()) dfd->DFDGUID = file.last();
        if (file.last().isEmpty()) {
            file[1] = dfd->DFDGUID;
            files[i] = file;
        }

        if (addToDatabase)
            alreadyAddedFiles->append(file);

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
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        items << item;
    }
    tree->setSortingEnabled(false);
    tree->addTopLevelItems(items);
//    tree->sortItems(0,Qt::AscendingOrder);
    tree->setSortingEnabled(true);
}

void MainWindow::deleteFiles(const QVector<int> &indexes)
{
    bool taken = false;

    for (int i=indexes.size()-1; i>=0; --i) {
        SortableTreeWidgetItem *item = dynamic_cast<SortableTreeWidgetItem *>(tree->topLevelItem(indexes.at(i)));
        if (item) {
            plot->deleteGraphs(item->dfd->DFDGUID);
        }
        delete tree->takeTopLevelItem(indexes.at(i));
        alreadyAddedFiles->removeAt(indexes.at(i));
        taken = true;
    }
    if (taken) {
        channelsTable->setRowCount(0);
    }
}

void MainWindow::updateRecordState(int recordIndex)
{

}
