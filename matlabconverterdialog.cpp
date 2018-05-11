#include "matlabconverterdialog.h"
#include <QtWidgets>
#include "matlabfiledescriptor.h"
#include "mainwindow.h"
#include "checkableheaderview.h"

bool fileExists(const QString &s)
{
    QString f = s;
    f.replace(".mat",".dfd");
    QString f1 = s;
    f1.replace(".mat", ".raw");
    if (QFile::exists(f) && QFile::exists(f1)) return true;
    return false;
}

MatlabConverterDialog::MatlabConverterDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Конвертер matlab файлов");
    thread = 0;
    m_addFiles = false;

    progress = new QProgressBar(this);
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(start()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(stop()));
    buttonBox->buttons().first()->setDisabled(true);

    convertor = new MatlabConvertor();

    edit = new QLineEdit(this);
    edit->setReadOnly(true);

    button = new QPushButton("Выбрать", this);
    connect(button, &QPushButton::clicked, [=](){
        folder = MainWindow::getSetting("matlabFolder").toString();
        folder = QFileDialog::getExistingDirectory(this, "Выберите папку с файлами *.mat",folder);
        edit->setText(folder);
        convertor->setFolder(folder);
        MainWindow::setSetting("matlabFolder", folder);
        QStringList matFiles = convertor->getMatFiles();
        tree->clear();
        int i=1;
        foreach (const QString &f, matFiles) {
            QTreeWidgetItem *item = new QTreeWidgetItem(tree);
            item->setText(0, QString::number(i++));
            item->setText(1, f);
            if (fileExists(f)) {
                item->setIcon(2,QIcon(":/icons/tick.png"));
                item->setCheckState(1, Qt::Unchecked);
            }
            else item->setCheckState(1, Qt::Checked);
        }
        tree->resizeColumnToContents(0);
        tree->resizeColumnToContents(1);
        progress->setRange(0, i-1);
        buttonBox->buttons().first()->setDisabled(matFiles.isEmpty());
    });

    tree = new QTreeWidget(this);
    tree->setAlternatingRowColors(true);
    tree->setColumnCount(3);
    tree->setHeaderLabels(QStringList()<<"№"<<"Файл"<<"Конвертирован");

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



    QSplitter *splitter = new QSplitter(Qt::Vertical, this);
    QWidget *first = new QWidget(this);

    QGridLayout *grid = new QGridLayout;
    grid->addWidget(new QLabel("Папка:", this),0,0);
    grid->addWidget(edit,0,1);
    grid->addWidget(button,0,2);
    grid->addWidget(tree, 1,0,1,3);
    grid->addWidget(progress,2,0,1,3);
    first->setLayout(grid);

    QWidget *second = new QWidget(this);
    QGridLayout *grid1 = new QGridLayout;
    grid1->addWidget(textEdit,0,0);
    grid1->addWidget(openFolderButton, 1,0);
    grid1->addWidget(addFilesButton, 2, 0);
    grid1->addWidget(buttonBox,3,0);
    second->setLayout(grid1);
    splitter->addWidget(first);
    splitter->addWidget(second);

    QVBoxLayout *l = new QVBoxLayout;
    l->addWidget(splitter);
    l->setMargin(0);
    setLayout(l);

    resize(500,400);
}

MatlabConverterDialog::~MatlabConverterDialog()
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

void MatlabConverterDialog::accept()
{
    convertedFiles = convertor->getNewFiles();
    QDialog::accept();
}

void MatlabConverterDialog::reject()
{
    stop();
    QDialog::reject();
}

void MatlabConverterDialog::updateProgressIndicator()
{
    progress->setValue(progress->value()+1);
}

void MatlabConverterDialog::start()
{
    buttonBox->buttons().first()->setDisabled(true);
    if (!thread) thread = new QThread;
    convertor->moveToThread(thread);
    QStringList toConvert;
    for (int i=0; i<tree->topLevelItemCount(); ++i)
        if (tree->topLevelItem(i)->checkState(1)==Qt::Checked) toConvert << tree->topLevelItem(i)->text(1);
    convertor->setFilesToConvert(toConvert);
    connect(thread, SIGNAL(started()), convertor, SLOT(convert()));
    connect(convertor, SIGNAL(finished()), thread, SLOT(quit()));
    connect(convertor, SIGNAL(finished()), this, SLOT(finalize()));
    connect(convertor, SIGNAL(tick()), SLOT(updateProgressIndicator()));
    //connect(convertor, SIGNAL(tick(QString)), SLOT(updateProgressIndicator(QString)));
    connect(convertor, SIGNAL(message(QString)), textEdit, SLOT(appendHtml(QString)));

    progress->setValue(0);

    thread->start();
}

void MatlabConverterDialog::stop()
{
    if (thread)
        thread->requestInterruption();
    QDialog::accept();
}

void MatlabConverterDialog::finalize()
{
    if (openFolderButton->isChecked()) {
        QDir dir(folder);
        QProcess::startDetached("explorer.exe", QStringList(dir.toNativeSeparators(dir.absolutePath())));
    }
    m_addFiles = addFilesButton->isChecked();

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


