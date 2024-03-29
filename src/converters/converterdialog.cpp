#include "converterdialog.h"
#include <QtWidgets>
#include "checkableheaderview.h"
#include "logging.h"
#include "algorithms.h"
#include "settings.h"
#include "fileformats/abstractformatfactory.h"
#include "app.h"
#include "fileformats/filedescriptor.h"

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
    progress->setTextVisible(false);
    progress->setFixedHeight(10);

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(start()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(stop()));
//    buttonBox->buttons().constFirst()->setDisabled(true);

    convertor = new FileConverter();

    addFilesButton = new QCheckBox("Добавить новые файлы в текущую вкладку", this);

    button = new QPushButton("Добавить файлы", this);
    connect(button, &QPushButton::clicked, [=](){
        folder = se->getSetting("uffFolder").toString();
        QStringList filters = App->formatFactory->allFilters();
        QStringList files = QFileDialog::getOpenFileNames(this, "Выберите файлы",folder,
                                                          filters.join(";;"));

        if (!files.isEmpty())
            se->setSetting("uffFolder", QFileInfo(files.constFirst()).canonicalPath());

        foreach (const QString &f, files) {
            addFile(f);
        }
        tree->resizeColumnToContents(0);
        buttonBox->buttons().constFirst()->setDisabled(files.isEmpty());
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
    formatBox->addItems(App->formatFactory->allFilters());

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
    grid1->addWidget(addFilesButton, 3,0);
    second->setLayout(grid1);
    splitter->addWidget(first);
    splitter->addWidget(second);

    QVBoxLayout *l = new QVBoxLayout;
    l->addWidget(splitter);
    l->setMargin(0);
    setLayout(l);

    resize(qApp->primaryScreen()->availableSize().width()/2,
           qApp->primaryScreen()->availableSize().height()/2);

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
    buttonBox->buttons().constFirst()->setDisabled(true);
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
    connect(convertor, SIGNAL(message(QString)), textEdit, SLOT(appendHtml(QString)));

    thread->start();
}

void ConverterDialog::stop()
{
    if (thread)
        thread->requestInterruption();

    accept();
}

void ConverterDialog::finalize()
{
    m_addFiles = addFilesButton->isChecked();
}






FileConverter::FileConverter(QObject *parent) : QObject(parent)
{

}

QStringList FileConverter::getUffFiles() const
{
    QStringList result;
    foreach (const QFileInfo &fi, uffFiles)
        result << fi.canonicalFilePath();

    return result;
}

bool FileConverter::convert()
{
    //TODO: проверить конвертацию файлов uff
    if (QThread::currentThread()->isInterruptionRequested()) return false;

    //Converting
    for(const QString &fileToConvert: filesToConvert) {
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

        FileDescriptor *sourceFile = App->formatFactory->createDescriptor(sourceFileName);

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
            auto destFiles = App->formatFactory->createDescriptors(*sourceFile, destFileName);

            for (auto f: destFiles) {
                if (f) newFiles.append(f->fileName());
                delete f;
            }
            emit message("&nbsp;&nbsp;&nbsp;Готово.");
        }
        else {
            emit message("&nbsp;&nbsp;&nbsp;Файл неизвестного типа, пропускаю.");
            emit tick();
            continue;
        }

        delete sourceFile;
        emit tick();
    }
    emit tick();
    emit message("<font color=blue>Конвертация закончена.</font>");
    /*if (noErrors) */emit finished();
    return true;
}
