#include "filehandlerdialog.h"

#include <QtWidgets>
#include "filehandler.h"
#include "logging.h"


FileHandlerDialog::FileHandlerDialog(FileHandler *fileHandler, QWidget *parent)
    : QDialog(parent), fileHandler(fileHandler)
{DD;
    setWindowTitle("Отслеживаемые файлы");

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));

    model = new HandlerModel(fileHandler, this);
    tree = new QTreeView(this);
    tree->setModel(model);
    tree->setAlternatingRowColors(true);
    tree->setRootIsDecorated(false);
    tree->resizeColumnToContents(0);
    tree->setSelectionBehavior(QAbstractItemView::SelectRows);
    tree->setSelectionMode(QAbstractItemView::ExtendedSelection);

    removeFileAction = new QAction("Убрать из отслеживаемых", this);
    removeFileAction->setShortcut(Qt::Key_Delete);
    removeFileAction->setIcon(QIcon(":/icons/list-remove.png"));
    removeFileAction->setEnabled(false);
    connect(removeFileAction, &QAction::triggered, this, &FileHandlerDialog::removeTrackedFile);

    connect(tree->selectionModel(), &QItemSelectionModel::selectionChanged, [=](const QItemSelection &selected, const QItemSelection &deselected)
    {
        Q_UNUSED(deselected);
        removeFileAction->setDisabled(selected.isEmpty());
    });

    tree->setContextMenuPolicy(Qt::ActionsContextMenu);
    tree->addAction(removeFileAction);

    auto toolBar = new QToolBar(this);
    toolBar->addAction(removeFileAction);
    toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    QGridLayout *l = new QGridLayout;
    l->addWidget(toolBar, 0,0);
    l->addWidget(tree,1,0);
    l->addWidget(buttonBox,2,0);
    setLayout(l);
    resize(qApp->primaryScreen()->availableSize()/3);
}

void FileHandlerDialog::removeTrackedFile()
{DD;
    auto selection = tree->selectionModel()->selectedRows();
    for (auto b = selection.rbegin(); b!=selection.rend(); ++b) {
        model->untrack((*b).row());
    }
}


HandlerModel::HandlerModel(FileHandler *fileHandler, QObject *parent)
    : QAbstractTableModel(parent), fileHandler(fileHandler)
{DD;

}

int HandlerModel::rowCount(const QModelIndex &parent) const
{DD;
    Q_UNUSED(parent);
    return fileHandler->count();
}

int HandlerModel::columnCount(const QModelIndex &parent) const
{DD;
    Q_UNUSED(parent);
    return 2;
}

QVariant HandlerModel::data(const QModelIndex &index, int role) const
{DD;
    if (index.isValid()) {
        auto item = fileHandler->item(index.row());
        switch (role) {
            case Qt::DisplayRole: {
                switch (index.column()) {
                    case 0: return item.path;
                    case 1: {
                        switch (item.type) {
                            case FileHandler::File: return "Файл";
                            case FileHandler::Folder: return "Папка";
                            case FileHandler::FolderWithSubfolders: return "Папка с вложенными папками";
                        }
                        break;
                    }
                }
                break;
            }
            case Qt::ForegroundRole: {
                if (index.column()==0) {
                    if (!item.good) return QBrush(Qt::red);
                }
                break;
            }
            case Qt::ToolTipRole: {
                if (index.column()==0) {
                    if (!item.good) return "Такого файла уже нет";
                }
                break;
            }
        }
    }
    return QVariant();
}

QVariant HandlerModel::headerData(int section, Qt::Orientation orientation, int role) const
{DD;
    if (orientation == Qt::Horizontal && role==Qt::DisplayRole) {
        switch (section) {
            case 0: return "Путь";
            case 1: return "Тип";
        }
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}

void HandlerModel::untrack(int index)
{DD;
    beginResetModel();
    fileHandler->untrack(fileHandler->item(index));
    endResetModel();
}
