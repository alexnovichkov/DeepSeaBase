#include "mainwindow.h"

#include <QtWidgets>
#include "qcustomplot.h"

#include "convertdialog.h"
#include "sortabletreewidgetitem.h"

const int treeColumnCount = 8;
const int channelsTableColumnsCount = 6;


const float scrollBarRange = 1000.0;




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
    : QMainWindow(parent), record(0), freeGraph(0)
{
    setWindowTitle(tr("DeepSea Database"));
    setAcceptDrops(true);

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

    QMenu *fileMenu = menuBar()->addMenu(tr("Файл"));
    fileMenu->addAction(addFolderAct);

    QMenu *recordsMenu = menuBar()->addMenu(QString("Записи"));
    recordsMenu->addAction(delFilesAct);


    tree = new QTreeWidget(this);
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
                          << QString("Каналы"));
    tree->header()->setStretchLastSection(false);
    tree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    connect(tree, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),SLOT(currentRecordChanged(QTreeWidgetItem*,QTreeWidgetItem*)));
    tree->sortByColumn(0, Qt::AscendingOrder);
//    tree->setSortingEnabled(true);

    QMap<int, SortableTreeWidgetItem::DataType> typeMap;
    typeMap.insert(0, SortableTreeWidgetItem::DataTypeInteger);
    typeMap.insert(7, SortableTreeWidgetItem::DataTypeInteger);
    typeMap.insert(2, SortableTreeWidgetItem::DataTypeDate);
    typeMap.insert(4, SortableTreeWidgetItem::DataTypeFloat);
    typeMap.insert(6, SortableTreeWidgetItem::DataTypeFloat);
    SortableTreeWidgetItem::setTypeMap(typeMap);

    channelsTable = new QTableWidget(0,6,this);
    channelsTable->setHorizontalHeaderLabels(QStringList()
                                             << QString("Канал") //ChanAddress
                                             << QString("Имя") //ChanName
                                             << QString("Вход") //InputType
                                             << QString("Ед.изм.") //YName
                                             << QString("Датчик") //SensName
                                             << QString("Описание") //ChanDscr
                                             );
    channelsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    channelsTable->horizontalHeader()->setStretchLastSection(false);
    connect(channelsTable, SIGNAL(currentCellChanged(int,int,int,int)),this,SLOT(maybePlotChannel(int,int,int,int)));
    connect(channelsTable, SIGNAL(itemChanged(QTableWidgetItem*)), SLOT(maybePlotChannel(QTableWidgetItem*)));

    filePathLabel = new QLabel(this);
    filePathLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);

    plot = new QCustomPlot(this);
    plot->setMinimumWidth(400);
    plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom |
                          QCP::iSelectLegend | QCP::iSelectPlottables);
    plot->axisRect()->setupFullAxesBox(true);
    plot->legend->setVisible(true);
    plot->legend->setSelectableParts(QCPLegend::spItems);

    hScrollBar = new QScrollBar(Qt::Horizontal, this);
    vScrollBar = new QScrollBar(Qt::Vertical, this);
    hScrollBar->setRange(0, scrollBarRange);
    vScrollBar->setRange(0, scrollBarRange);

