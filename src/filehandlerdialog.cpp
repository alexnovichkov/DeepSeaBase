#include "filehandlerdialog.h"

#include <QtWidgets>
#include "filehandler.h"



FileHandlerDialog::FileHandlerDialog(FileHandler *fileHandler, QWidget *parent)
    : QDialog(parent), fileHandler(fileHandler)
{
    setWindowTitle("Отслеживаемые файлы");

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));

    model = new HandlerModel(fileHandler, this);
    tree = new QTreeView(this);
    tree->setModel(model);
    tree->setAlternatingRowColors(true);
    tree->setRootIsDecorated(false);
    tree->resizeColumnToContents(0);

    QGridLayout *l = new QGridLayout;
    l->addWidget(tree,0,0);
    l->addWidget(buttonBox,1,0);
    setLayout(l);
    resize(qApp->primaryScreen()->availableSize()/3);
}


HandlerModel::HandlerModel(FileHandler *fileHandler, QObject *parent)
    : QAbstractTableModel(parent), fileHandler(fileHandler)
{

}

int HandlerModel::rowCount(const QModelIndex &parent) const
{
    return fileHandler->count();
}

int HandlerModel::columnCount(const QModelIndex &parent) const
{
    return 2;
}

QVariant HandlerModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid()) {
        if (role==Qt::DisplayRole) {
            auto item = fileHandler->item(index.row());
            switch (index.column()) {
                case 0: return item.first;
                case 1: {
                    switch (item.second) {
                        case FileHandler::File: return "Файл";
                        case FileHandler::Folder: return "Папка";
                        case FileHandler::FolderWithSubfolders: return "Папка с вложенными папками";
                    }
                }
            }
        }
    }
    return QVariant();
}

QVariant HandlerModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role==Qt::DisplayRole) {
        switch (section) {
            case 0: return "Путь";
            case 1: return "Тип";
        }
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}
