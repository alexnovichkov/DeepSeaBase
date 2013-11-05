#include "mainwindow.h"

#include <QtWidgets>

#include "convertdialog.h"
#include "sortabletreewidgetitem.h"
#include "checkableheaderview.h"
#include "plot.h"
#include "tabwidget.h"
#include "colorselector.h"
#include "coloreditdialog.h"

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
    setWindowTitle(tr("DeepSea Database"));
    setAcceptDrops(true);

    plot = new Plot(this);
    connect(plot, SIGNAL(fileCreated(QString,bool)), SLOT(addFile(QString,bool)));
    connect(plot, SIGNAL(fileChanged(QString, bool)), SLOT(updateFile(QString,bool)));

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

    meanAct = new QAction(QString("Вывести среднее"), this);
    meanAct->setIcon(QIcon(":/icons/mean.png"));
    connect(meanAct, SIGNAL(triggered()), plot, SLOT(calculateMean()));

    editColorsAct = new QAction(QString("Изменить цвета графиков"), this);
    editColorsAct->setIcon(QIcon(":/icons/colors.png"));
    connect(editColorsAct, SIGNAL(triggered()), this, SLOT(editColors()));

    QMenu *fileMenu = menuBar()->addMenu(tr("Файл"));
    fileMenu->addAction(addFolderAct);

    QMenu *recordsMenu = menuBar()->addMenu(QString("Записи"));
    recordsMenu->addAction(delFilesAct);
    recordsMenu->addAction(rescanBaseAct);

    QMenu *settingsMenu = menuBar()->addMenu(QString("Настройки"));
    settingsMenu->addAction(editColorsAct);

    QToolBar *toolBar = new QToolBar(this);
    toolBar->setOrientation(Qt::Vertical);
    toolBar->addAction(clearPlotAct);
    toolBar->addAction(savePlotAct);
    toolBar->addAction(copyToClipboardAct);
    toolBar->addAction(printPlotAct);
    toolBar->addAction(switchCursorAct);
    toolBar->addAction(meanAct);

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

    tab->channelsTable->blockSignals(true);
    tab->channelsTable->setRowCount(0);
    tab->channelsTable->blockSignals(false);

    if (!QFileInfo(item->dfd->dfdFileName).exists()
        || !QFileInfo(item->dfd->rawFileName).exists()) {
        QMessageBox::warning(this,"Не могу получить список каналов","Такого файла уже нет");
        return;
    }

    record = item->dfd;
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