//    connect(hScrollBar, &QScrollBar::valueChanged, [=](int value){
//        const QCPRange r = plot->xAxis->range();
//        if (qAbs(r.center()-value/scrollBarRange) > 0.01) // if user is dragging plot, we don't want to replot twice
//        {
//            plot->xAxis->setRange(value/scrollBarRange, r.size(), Qt::AlignCenter);
//            plot->replot();
//        }
//    });
//    connect(vScrollBar, &QScrollBar::valueChanged, [=](int value){
//        const QCPRange r = plot->yAxis->range();
//        if (qAbs(r.center()+value/scrollBarRange) > 0.01) // if user is dragging plot, we don't want to replot twice
//        {
//            plot->yAxis->setRange(-value/scrollBarRange, r.size(), Qt::AlignCenter);
//            plot->replot();
//        }
//    });
    connect(plot->xAxis, SIGNAL(rangeChanged(QCPRange)), SLOT(xRangeChanged(QCPRange)));
    connect(plot->yAxis, SIGNAL(rangeChanged(QCPRange)), SLOT(yRangeChanged(QCPRange)));

    connect(plot, SIGNAL(plottableClick(QCPAbstractPlottable*,QMouseEvent*)), this, SLOT(graphClicked(QCPAbstractPlottable*)));
    connect(plot, SIGNAL(selectionChangedByUser()), this, SLOT(plotSelectionChanged()));

    plot->xAxis->setMaxRange(QCPRange(0, 0));
    plot->yAxis->setMaxRange(QCPRange(0, 0));

    QWidget *tablesWidget = new QWidget(this);
    QGridLayout *tablesLayout = new QGridLayout;
    tablesLayout->addWidget(tree,0,0,2,1);
    tablesLayout->addWidget(channelsTable,1,1);
    tablesLayout->addWidget(filePathLabel,0,1);
    tablesWidget->setLayout(tablesLayout);

    QWidget *plotsWidget = new QWidget(this);
    QGridLayout *plotsLayout = new QGridLayout;
    plotsLayout->addWidget(plot,0,0);
    plotsLayout->addWidget(vScrollBar,0,1);
    plotsLayout->addWidget(hScrollBar,1,0);
    plotsWidget->setLayout(plotsLayout);

    splitter = new QSplitter(Qt::Vertical, this);
    splitter->addWidget(tablesWidget);
    splitter->addWidget(plotsWidget);

    QByteArray mainSplitterState = getSetting("mainSplitterState").toByteArray();
    if (!mainSplitterState.isEmpty())
        splitter->restoreState(mainSplitterState);

    setCentralWidget(splitter);

    alreadyAddedFiles = getSetting("alreadyAdded").toStringList();
    if (!alreadyAddedFiles.isEmpty()) addExistingFiles();
}

MainWindow::~MainWindow()
{
    setSetting("mainSplitterState",splitter->saveState());
    setSetting("alreadyAdded", alreadyAddedFiles);
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
        if (!alreadyAddedFiles.contains(file))
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
    addFiles(alreadyAddedFiles, false);
}

void MainWindow::deleteFiles()
{
    int count = tree->topLevelItemCount();
    bool taken = false;
    for (int i=count-1; i>=0; --i) {
        if (tree->topLevelItem(i)->isSelected()) {
            delete tree->takeTopLevelItem(i);
            alreadyAddedFiles.removeAt(i);
            taken = true;
        }
    }
    if (taken) {
        channelsTable->setRowCount(0);
        //channelsTable->clear();
        plot->clearGraphs();
    }
}

void MainWindow::currentRecordChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{//qDebug()<<Q_FUNC_INFO;
    Q_UNUSED(previous)
    if (!current /*|| current==previous*/) return;

    SortableTreeWidgetItem *item = dynamic_cast<SortableTreeWidgetItem *>(current);

    record = item?item->dfd:0;
    filePathLabel->setText(record->dfdFileName);

    int chanCount = record->channels.size();
    if (chanCount == 0) return;

    QStringList headers = record->channels[0]->getHeaders();

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
}

QColor getColor(int index)
{
    //index--;
    static uint colors[16]={
        0x00b40000,
        0x00000080,
        0x00008080,
        0x00803f00,
        0x00ff8000,
        0x000000ff,
        0x00808000,
        0x0000ffff,
        0x00f0f0c0, //240, 240, 192
        0x00800080,
        0x00ff00ff,
        0x00007800, //0, 120, 0
        0x00000000,
        0x00ff8080,
        0x008080ff,
        0x00a0a0a4
    };
    if (index<0 || index>15) return QColor(QRgb(0x00808080));
    return QColor(QRgb(colors[index]));
}

