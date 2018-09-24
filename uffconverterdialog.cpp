#include "uffconverterdialog.h"
#include <QtWidgets>
#include "ufffile.h"
//#include "uffconvertor.h"
#include "dfdfiledescriptor.h"
#include "ufffile.h"
#include "mainwindow.h"
#include "checkableheaderview.h"
#include "logging.h"
#include "algorithms.h"

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

    convertor = new UffConvertor();

    button = new QPushButton("Добавить файлы", this);
    connect(button, &QPushButton::clicked, [=](){
        folder = MainWindow::getSetting("uffFolder").toString();
        QStringList files = QFileDialog::getOpenFileNames(this, "Выберите файлы",folder,
                                                          "Файлы dfd (*.dfd);;Файлы uff (*.uff);;Все файлы (*.dfd, *.uff)");

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



    QSplitter *splitter = new QSplitter(Qt::Vertical, this);
    QWidget *first = new QWidget(this);

    QGridLayout *grid = new QGridLayout;
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

    foreach(FileDescriptor *fd,dataBase)
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






UffConvertor::UffConvertor(QObject *parent) : QObject(parent)
{

}

QStringList UffConvertor::getUffFiles() const
{
    QStringList result;
    foreach (const QFileInfo &fi, uffFiles)
        result << fi.canonicalFilePath();

    return result;
}

bool UffConvertor::convert()
{
    if (QThread::currentThread()->isInterruptionRequested()) return false;

    //Converting
    foreach(const QString &fi, filesToConvert) {
        if (QThread::currentThread()->isInterruptionRequested()) return false;

        emit message("<font color=green>Конвертируем файл "+fi+"</font>");

        bool fileIsDfd = fi.endsWith(".dfd",Qt::CaseInsensitive);
        bool fileIsUff = fi.endsWith(".uff",Qt::CaseInsensitive) || fi.endsWith(".unv",Qt::CaseInsensitive);

//        if (!fileIsDfd) {
//            emit message("&nbsp;&nbsp;&nbsp;Файл имеет тип UFF. Такие файлы пока не обрабатываюся.");
//            continue;
//        }

        QString destFileName = fi;
        QString sourceFileName = fi;

        if (fileIsDfd) {//replace *.dfd to *.uff
            destFileName = createUniqueFileName("", destFileName, "", "uff", false);

            DfdFileDescriptor dfdFile(sourceFileName);
            dfdFile.read();
            dfdFile.populate();

            if (dfdFile.DataType > DfdDataType::FilterData) {//типы 0, 1, 2 - исходные данные
                emit message("&nbsp;&nbsp;&nbsp;Файл не имеет временных реализаций. Такие файлы пока не обрабатываются.");
                continue;
            }

            emit message(QString("&nbsp;&nbsp;&nbsp;В файле %1 каналов").arg(dfdFile.channelsCount()));

            //reading uff file structure
            UffFileDescriptor uffFile(destFileName);

            //заполнение header
            uffFile.header.type151[10].value = QDateTime(dfdFile.Date, dfdFile.Time);
            uffFile.header.type151[12].value = QDateTime(dfdFile.Date, dfdFile.Time);
            uffFile.header.type151[16].value = QDateTime::currentDateTime();

            int referenceChannelNumber = -1; //номер опорного канала ("сила")
            //заполнение каналов
            for (int i=0; i<dfdFile.channelsCount(); ++i) {
                Function *f = new Function(*dfdFile.channel(i));
                f->type58[15].value = i+1;
                f->type58[10].value = QString("Record %1").arg(i+1);

                if (dfdFile.channel(i)->xName().toLower()=="сила")
                    referenceChannelNumber = i;

                f->type58[8].value = QDateTime(dfdFile.Date, dfdFile.Time);
                uffFile.channels << f;
                //clearing
                dfdFile.channel(i)->setPopulated(false);
                dfdFile.channel(i)->xValues().clear(); dfdFile.channel(i)->xValues().squeeze();
                dfdFile.channel(i)->yValues().clear(); dfdFile.channel(i)->yValues().squeeze();
            }
            //заполнение инфы об опорном канале
            if (referenceChannelNumber>=0) {
                for (int nc=0; nc<uffFile.channels.size(); ++nc) {
                    uffFile.channels[nc]->type58[18].value = dfdFile.channel(nc)->name();
                    uffFile.channels[nc]->type58[19].value = referenceChannelNumber+1;
                }
            }

            //сохранение
            emit message("&nbsp;&nbsp;&nbsp;Сохраняю файл");
            uffFile.setChanged(true);
            uffFile.write();
            newFiles.append(destFileName);

            emit message("&nbsp;&nbsp;&nbsp;Готово.");
            emit tick();
        }
        else if (fileIsUff) {//replace *.uff to *.dfd
            destFileName = createUniqueFileName("", destFileName, "", "dfd", false);

            UffFileDescriptor uffFile(sourceFileName);
            uffFile.read();
            if (uffFile.type()==Descriptor::Unknown) {
                emit message("&nbsp;&nbsp;&nbsp;Файл неизвестного типа, пропускаю.");
                continue;
            }
            uffFile.populate();

            DfdFileDescriptor dfdFileDescriptor(destFileName);
            dfdFileDescriptor.fillPreliminary(uffFile.type());
            QList<QPair<FileDescriptor *, int> > channelsToCopy;
            for (int chn=0; chn<uffFile.channelsCount(); ++chn)
                channelsToCopy << qMakePair<FileDescriptor *, int>(&uffFile, chn);

            dfdFileDescriptor.copyChannelsFrom(channelsToCopy);
            dfdFileDescriptor.fillRest();

            dfdFileDescriptor.setChanged(true);
            dfdFileDescriptor.setDataChanged(true);
            dfdFileDescriptor.write();
            dfdFileDescriptor.writeRawFile();

            emit message("Готово.");
            emit tick();
        }
        else {
            emit message("&nbsp;&nbsp;&nbsp;Файл неизвестного типа, пропускаю.");
            continue;
        }
    }
    emit message("<font color=blue>Конвертация закончена.</font>");
    /*if (noErrors) */emit finished();
    return true;
}
