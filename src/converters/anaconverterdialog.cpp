#include "anaconverterdialog.h"


#include <QtWidgets>

#include "anaconverter.h"
#include "settings.h"
#include "fileformats/abstractformatfactory.h"
#include "app.h"
#include "fileformats/anafile.h"

AnaConverterDialog::AnaConverterDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Конвертер ANA/ANP файлов");
    thread = 0;

    progress = new QProgressBar(this);
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(start()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(stop()));
    buttonBox->buttons().constFirst()->setDisabled(true);

    converter = new AnaConverter();

    fileFormat = new QComboBox(this);
    fileFormat->addItems(App->formatFactory->allFilters());
    connect(fileFormat, SIGNAL(currentTextChanged(QString)), SLOT(updateFormat()));

    edit = new QLineEdit(this);
    edit->setReadOnly(true);
    edit->setPlaceholderText("путь/к/папке/ANP/");

    button = new QPushButton("Выбрать", this);
    connect(button, SIGNAL(pressed()), this, SLOT(chooseFiles()));

    tree = new QTreeWidget(this);
    tree->setAlternatingRowColors(true);
    tree->setColumnCount(4);
    tree->setHeaderLabels({"№","Файл","Канал","Дата/время"});

    textEdit = new QPlainTextEdit(this);

    openFolderButton = new QCheckBox("Открыть папку с файлами после конвертации", this);

    addFilesButton = new QCheckBox("Добавить новые файлы в текущую вкладку", this);

    QSplitter *splitter = new QSplitter(Qt::Vertical, this);
    QWidget *first = new QWidget(this);

    QGridLayout *grid = new QGridLayout;
    grid->addWidget(new QLabel("Папка:", this),1,0);
    grid->addWidget(edit,1,1);
    grid->addWidget(button,1,2);
    grid->addWidget(tree, 2,0,1,3);
    grid->addWidget(progress,3,0,1,3);
    first->setLayout(grid);

    QWidget *second = new QWidget(this);
    QGridLayout *grid1 = new QGridLayout;
    grid1->addWidget(textEdit,0,0,1,4);
    grid1->addWidget(new QLabel("Сохранять как", this), 1,0,1,1);
    grid1->addWidget(fileFormat, 1,1,1,1);
    grid1->addWidget(openFolderButton, 2,0,1,4);
    grid1->addWidget(addFilesButton, 3, 0,1,4);
    grid1->addWidget(buttonBox,4,0,1,4);
    second->setLayout(grid1);
    splitter->addWidget(first);
    splitter->addWidget(second);

    QVBoxLayout *l = new QVBoxLayout;
    l->addWidget(splitter);
    l->setMargin(0);
    setLayout(l);

    resize(qApp->primaryScreen()->availableSize().width()/3,
           qApp->primaryScreen()->availableSize().height()/2);
}

AnaConverterDialog::~AnaConverterDialog()
{
    if (converter) {
        converter->deleteLater();
    }
    if (thread) {
        thread->quit();
        thread->wait();
        thread->deleteLater();
    }
}

bool AnaConverterDialog::addFiles() const
{
    return addFilesButton->isChecked();
}

void AnaConverterDialog::accept()
{
    convertedFiles << converter->getNewFiles();
    QDialog::accept();
}

void AnaConverterDialog::reject()
{
    stop();
    QDialog::reject();
}

void AnaConverterDialog::updateProgressIndicator()
{
    progress->setValue(progress->value()+1);
}

void AnaConverterDialog::chooseFiles()
{
    folder = Settings::getSetting("anaFolder").toString();
    folder = QFileDialog::getExistingDirectory(this, "Выберите папку с файлами *.anp/*.ana", folder);

    edit->setText(folder);
    Settings::setSetting("anaFolder", folder);

    QDir folderDir(folder);
    QFileInfoList files = folderDir.entryInfoList(QStringList()<<"*.anp", QDir::Files | QDir::Readable);

    tree->clear();
    int i=1;
    for (const QFileInfo &f: qAsConst(files)) {
        QTreeWidgetItem *item = new QTreeWidgetItem(tree);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        item->setText(0, QString::number(i++));
        item->setText(1, f.canonicalFilePath());
        AnaFile file(f.canonicalFilePath());
        file.read();
        item->setText(2, file.channel(0)->name());
        item->setText(3, file.dateTime().toString("yyyy.MM.dd hh:mm:ss"));
    }
    tree->resizeColumnToContents(0);
    tree->resizeColumnToContents(1);
    updateFormat();

    buttonBox->buttons().constFirst()->setDisabled(files.isEmpty());
}

void AnaConverterDialog::start()
{
    buttonBox->buttons().constFirst()->setDisabled(true);

    QStringList toConvert;
    for (int i=0; i<tree->topLevelItemCount(); ++i)
        toConvert << tree->topLevelItem(i)->text(1);
    converter->setFilesToConvert(toConvert);

    if (toConvert.isEmpty()) {
        textEdit->appendHtml("<font color=red>Error!</font> Отсутствуют файлы для конвертации.");
        buttonBox->buttons().constFirst()->setDisabled(false);
        return;
    }

    progress->setRange(0, toConvert.size());

    if (!thread) thread = new QThread;
    converter->moveToThread(thread);
    connect(thread, SIGNAL(started()), converter, SLOT(convert()));
    connect(converter, SIGNAL(finished()), thread, SLOT(quit()));
    connect(converter, SIGNAL(finished()), this, SLOT(finalize()));
    connect(converter, SIGNAL(tick()), SLOT(updateProgressIndicator()));
    //connect(convertor, SIGNAL(tick(QString)), SLOT(updateProgressIndicator(QString)));
    connect(converter, SIGNAL(message(QString)), textEdit, SLOT(appendHtml(QString)));

    progress->setValue(0);

    thread->start();
}

void AnaConverterDialog::stop()
{
    if (thread)
        thread->requestInterruption();
    QDialog::accept();
}

void AnaConverterDialog::finalize()
{
    if (openFolderButton->isChecked()) {
        QDir dir(folder);
        QProcess::startDetached("explorer.exe", QStringList(dir.toNativeSeparators(dir.absolutePath())));
    }
}

void AnaConverterDialog::updateFormat()
{
    QString formatString = fileFormat->currentText();
    QString suffix = formatString.right(4);
    suffix.chop(1);
    converter->setDestinationFormat(suffix.toLower());
}
