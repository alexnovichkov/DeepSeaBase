#include "mainwindow.h"

#include <QtWidgets>
//#include <QtGui>

#include "dfdfiledescriptor.h"

const int treeColumnCount = 12;
const QString dateTimeFormat = "dd MMMM yyyy, hh:mm:ss";

class TreeWidgetItem : public QTreeWidgetItem
{
public:
    TreeWidgetItem(QTreeWidget *tree) : QTreeWidgetItem(tree), dfd(0)  {}
    TreeWidgetItem(QTreeWidget * parent, const QStringList & strings)
        : QTreeWidgetItem (parent,strings), dfd(0)  {}
    TreeWidgetItem(DfdFileDescriptor *dfd, const QStringList & strings)
        : QTreeWidgetItem (strings) , dfd(dfd)
    {

    }

    bool operator< (const QTreeWidgetItem &other) const
    {
        int sortCol = treeWidget()->sortColumn();
        QString s = text(sortCol);
        switch (sortCol) {
            case 0:
                return s.toInt() < other.text(sortCol).toInt();
            case 2:
                return QDateTime::fromString(s, dateTimeFormat)
                        < QDateTime::fromString(other.text(sortCol), dateTimeFormat);
            case 3:
            case 6:
                return s.toDouble() < other.text(sortCol).toDouble();
            case 7:
                return s.toInt() < other.text(sortCol).toInt();
            case 8:
                return s.toDouble() < other.text(sortCol).toDouble();
            case 9:
                return s.toInt() < other.text(sortCol).toInt();
            default:
                break;
        }
        return QTreeWidgetItem::operator <(other);
    }
private:
    DfdFileDescriptor *dfd;
};

QString dataTypeDescription(int type)
{
    switch (type) {
        case 0: return QString("Данные"); break;
        case 1: return QString("Данные1"); break;
        case 2: return QString("Данные2"); break;
        case 3: return QString("Данные3"); break;
        case 4: return QString("Данные4"); break;
        case 5: return QString("Данные5"); break;
        case 6: return QString("Данные6"); break;
        case 7: return QString("Данные7"); break;
        case 8: return QString("Данные8"); break;
        case 9: return QString("Данные9"); break;
        case 10: return QString("Данные10"); break;
        case 11: return QString("Данные11"); break;
        case 12: return QString("Данные12"); break;
        case 13: return QString("Данные13"); break;
        case 14: return QString("Данные14"); break;
        case 15: return QString("Данные15"); break;
        case 16: return QString("Огибающая"); break;
        case 17: return QString("МО"); break;
        case 18: return QString("СКЗ"); break;
        case 19: return QString("Ассиметрия"); break;
        case 20: return QString("Эксцесс"); break;
        case 21: return QString("Фаза (Гильб.)"); break;
        case 32: return QString("Корреляция"); break;
        case 33: return QString("X-Корр."); break;
        case 64: return QString("Гистограмма"); break;
        case 65: return QString("ЭФР"); break;
        case 66: return QString("Гистограмма (%)"); break;
        case 67: return QString("Плотн. вероятн."); break;
        case 128: return QString("Спектр"); break;
        case 129: return QString("ПСМ"); break;
        case 130: return QString("Спектр СКЗ"); break;
        case 144: return QString("X-Спектр"); break;
        case 145: return QString("X-Фаза"); break;
        case 146: return QString("Когерентность"); break;
        case 147: return QString("ПФ"); break;
        case 148: return QString("X-Спектр Re"); break;
        case 149: return QString("X-Спектр Im"); break;
        case 150: return QString("ПФ Re"); break;
        case 151: return QString("ПФ Im"); break;
        case 152: return QString("ДН X-Спектр"); break;
        case 153: return QString("Кепстр"); break;
        case 154: return QString("ДН ПФ"); break;
        case 155: return QString("Спектр огиб."); break;
        case 156: return QString("Окт.спектр"); break;
        case 157: return QString("Тр.окт.спектр"); break;
        default: return "";
    }
    return "";
}

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
    : QMainWindow(parent)
{
    setWindowTitle(tr("DeepSea Database"));
    setAcceptDrops(true);

    addFolderAct = new QAction(qApp->style()->standardIcon(QStyle::SP_DialogOpenButton), tr("Add folder"),this);
    addFolderAct->setShortcut(tr("Ctrl+O"));
    connect(addFolderAct, SIGNAL(triggered()), SLOT(addFolder()));

    QMenu *fileMenu = menuBar()->addMenu(tr("File"));
    fileMenu->addAction(addFolderAct);


    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

//    QList<QByteArray> b = QTextCodec::availableCodecs();
//    qSort(b);
//    qDebug()<<b;

    tree = new QTreeWidget(this);
    tree->setRootIsDecorated(false);
    tree->setContextMenuPolicy(Qt::ActionsContextMenu);
    tree->addAction(addFolderAct);
    tree->setHeaderLabels(QStringList()
                          << QString("№")
                          << QString("Файл")
                          << QString("Дата")
                          << QString("Тип")
                          << QString("Размер")
                          << QString("Ось Х")
                          << QString("Шаг")
                          << QString("Каналы")
                          << QString("Смещение")
                          << QString("Блок")
                          << QString("Создан")
                          << QString("Формат"));
    tree->header()->setStretchLastSection(false);
    tree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    tree->setSortingEnabled(true);

    QVBoxLayout *centralLayout = new QVBoxLayout;
    centralLayout->addWidget(tree);
    centralWidget->setLayout(centralLayout);
}

MainWindow::~MainWindow()
{
    qDeleteAll(records);
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

    addFiles(filesToAdd, directory);
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

void MainWindow::addFiles(const QStringList &files, const QString &folder)
{
    QStringList descrKeys;
    QList<QTreeWidgetItem *> items;
    QList<DfdFileDescriptor *> dfds;
    int pos = tree->topLevelItemCount();
    foreach (const QString &file, files) {
        DfdFileDescriptor *dfd = new DfdFileDescriptor(file);
        dfd->read();
        descrKeys.append(dfd->userComments.keys());
        dfds << dfd;
        QTreeWidgetItem *item = new TreeWidgetItem(dfd, QStringList()
                                                   << QString::number(++pos)
                                                    << QFileInfo(dfd->dfdFileName).completeBaseName()
                                                    << QDateTime(dfd->Date,dfd->Time).toString(dateTimeFormat)
                                                    << dataTypeDescription(dfd->DataType)
                                                    << QString::number(dfd->NumInd * dfd->XStep)
                                                    << dfd->XName
                                                    << QString::number(dfd->XStep)
                                                    << QString::number(dfd->NumChans)
                                                    << QString::number(dfd->XBegin)
                                                    << QString::number(dfd->BlockSize)
                                                    << dfd->CreatedBy
                                                    << dfd->DescriptionFormat);
        items << item;
    }
    descrKeys.removeDuplicates();
    tree->setColumnCount(treeColumnCount + descrKeys.size());
    for (int i=0; i<descrKeys.size(); ++i) {
        tree->headerItem()->setText(i+treeColumnCount, descrKeys.at(i));
    }
    for (int i=0; i<items.size(); ++i) {
        DfdFileDescriptor *dfd = dfds.at(i);
        for (int key=0; key<descrKeys.size(); ++key) {
            QString title = descrKeys.at(key);
            if (dfd->userComments.contains(title))
                items.at(i)->setText(key + treeColumnCount, dfd->userComments.value(title));
        }
    }
    tree->addTopLevelItems(items);
    records.append(dfds);
    tree->sortItems(0,Qt::AscendingOrder);
}
