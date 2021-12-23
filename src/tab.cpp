#include "tab.h"

#include <QtWidgets>
#include "mainwindow.h"

#include "logging.h"

#include "headerview.h"
#include "filestable.h"
#include "model.h"
#include "channeltablemodel.h"
#include "sortfiltermodel.h"
#include "filterheaderview.h"
#include "fileformats/filedescriptor.h"
#include "filehandler.h"
#include "stepitemdelegate.h"
#include "channelstable.h"

Tab::Tab(MainWindow *parent) : QSplitter(parent), parent(parent)
{DD;
    setOrientation(Qt::Horizontal);

    model = new Model(this);
    connect(model, &Model::needAddFiles, [=](const QStringList &files){
        parent->addFiles(files);
        fileHandler->trackFiles(files);
    });
    sortModel = new SortFilterModel(this);
    sortModel->setSourceModel(model);

    channelModel = new ChannelTableModel(this);
    connect(channelModel, SIGNAL(modelChanged()), parent, SLOT(updateActions()));
    connect(channelModel, &ChannelTableModel::maybeUpdateChannelProperty,
            [=](int channel, const QString &description, const QString &p, const QString &val){
        if (model->selected().size()>1) {
            if (QMessageBox::question(this,"DeepSea Base",QString("Выделено несколько файлов. Записать %1\n"
                                      "во все эти файлы?").arg(description))==QMessageBox::Yes)
            {
                model->setChannelProperty(channel, p, val);
            }
        }
        model->updateFile(record);
    });

    filesTable = new FilesTable(this);
    filesTable->setModel(sortModel);

    filterHeader = new FilteredHeaderView(Qt::Horizontal, filesTable);
    filesTable->setHeader(filterHeader);
    connect(filterHeader, SIGNAL(filterChanged(QString,int)), sortModel, SLOT(setFilter(QString,int)));

    filesTable->header()->setStretchLastSection(false);
    filesTable->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    filesTable->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    filesTable->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    filesTable->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    filesTable->header()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    filesTable->header()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    filesTable->header()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    filesTable->header()->setSectionResizeMode(7, QHeaderView::ResizeToContents);

    connect(filesTable->selectionModel(),SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(filesSelectionChanged(QItemSelection,QItemSelection)));
    connect(filesTable->selectionModel(),SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(updateChannelsTable(QModelIndex,QModelIndex)));
    filesTable->setItemDelegateForColumn(MODEL_COLUMN_XSTEP, new StepItemDelegate);
    connect(filesTable, &QTreeView::customContextMenuRequested, [=](){
        QMenu menu(filesTable);
        int column = filesTable->currentIndex().column();
        if (!filesTable->selectionModel()->hasSelection()) {
            menu.addAction(parentActions.value("addFolder"));
            menu.addAction(parentActions.value("addFile"));
            menu.exec(QCursor::pos());
        }
        else if (column == MODEL_COLUMN_FILENAME) {
            menu.addAction(parentActions.value("addFolder"));
            menu.addAction(parentActions.value("addFile"));
            menu.addAction(parentActions.value("deleteFiles"));
            menu.addAction(parentActions.value("calculateSpectre"));
            //menu.addAction(calculateSpectreDeepSeaAct);
            menu.addAction(parentActions.value("convert"));
            menu.addAction(parentActions.value("rename"));
            menu.exec(QCursor::pos());
        }
        else if (column == MODEL_COLUMN_LEGEND) {
            //legend
            menu.addAction(copyToLegendAct);
            menu.exec(QCursor::pos());
        }
    });

    connect(model, SIGNAL(modelChanged()), parent, SLOT(updateActions()));

    fileHandler = new FileHandler(this);

    channelsTable = new ChannelsTable(this);
    channelsTable->setContextMenuPolicy(Qt::CustomContextMenu);
    channelsTable->setModel(channelModel);
    connect(channelsTable->selectionModel(),SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(channelsSelectionChanged(QItemSelection,QItemSelection)));
    connect(channelsTable,SIGNAL(yNameChanged(QString)),channelModel,SLOT(setYName(QString)));
    channelsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    channelsTable->horizontalHeader()->setStretchLastSection(false);

    filePathLabel = new QLabel(this);
    filePathLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    filePathLabel->setSizePolicy(QSizePolicy::Expanding, filePathLabel->sizePolicy().verticalPolicy());

    openFolderAct = new QAction("Открыть папку с этой записью", this);
    openFolderAct->setEnabled(false);
    openFolderAct->setIcon(QIcon(":/icons/open24.png"));
    connect(openFolderAct, &QAction::triggered, [=](){
        if (!filePathLabel->text().isEmpty()) {
            QDir dir(filePathLabel->text());
            dir.cdUp();
            QProcess::startDetached("explorer.exe", QStringList(dir.toNativeSeparators(dir.absolutePath())));
        }
    });
    editFileAct = new QAction("Редактировать этот файл в текстовом редакторе", this);
    editFileAct->setEnabled(false);
    editFileAct->setIcon(QIcon(":/icons/edit24.png"));
    connect(editFileAct, &QAction::triggered, [=](){
        QString file = QDir::toNativeSeparators(filePathLabel->text());
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

    plotSelectedChannelsAct = new QAction(QString("Построить выделенные каналы"), this);
    QIcon plotIcon(":/icons/plot24.png");
    plotIcon.addFile(":/icons/plot.png");
    plotSelectedChannelsAct->setIcon(plotIcon);
    connect(plotSelectedChannelsAct, &QAction::triggered, [=](){
        auto toPlot = channelModel->selectedChannels();
        if (!toPlot.isEmpty()) emit needPlotChannels(true, toPlot);
    });
    channelsTable->addAction("plot", plotSelectedChannelsAct);

    plotSelectedChannelsOnRightAct = new QAction(QString("...на правой оси"), this);
    connect(plotSelectedChannelsOnRightAct, &QAction::triggered, [=](){
        auto toPlot = channelModel->selectedChannels();
        if (!toPlot.isEmpty()) emit needPlotChannels(false, toPlot);
    });
    channelsTable->addAction("plotRight", plotSelectedChannelsOnRightAct);

    copyToLegendAct = new QAction("Перенести сюда названия файлов", this);
    connect(copyToLegendAct, &QAction::triggered, model, &Model::copyToLegend);

    QToolBar *channelsToolBar = new QToolBar(this);
    channelsToolBar->setFloatable(false);
    channelsToolBar->setMovable(false);
    channelsToolBar->setIconSize(QSize(24,24));
    channelsToolBar->setContentsMargins(0,0,0,0);
    channelsToolBar->addAction(plotSelectedChannelsAct);
    channelsToolBar->addSeparator();
    channelsToolBar->addAction(editFileAct);
    channelsToolBar->addAction(openFolderAct);
    channelsToolBar->addSeparator();
    channelsToolBar->addWidget(filePathLabel);

    QWidget *channelsWidget = new QWidget(this);
    channelsWidget->setContentsMargins(0,0,0,0);

    QGridLayout *channelsLayout = new QGridLayout;
    channelsLayout->setContentsMargins(0,0,0,0);
    channelsLayout->addWidget(channelsToolBar,0,0,1,3);
    channelsLayout->addWidget(channelsTable,1,0,1,3);
    channelsWidget->setLayout(channelsLayout);

    addWidget(filesTable);
    addWidget(channelsWidget);

    QByteArray upperSplitterState = App->getSetting("upperSplitterState").toByteArray();
    if (!upperSplitterState.isEmpty()) restoreState(upperSplitterState);

    updateActions();
}

void Tab::updateChannelsTable(FileDescriptor *descriptor)
{
    record = descriptor;
    channelModel->setDescriptor(descriptor);
    if (!descriptor) return;

    if (!descriptor->fileExists()) {
        QMessageBox::warning(this,"Не могу получить список каналов","Такого файла уже нет");
        return;
    }

    filePathLabel->setText(descriptor->fileName());

    emit descriptorChanged();
    //updateActions();
}

void Tab::updateActions()
{
    openFolderAct->setEnabled(record != nullptr);
    editFileAct->setEnabled(record != nullptr);
    plotSelectedChannelsAct->setEnabled(channelModel->hasSelection());
    plotSelectedChannelsOnRightAct->setEnabled(channelModel->hasSelection());
    copyToLegendAct->setEnabled(model->selected().size()>0);
}

void Tab::addParentAction(const QString &name, QAction *action)
{
    parentActions.insert(name, action);
}

void Tab::updateChannelsTable(const QModelIndex &current, const QModelIndex &previous)
{DD;
    if (current.isValid()) {
        if (current.model() != sortModel) return;

        if (previous.isValid()) {
            if ((previous.row()==0 && previous.column() != current.column()) ||
                (previous.row() != current.row())) {
                QModelIndex index = sortModel->mapToSource(current);
                updateChannelsTable(model->file(index.row()).get());
            }
        }
        else {
            QModelIndex index = sortModel->mapToSource(current);
            updateChannelsTable(model->file(index.row()).get());
        }
    }
    else
        updateChannelsTable(nullptr);
}

void Tab::filesSelectionChanged(const QItemSelection &newSelection, const QItemSelection &oldSelection)
{DD;
    Q_UNUSED(oldSelection);
    if (newSelection.isEmpty()) filesTable->selectionModel()->setCurrentIndex(QModelIndex(), QItemSelectionModel::NoUpdate);

    QVector<int> indexes;

    const QModelIndexList list = filesTable->selectionModel()->selection().indexes();
    for (const QModelIndex &i: list) indexes << sortModel->mapToSource(i).row();

    std::sort(indexes.begin(), indexes.end());
    indexes.erase(std::unique(indexes.begin(), indexes.end()), indexes.end());

    model->setSelected(indexes);
    if (indexes.isEmpty()) {
        channelModel->clear();
        filePathLabel->clear();
    }
}

void Tab::channelsSelectionChanged(const QItemSelection &newSelection, const QItemSelection &oldSelection)
{DD;
    Q_UNUSED(oldSelection);
    if (newSelection.isEmpty()) channelsTable->selectionModel()->setCurrentIndex(QModelIndex(), QItemSelectionModel::NoUpdate);

    QVector<int> indexes;

    const QModelIndexList list = channelsTable->selectionModel()->selection().indexes();
    for (const QModelIndex &i: list) indexes << i.row();

    std::sort(indexes.begin(), indexes.end());
    indexes.erase(std::unique(indexes.begin(), indexes.end()), indexes.end());

    channelModel->setSelected(indexes);
    //updateActions();
}
