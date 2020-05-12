#include "uffconverterdialog.h"
#include <QtWidgets>
//#include "fileformats/ufffile.h"
//#include "fileformats/dfdfiledescriptor.h"
#include "checkableheaderview.h"
#include "logging.h"
#include "algorithms.h"
#include "mainwindow.h"
#include "fileformats/formatfactory.h"

void ConverterDialog::addFile(const QString &fileName)
{
    QTreeWidgetItem *item = new QTreeWidgetItem(tree);
    item->setText(0, fileName);
    item->setCheckState(0, Qt::Checked);
}

ConverterDialog::ConverterDialog(QList<FileDescriptor *> dataBase, QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Конвертер файлов");
    thread = 0;

    progress = new QProgressBar(this);
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(start()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(stop()));
//    buttonBox->buttons().first()->setDisabled(true);

    convertor = new FileConvertor();

    button = new QPushButton("Добавить файлы", this);
    connect(button, &QPushButton::clicked, [=](){
        folder = MainWindow::getSetting("uffFolder").toString();
        QStringList filters = FormatFactory::allFilters();
        QStringList files = QFileDialog::getOpenFileNames(this, "Выберите файлы",folder,
                                                          filters.join(";;"));

        if (!files.isEmpty())
            MainWindow::setSetting("uffFolder", QFileInfo(files.first()).canonicalPath());

        foreach (const QString &f, files) {
            addFile(f);
        }
        tree->resizeColumnToContents(0);
        buttonBox->buttons().first()->setDisabled(files.isEmpty());
    });

    tree = new QTreeWidget(this);
    tree->setAlternatingRowColors(true);
    tree->setColumnCount(2);
    tree->setHeaderLabels(QStringList()<<"Файл"<<"Конвертирован");

    CheckableHeaderView *tableHeader = new CheckableHeaderView(Qt::Horizontal, tree);

    tree->setHeader(tableHeader);
    tableHeader->setCheckable(0,true);
    connect(tableHeader, &CheckableHeaderView::toggled, [this](int column, Qt::CheckState state)
    {
        if (column<0 || column >= tree->columnCount()) return;

        if (state == Qt::PartiallyChecked) return;
        for (int i=0; i<tree->topLevelItemCount(); ++i)
            tree->topLevelItem(i)->setCheckState(column, state);
    });
    connect(tree, &QTreeWidget::itemChanged, [=](QTreeWidgetItem *item,int col)
    {
        Q_UNUSED(item)
        int checked = 0;
        for (int i=0; i<tree->topLevelItemCount(); ++i) {
            if (tree->topLevelItem(i)->checkState(col)==Qt::Checked) checked++;
        }

        if (checked==0)
            tableHeader->setCheckState(col, Qt::Unchecked);
        else if (checked == tree->topLevelItemCount())
            tableHeader->setCheckState(col, Qt::Checked);
        else
            tableHeader->setCheckState(col, Qt::PartiallyChecked);
    });
    tableHeader->setCheckState(1,Qt::Unchecked);


    textEdit = new QPlainTextEdit(this);


    formatBox = new QComboBox(this);
    formatBox->addItems(FormatFactory::allFilters());

    QSplitter *splitter = new QSplitter(Qt::Vertical, this);
    QWidget *first = new QWidget(this);

    QGridLayout *grid = new QGridLayout;
    grid->addWidget(new QLabel("Сохранять как", this),0,0);
    grid->addWidget(formatBox,0,1);
    grid->addWidget(button,0,2);
    grid->addWidget(tree, 1,0,1,3);
    grid->addWidget(progress,2,0,1,3);
    first->setLayout(grid);

    QWidget *second = new QWidget(this);
    QGridLayout *grid1 = new QGridLayout;
    grid1->addWidget(textEdit,0,0);
    grid1->addWidget(buttonBox,1,0);
    second->setLayout(grid1);
    splitter->addWidget(first);
    splitter->addWidget(second);

    QVBoxLayout *l = new QVBoxLayout;
    l->addWidget(splitter);
    l->setMargin(0);
    setLayout(l);

    resize(500,400);

    foreach(FileDescriptor *fd, dataBase)
        addFile(fd->fileName());
    tree->resizeColumnToContents(0);
}

ConverterDialog::~ConverterDialog()
{
    if (convertor) {
        convertor->deleteLater();
    }
    if (thread) {
        thread->quit();
        thread->wait();
        thread->deleteLater();
    }
}