void MainWindow::plotChannel(Channel *channel, const GraphIndex &idx, bool addToFixed)
{
    bool plotOnFirstYAxis = false;
    bool plotOnSecondYAxis = false;


    if (plot->graphCount()==0) {
        plotOnFirstYAxis = true;
    }
    // есть один временный график
    else if (graphs.isEmpty() && !addToFixed) {
        plotOnFirstYAxis = true;
    }
    // есть постоянные графики
    else {
        // тип графика не совпадает
        /** TODO: заменить на анализ типа графика */
        if (channel->parent->XName != plot->xAxis->label()) return;

        if (channel->YName == plot->yAxis->label())
            plotOnFirstYAxis = true;
        else
            // trying to plot on second yaxis
            if (plot->yAxis2->graphs().isEmpty() || channel->YName == plot->yAxis2->label()) {
                // plotting on second yaxis
                plotOnSecondYAxis = true;
            }
    }

    if (!plotOnFirstYAxis && !plotOnSecondYAxis) return;


    if (addToFixed) {
        QCPGraph *graph = plot->addGraph(plot->xAxis, plotOnFirstYAxis ? plot->yAxis : plot->yAxis2);
        QColor nextColor = getColor(graphs.size());
        graph->setPen(nextColor);
        graph->setData(channel->data, true);
        graph->setName(channel->legendName);
        graphs.insert(idx, graph);
    }
    else {
        if (!freeGraph) {
            freeGraph = plot->addGraph(plot->xAxis, plotOnFirstYAxis ? plot->yAxis : plot->yAxis2);
            QColor nextColor = getColor(200);
            freeGraph->setPen(nextColor);
        }
        freeGraph->setName(channel->legendName);
        freeGraph->setData(channel->data, true);
    }

    plot->xAxis->setMaxRange(QCPRange(channel->xMin, channel->xMax));
    double xmin = channel->xMin;
    double xmax = channel->xMaxInitial;
    if (plot->graphCount()>1) {
        xmin = qMin(xmin, plot->xAxis->range().lower);
        xmax = qMax(xmax, plot->xAxis->range().upper);
    }

    plot->xAxis->setRange(xmin, xmax);
    plot->xAxis->setLabel(channel->parent->XName);

    QCPAxis *ax = 0;
    if (plotOnFirstYAxis) ax = plot->yAxis;
    if (plotOnSecondYAxis) ax = plot->yAxis2;
    if (ax) {
        ax->setMaxRange(QCPRange(channel->yMin, channel->yMax));
        double ymin = channel->yMinInitial;
        double ymax = channel->yMaxInitial;
        if (plot->graphCount()>1) {
            ymin = qMin(ymin, ax->range().lower);
            ymax = qMax(ymax, ax->range().upper);
        }
        ax->setRange(ymin, ymax);
        ax->setLabel(channel->YName);
    }

    plot->replot();
}

void MainWindow::maybePlotChannel(int currentRow, int currentColumn, int previousRow, int previousColumn)
{
    Q_UNUSED(previousColumn);
    if (currentRow<0 || currentColumn<0 || currentRow==previousRow) return;

    Channel *ch = record->channels.at(currentRow);
    GraphIndex index = GraphIndex(record->dfdFileName, ch->channelIndex);

    if (!ch->data)
        ch->populateData();

    plotChannel(ch, index, false);
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
    if (!item) return;
    if (item->column()!=0) return;

    Qt::CheckState state = item->checkState();
    Channel *ch = record->channels[item->row()];
    ch->checkState = state;

    QFont oldFont = channelsTable->font();
    QFont boldFont = oldFont;
    boldFont.setBold(true);

    if (state == Qt::Checked && !ch->data)
        ch->populateData();

    GraphIndex idx = GraphIndex(record->dfdFileName, ch->channelIndex);
    if (state == Qt::Checked) {
        if (!graphs.contains(idx)) {
            //add graph
            plotChannel(ch, idx, true);
        }
        item->setFont(boldFont);
    }
    else if (state == Qt::Unchecked) {
        if (graphs.contains(idx)) {
            //remove graph
            plot->removeGraph(graphs.value(idx));
            graphs.remove(idx);
            plot->replot();
            item->setFont(channelsTable->font());
        }
    }
    tree->currentItem()->setFont(1,allUnchecked(record->channels)?oldFont:boldFont);
    //tree->topLevelItem(records.indexOf(record))
}

