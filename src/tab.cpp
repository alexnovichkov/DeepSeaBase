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
    connect(channelModel,SIGNAL(maybePlot(int)), parent, SLOT(plotChannel(int)));
    connect(channelModel,SIGNAL(deleteCurve(int)), parent, SLOT(deleteCurve(int)));

    filesTable = new FilesTable(this);
    //connect(tab->filesTable, &FilesTable::addFiles, this, &MainWindow::addFiles);
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
    connect(model, SIGNAL(modelChanged()), parent, SLOT(updateActions()));

    fileHandler = new FileHandler(this);

    channelsTable = new ChannelsTable(this);
    channelsTable->setContextMenuPolicy(Qt::CustomContextMenu);
    channelsTable->setModel(channelModel);
    connect(channelsTable->selectionModel(),SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(channelsSelectionChanged(QItemSelection,QItemSelection)));
//    auto tableHeader = new HeaderView(Qt::Horizontal, channelsTable);
//    channelsTable->setHorizontalHeader(tableHeader);
//    tableHeader->setCheckable(0,true);

    channelsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    channelsTable->horizontalHeader()->setStretchLastSection(false);

    filePathLabel = new QLabel(this);
    filePathLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    filePathLabel->setSizePolicy(QSizePolicy::Expanding, filePathLabel->sizePolicy().verticalPolicy());

    QAction *openFolderAct = new QAction("Открыть папку с этой записью", this);
    openFolderAct->setIcon(QIcon(":/icons/open.png"));
    connect(openFolderAct, &QAction::triggered, [=](){
        if (!filePathLabel->text().isEmpty()) {
            QDir dir(filePathLabel->text());
            dir.cdUp();
            QProcess::startDetached("explorer.exe", QStringList(dir.toNativeSeparators(dir.absolutePath())));
        }
    });
    QAction *editFileAct = new QAction("Редактировать этот файл в текстовом редакторе", this);
    editFileAct->setIcon(QIcon(":/icons/edit.png"));
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
    QToolBar *channelsToolBar = new QToolBar(this);
    channelsToolBar->setFloatable(false);
    channelsToolBar->setMovable(false);
    channelsToolBar->setIconSize(QSize(16,16));
    channelsToolBar->setContentsMargins(0,0,0,0);
    channelsToolBar->addWidget(filePathLabel);
    channelsToolBar->addAction(editFileAct);
    channelsToolBar->addAction(openFolderAct);
    QWidget *channelsWidget = new QWidget(this);
    channelsWidget->setContentsMargins(0,0,0,0);
    QGridLayout *channelsLayout = new QGridLayout;
    channelsLayout->addWidget(channelsToolBar,0,0,1,3);
    channelsLayout->addWidget(channelsTable,1,0,1,3);
    channelsWidget->setLayout(channelsLayout);

    addWidget(filesTable);
    addWidget(channelsWidget);

    QByteArray upperSplitterState = App->getSetting("upperSplitterState").toByteArray();
    if (!upperSplitterState.isEmpty()) restoreState(upperSplitterState);
}

void Tab::updateChannelsTable(const QModelIndex &current, const QModelIndex &previous)
{DD;
    if (!current.isValid()) return;
    if (current.model() != sortModel) return;

    if (previous.isValid()) {
        if ((previous.row()==0 && previous.column() != current.column()) ||
            (previous.row() != current.row())) {
            QModelIndex index = sortModel->mapToSource(current);
            parent->updateChannelsTable(model->file(index.row()).get());
        }
    }
    else {
        QModelIndex index = sortModel->mapToSource(current);
        parent->updateChannelsTable(model->file(index.row()).get());
    }
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
}