void ConverterDialog::accept()
{
    convertedFiles = convertor->getNewFiles();
    QDialog::accept();
}

void ConverterDialog::reject()
{
    stop();
    QDialog::reject();
}

void ConverterDialog::updateProgressIndicator()
{
    progress->setValue(progress->value()+1);
}

void ConverterDialog::start()
{
    buttonBox->buttons().first()->setDisabled(true);
    if (!thread) thread = new QThread;
    convertor->moveToThread(thread);
    QStringList toConvert;
    for (int i=0; i<tree->topLevelItemCount(); ++i)
        if (tree->topLevelItem(i)->checkState(0)==Qt::Checked) toConvert << tree->topLevelItem(i)->text(0);
    convertor->setFilesToConvert(toConvert);

    convertor->setDestinationFormat(formatBox->currentText());

    if (toConvert.isEmpty()) {
        textEdit->appendHtml("Ни одного файла не выделено.");
        return;
    }

    progress->setRange(0, toConvert.size());
    progress->setValue(0);

    connect(thread, SIGNAL(started()), convertor, SLOT(convert()));
    connect(convertor, SIGNAL(finished()), thread, SLOT(quit()));
    connect(convertor, SIGNAL(finished()), this, SLOT(finalize()));
    connect(convertor, SIGNAL(tick()), SLOT(updateProgressIndicator()));
    //connect(convertor, SIGNAL(tick(QString)), SLOT(updateProgressIndicator(QString)));
    connect(convertor, SIGNAL(message(QString)), textEdit, SLOT(appendHtml(QString)));

    thread->start();
}

void ConverterDialog::stop()
{
    if (thread)
        thread->requestInterruption();
    QDialog::accept();
}

void ConverterDialog::finalize()
{
//    buttonBox->buttons().first()->setDisabled(false);
//    if (convertor) {
//        convertor->deleteLater();
//        //convertor=0;
//    }
//    if (thread) {
//        thread->quit();
//        thread->wait();
//        thread->deleteLater();
//       // thread = 0;
    //    }
}






FileConvertor::FileConvertor(QObject *parent) : QObject(parent)
{

}

QStringList FileConvertor::getUffFiles() const
{
    QStringList result;
    foreach (const QFileInfo &fi, uffFiles)
        result << fi.canonicalFilePath();

    return result;
}

bool FileConvertor::convert()
{
    //TODO: проверить конвертацию файлов uff
    if (QThread::currentThread()->isInterruptionRequested()) return false;

    //Converting
    foreach(const QString &fileToConvert, filesToConvert) {
        if (QThread::currentThread()->isInterruptionRequested()) return false;

        emit message("<font color=green>Конвертируем файл "+fileToConvert+"</font>");

        //проверяем тип файла source and dest
        QFileInfo fi(fileToConvert);
        QString destSuffix = destinationFormat.right(4).toLower();
        destSuffix.chop(1);
        if (fi.suffix().toLower() == destSuffix) {
            emit message("<font color=blue>Пропускаем файл - типы совпадают.</font>");
            emit tick();
            continue;
        }

        QString sourceFileName = fileToConvert;
        QString destFileName = createUniqueFileName("", fileToConvert, "", destSuffix, false);

        FileDescriptor *sourceFile = FormatFactory::createDescriptor(sourceFileName);

        if (sourceFile) {
            sourceFile->read();
//            if (sourceFile->type()==Descriptor::Unknown) {
//                emit message("&nbsp;&nbsp;&nbsp;Файл неизвестного типа, пропускаю.");
//                delete sourceFile;
//                emit tick();
//                continue;
//            }

            emit message(QString("&nbsp;&nbsp;&nbsp;В файле %1 каналов").arg(sourceFile->channelsCount()));

            emit message("&nbsp;&nbsp;&nbsp;Сохраняю файл");
            FileDescriptor *destFile = FormatFactory::createDescriptor(*sourceFile, destFileName);

            if (destFile) {
                newFiles.append(destFileName);
                emit message("&nbsp;&nbsp;&nbsp;Готово.");
            }
            delete destFile;
        }
        else {
            emit message("&nbsp;&nbsp;&nbsp;Файл неизвестного типа, пропускаю.");
            emit tick();
            continue;
        }

        delete sourceFile;
        emit tick();
    }
    emit message("<font color=blue>Конвертация закончена.</font>");
    /*if (noErrors) */emit finished();
    return true;
}