void MainWindow::xRangeChanged(const QCPRange &range)
{
    double Min = plot->xAxis->maxRange().lower;
    double Range = plot->xAxis->maxRange().size();
    int pageStep = qRound(range.size()/Range*scrollBarRange);
    int maxVal = qRound(scrollBarRange - pageStep);
    hScrollBar->setPageStep(pageStep);
    hScrollBar->setMaximum(maxVal);
    double dist = qAbs(range.lower - Min);
    int barVal = qRound(dist/Range*scrollBarRange);
    hScrollBar->setValue(barVal);
}

void MainWindow::yRangeChanged(const QCPRange &range)
{
    double Min = plot->yAxis->maxRange().lower;
    double Range = plot->yAxis->maxRange().size();
    int pageStep = qRound(range.size()/Range*scrollBarRange);
    vScrollBar->setPageStep(pageStep);
    vScrollBar->setMaximum(scrollBarRange - pageStep);
    double dist = qAbs(range.lower - Min);
    int barVal = qRound(dist/Range*scrollBarRange);
    vScrollBar->setValue(barVal);
}

void MainWindow::graphClicked(QCPAbstractPlottable *plottable)
{
    Q_UNUSED(plottable);
    //ui->statusBar->showMessage(QString("Clicked on graph '%1'.").arg(plottable->name()), 1000);
}

void MainWindow::plotSelectionChanged()
{
    // synchronize selection of graphs with selection of corresponding legend items:
    for (int i=0; i<plot->graphCount(); ++i)
    {
        QCPGraph *graph = plot->graph(i);
        QCPPlottableLegendItem *item = plot->legend->itemWithPlottable(graph);
        if (item->selected() || graph->selected())
        {
            item->setSelected(true);
            graph->setSelected(true);
        }
    }
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
            if (alreadyAddedFiles.contains(newFiles.at(i))) newFiles.removeAt(i);
        }

        addFiles(newFiles, true);
    }
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
    QList<QTreeWidgetItem *> items;
    QList<DfdFileDescriptor *> dfds;
    int pos = tree->topLevelItemCount();
    foreach (const QString &file, files) {
        DfdFileDescriptor *dfd = new DfdFileDescriptor(file);
        dfd->read();
        dfds << dfd;
        if (addToDatabase) alreadyAddedFiles << file;

        QTreeWidgetItem *item =
                new SortableTreeWidgetItem(dfd, QStringList()
                                   << QString::number(++pos) // QString("№") 0
                                   << QFileInfo(dfd->dfdFileName).completeBaseName() //QString("Файл") 1
                                   << QDateTime(dfd->Date,dfd->Time).toString(dateTimeFormat) // QString("Дата") 2
                                   << dataTypeDescription(dfd->DataType) // QString("Тип") 3
                                   << QString::number(dfd->NumInd * dfd->XStep) // QString("Размер") 4
                                   << dfd->XName // QString("Ось Х") 5
                                   << QString::number(dfd->XStep) // QString("Шаг") 6
                                   << QString::number(dfd->NumChans));  // QString("Каналы")); 7
        items << item;
    }
    tree->setSortingEnabled(false);
    tree->addTopLevelItems(items);
//    tree->sortItems(0,Qt::AscendingOrder);
    tree->setSortingEnabled(true);
}
