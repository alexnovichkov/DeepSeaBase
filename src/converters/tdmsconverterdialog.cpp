#include "tdmsconverterdialog.h"

#include <QtWidgets>
#include "fileformats/tdmsfile.h"
#include "mainwindow.h"
#include "checkableheaderview.h"
#include "logging.h"
#include "algorithms.h"

TDMSConverterDialog::TDMSConverterDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Конвертер tdm/tdms файлов");
    thread = 0;
    m_addFiles = false;

    progress = new QProgressBar(this);
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(start()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(stop()));
    buttonBox->buttons().constFirst()->setDisabled(true);

    convertor = new TDMSFileConvertor();

    edit = new QLineEdit(this);
    edit->setReadOnly(true);
    edit->setPlaceholderText("путь/к/папке/TDM/");

    button = new QPushButton("Выбрать", this);
    connect(button, SIGNAL(pressed()), this, SLOT(chooseFiles()));

    tree = new QTreeWidget(this);
    tree->setAlternatingRowColors(true);
    tree->setColumnCount(3);
    tree->setHeaderLabels(QStringList()<<"№"<<"Файл"<<"Конвертирован");
    //tree->setItemDelegateForColumn(4, new ComboBoxItemDelegate(convertor, this));
    //connect(tree, SIGNAL(itemClicked(QTreeWidgetItem*,int)), SLOT(editItem(QTreeWidgetItem*,int)));

    CheckableHeaderView *tableHeader = new CheckableHeaderView(Qt::Horizontal, tree);

    tree->setHeader(tableHeader);
    tableHeader->setCheckable(1,true);
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

    openFolderButton = new QCheckBox("Открыть папку с файлами после конвертации", this);

    addFilesButton = new QCheckBox("Добавить новые файлы в текущую вкладку", this);

    rawFileFormat = new QComboBox(this);
    rawFileFormat->addItem("действительные числа в формате single");
    rawFileFormat->addItem("целые 16-битные числа");

    QSplitter *splitter = new QSplitter(Qt::Vertical, this);
    QWidget *first = new QWidget(this);

    QGridLayout *grid = new QGridLayout;
//    grid->addWidget(new QLabel("Конвертор читает файлы MAT, сохраненные с любыми выставленными флагами:\n"
//                               "- он понимает данные, сохраненные как double\n"
//                               "- он умеет читать сгруппированные каналы\n"
//                               "- он понимает, когда файл сохранен не в единицах СИ", this),0,0,1,3);
    grid->addWidget(new QLabel("Папка:", this),1,0);
    grid->addWidget(edit,1,1);
    grid->addWidget(button,1,2);
    grid->addWidget(tree, 2,0,1,3);
    grid->addWidget(progress,3,0,1,3);
    first->setLayout(grid);

    QWidget *second = new QWidget(this);
    QGridLayout *grid1 = new QGridLayout;
    grid1->addWidget(textEdit,0,0,1,3);
    grid1->addWidget(new QLabel("Записывать данные в файл RAW как", this), 1,0,1,1);
    grid1->addWidget(rawFileFormat, 1,1,1,1);
    grid1->addWidget(openFolderButton, 2,0,1,3);
    grid1->addWidget(addFilesButton, 3, 0,1,3);
    grid1->addWidget(buttonBox,4,0,1,3);
    second->setLayout(grid1);
    splitter->addWidget(first);
    splitter->addWidget(second);

    QVBoxLayout *l = new QVBoxLayout;
    l->addWidget(splitter);
    l->setMargin(0);
    setLayout(l);

    resize(700,500);
}

TDMSConverterDialog::~TDMSConverterDialog()
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

void TDMSConverterDialog::chooseFiles()
{
    folder = MainWindow::getSetting("tdmsFolder").toString();
    folder = QFileDialog::getExistingDirectory(this, "Выберите папку с файлами *.tdm/*.tdms", folder);

    edit->setText(folder);
    MainWindow::setSetting("tdmsFolder", folder);

    QDir folderDir(folder);
    QFileInfoList tdmsFiles = folderDir.entryInfoList(QStringList()<<"*.tdms", QDir::Files | QDir::Readable);
    tdmsFiles.append(folderDir.entryInfoList(QStringList()<<"*.tdm", QDir::Files | QDir::Readable));

    tree->clear();
    int i=1;
    foreach (const QFileInfo &f, tdmsFiles) {
        QTreeWidgetItem *item = new QTreeWidgetItem(tree);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        item->setText(0, QString::number(i++));
        item->setText(1, f.canonicalFilePath());
        if (fileExists(f.canonicalFilePath(), "dfd")) {
            item->setIcon(2,QIcon(":/icons/tick.png"));
            item->setCheckState(1, Qt::Unchecked);
        }
        else item->setCheckState(1, Qt::Checked);
    }
    tree->resizeColumnToContents(0);
    tree->resizeColumnToContents(1);

    buttonBox->buttons().constFirst()->setDisabled(tdmsFiles.isEmpty());
}

void TDMSConverterDialog::accept()
{
    convertedFiles = convertor->getNewFiles();
    QDialog::accept();
}

void TDMSConverterDialog::reject()
{
    stop();
    QDialog::reject();
}

void TDMSConverterDialog::updateProgressIndicator()
{
    progress->setValue(progress->value()+1);
}

void TDMSConverterDialog::start()
{
    buttonBox->buttons().constFirst()->setDisabled(true);

    QStringList toConvert;
    for (int i=0; i<tree->topLevelItemCount(); ++i)
        if (tree->topLevelItem(i)->checkState(1)==Qt::Checked) toConvert << tree->topLevelItem(i)->text(1);
    convertor->setFilesToConvert(toConvert);

    if (toConvert.isEmpty()) {
        textEdit->appendHtml("<font color=red>Error!</font> Отсутствуют файлы для конвертации.");
        buttonBox->buttons().constFirst()->setDisabled(false);
        return;
    }

    progress->setRange(0, toConvert.size()-1);
    convertor->setRawFileFormat(rawFileFormat->currentIndex());

    if (!thread) thread = new QThread;
    convertor->moveToThread(thread);
    connect(thread, SIGNAL(started()), convertor, SLOT(convert()));
    connect(convertor, SIGNAL(finished()), thread, SLOT(quit()));
    connect(convertor, SIGNAL(finished()), this, SLOT(finalize()));
    connect(convertor, SIGNAL(tick()), SLOT(updateProgressIndicator()));
    //connect(convertor, SIGNAL(tick(QString)), SLOT(updateProgressIndicator(QString)));
    connect(convertor, SIGNAL(message(QString)), textEdit, SLOT(appendHtml(QString)));

    progress->setValue(0);

    thread->start();
}

void TDMSConverterDialog::stop()
{
    if (thread)
        thread->requestInterruption();
    QDialog::accept();
}

void TDMSConverterDialog::finalize()
{
    if (openFolderButton->isChecked()) {
        QDir dir(folder);
        QProcess::startDetached("explorer.exe", QStringList(dir.toNativeSeparators(dir.absolutePath())));
    }
    m_addFiles = addFilesButton->isChecked();

//    buttonBox->buttons().constFirst()->setDisabled(false);
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