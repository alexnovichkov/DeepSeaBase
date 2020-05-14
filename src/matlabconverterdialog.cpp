#include "matlabconverterdialog.h"
#include <QtWidgets>
#include "fileformats/matlabconvertor.h"
#include "mainwindow.h"
#include "checkableheaderview.h"
#include "logging.h"
#include "fileformats/formatfactory.h"
#include "algorithms.h"


MatlabConverterDialog::MatlabConverterDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Конвертер matlab файлов");
    thread = 0;
    m_addFiles = false;

    progress = new QProgressBar(this);
    progress->setTextVisible(false);
    progress->setFixedHeight(15);
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(start()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(stop()));
    buttonBox->buttons().first()->setDisabled(true);

    convertor = new MatlabConvertor();

    edit = new QLineEdit(this);
    edit->setReadOnly(true);
    edit->setPlaceholderText("путь/к/папке/Mat/");

    button = new QPushButton("Выбрать", this);
    connect(button, SIGNAL(pressed()), this, SLOT(chooseMatFiles()));

    formatBox = new QComboBox(this);
    formatBox->addItems(FormatFactory::allFilters());
    connect(formatBox, SIGNAL(currentTextChanged(QString)), SLOT(updateFormat()));

    tree = new QTreeWidget(this);
    tree->setAlternatingRowColors(true);
    tree->setColumnCount(4);
    tree->setHeaderLabels(QStringList()<<"№"<<"Файл"<<"Конвертирован"<<"Запись");
    tree->setItemDelegateForColumn(3, new ComboBoxItemDelegate(convertor, this));

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
    grid->addWidget(new QLabel("Конвертор читает файлы MAT, сохраненные с любыми выставленными флагами:\n"
                               "- он понимает данные, сохраненные как double\n"
                               "- он умеет читать сгруппированные каналы\n"
                               "- он понимает, когда файл сохранен не в единицах СИ", this),0,0,1,3);
    grid->addWidget(new QLabel("Папка:", this),1,0);
    grid->addWidget(edit,1,1);
    grid->addWidget(button,1,2);
    grid->addWidget(tree, 2,0,1,3);
    grid->addWidget(progress,3,0,1,3);
    first->setLayout(grid);

    QWidget *second = new QWidget(this);
    QGridLayout *grid1 = new QGridLayout;
    grid1->addWidget(textEdit,0,0,1,4);
    grid1->addWidget(new QLabel("Сохранять файлы как", this), 1,0,1,1);
    grid1->addWidget(formatBox, 1,1,1,1);
    grid1->addWidget(new QLabel("Записывать данные в файл RAW как", this), 1,2,1,1);
    grid1->addWidget(rawFileFormat, 1,3,1,1);
    grid1->addWidget(openFolderButton, 2,0,1,4);
    grid1->addWidget(addFilesButton, 3,0,1,4);
    grid1->addWidget(buttonBox,4,0,1,4);
    second->setLayout(grid1);
    splitter->addWidget(first);
    splitter->addWidget(second);

    QVBoxLayout *l = new QVBoxLayout;
    l->addWidget(splitter);
    l->setMargin(0);
    setLayout(l);

    resize(700,500);
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

void MatlabConverterDialog::chooseMatFiles()
{
    folder = MainWindow::getSetting("matlabFolder").toString();
    folder = QFileDialog::getExistingDirectory(this, "Выберите папку с файлами *.mat", folder);

    edit->setText(folder);
    MainWindow::setSetting("matlabFolder", folder);

    QDir folderDir(folder);
    QFileInfoList matFiles = folderDir.entryInfoList(QStringList()<<"*.mat", QDir::Files | QDir::Readable);

    tree->clear();
    int i=1;
    foreach (const QFileInfo &f, matFiles) {
        QTreeWidgetItem *item = new QTreeWidgetItem(tree);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        item->setText(0, QString::number(i++));
        item->setText(1, f.canonicalFilePath());
    }
    tree->resizeColumnToContents(0);
    tree->resizeColumnToContents(1);
    updateFormat();

    buttonBox->buttons().first()->setDisabled(matFiles.isEmpty());

    QString xmlFileName = findXmlFile(false);

    convertor->xmlFileName = xmlFileName;
    if (xmlFileName.isEmpty()) {
        textEdit->appendHtml("<font color=red>Error!</font> Не могу найти файл Analysis.xml.");
        return;
    }

    //Читаем файл xml и определяем, какие в нем записи
    bool noErrors = true;
    convertor->readXml(noErrors);
    if (!noErrors) {
        textEdit->appendHtml("<font color=red>Error!</font> Не удалось прочитать файл " + xmlFileName);
        return;
    }

    textEdit->appendHtml("Файл Analysis.xml содержит следующие записи:");
    foreach (const Dataset &dataset, convertor->xml) {
        textEdit->appendHtml(QString("%1: %2").arg(dataset.id).arg(dataset.fileName));
    }

    for (int i=0; i<tree->topLevelItemCount(); ++i) {
        QString xdfFileName = tree->topLevelItem(i)->text(1);
        xdfFileName.replace(".mat",".xdf");

        //сопоставляем наши файлы mat с записями в файле xml
        for (int j = 0; j< convertor->xml.size(); ++j) {
            const Dataset &s = convertor->xml.at(j);
            QString xdfName = QFileInfo(xdfFileName).fileName().toLower();
            QString datasetName = s.fileName.toLower();
            if (xdfName == datasetName) {
                tree->topLevelItem(i)->setData(3, Qt::UserRole, j);
                tree->topLevelItem(i)->setText(3, s.id+": "+s.fileName);
                convertor->setDatasetForFile(tree->topLevelItem(i)->text(1), j);
                break;
            }
        }

        //проверяем, все ли записи нашли
        if (tree->topLevelItem(i)->text(3).isEmpty()) {
            textEdit->appendHtml(QString("<font color=red>ОШИБКА!</font> Не удалось найти запись для файла %1")
                                 .arg(tree->topLevelItem(i)->text(1)));
            textEdit->appendHtml(QString("Выберите нужную запись в выпадающем списке для этой записи."));
            tree->topLevelItem(i)->setData(3, Qt::UserRole, -1);
            tree->topLevelItem(i)->setText(3, "Ошибка!");
            convertor->setDatasetForFile(tree->topLevelItem(i)->text(1), -1);
        }
    }
}

void MatlabConverterDialog::updateFormat()
{
    QString formatString = formatBox->currentText();
    QString suffix = formatString.right(4);
    suffix.chop(1);
    for (int i=0; i<tree->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = tree->topLevelItem(i);
        if (fileExists(item->text(1), suffix)) {
            item->setIcon(2,QIcon(":/icons/tick.png"));
            item->setCheckState(1, Qt::Unchecked);
        }
        else {
            item->setCheckState(1, Qt::Checked);
            item->setIcon(2,QIcon());
        }
    }
    rawFileFormat->setEnabled(suffix.toLower() == "dfd");

    convertor->setDestinationFormat(suffix.toLower());
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

QString MatlabConverterDialog::findXmlFile(bool silent) const
{
    if (tree->topLevelItemCount()==0) return QString();

    //определяем файл Analysis.xml по первому файлу mat, так как они всё равно в одной папке
    QString file = tree->topLevelItem(0)->text(1);

    QDir folderDir(file); // сейчас указывает на файл
    folderDir.cdUp(); // теперь указывает на папку

    QString xmlFileName;
    // ищем в текущей папке с файлами mat
    QStringList xmlFiles = folderDir.entryList(QStringList()<<"*.xml", QDir::Files | QDir::Readable);
    // ищем в папке Settings
    if (xmlFiles.isEmpty()) {
        folderDir.cdUp();
        if (folderDir.cd("Settings"))
            xmlFiles = folderDir.entryList(QStringList()<<"*.xml", QDir::Files | QDir::Readable);
    }

    if (xmlFiles.contains("Analysis.xml",Qt::CaseInsensitive))
        xmlFileName = folderDir.canonicalPath()+"/"+"Analysis.xml";
    if (!silent && xmlFileName.isEmpty())
    // последняя попытка - вручную
        xmlFileName = QFileDialog::getOpenFileName(0, QString("Укажите файл XML с описанием каналов"), file, "Файлы XML (*.xml)");
    return xmlFileName;
}

void MatlabConverterDialog::start()
{
    buttonBox->buttons().first()->setDisabled(true);

    QStringList toConvert;
    for (int i=0; i<tree->topLevelItemCount(); ++i)
        if (tree->topLevelItem(i)->checkState(1)==Qt::Checked) toConvert << tree->topLevelItem(i)->text(1);
    convertor->setFilesToConvert(toConvert);

    if (toConvert.isEmpty()) {
        textEdit->appendHtml("<font color=red>Error!</font> Отсутствуют файлы для конвертации.");
        buttonBox->buttons().first()->setDisabled(false);
        return;
    }

    progress->setRange(0, toConvert.size());
    progress->setValue(0);
    convertor->setRawFileFormat(rawFileFormat->currentIndex());


    if (!thread) thread = new QThread;
    convertor->moveToThread(thread);
    connect(thread, SIGNAL(started()), convertor, SLOT(convert()));
    connect(convertor, SIGNAL(finished()), thread, SLOT(quit()));
    connect(convertor, SIGNAL(finished()), this, SLOT(finalize()));
    connect(convertor, SIGNAL(tick()), SLOT(updateProgressIndicator()));
    connect(convertor, SIGNAL(message(QString)), textEdit, SLOT(appendHtml(QString)));

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
}


QWidget *ComboBoxItemDelegate::createEditor(QWidget *parent,
                                            const QStyleOptionViewItem &/* option */,
                                            const QModelIndex &/* index */) const
{DD;
    QComboBox *editor = new QComboBox(parent);
    if (editor) {
        for (int i=0; i<convertor->xml.size(); ++i)
            editor->addItem(convertor->xml.at(i).id + ": "+convertor->xml.at(i).fileName);
    }
    return editor;
}

void ComboBoxItemDelegate::setEditorData(QWidget *editor,
                                         const QModelIndex &index) const
{DD;
    if (!index.isValid()) return;
    int row = index.model()->data(index, Qt::UserRole).toInt();

    QComboBox *box = qobject_cast<QComboBox*>(editor);
    if (box) box->setCurrentIndex(row);
}

void ComboBoxItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                        const QModelIndex &index) const
{DD;
    if (!index.isValid()) return;

    QComboBox *box = qobject_cast<QComboBox*>(editor);
    if (box) {
        model->setData(index, box->currentIndex(), Qt::UserRole);
        model->setData(index, box->currentText(), Qt::DisplayRole);
        convertor->setDatasetForFile(model->data(model->index(index.row(), 1)).toString(), box->currentIndex());
    }
}

void ComboBoxItemDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (!editor || !index.isValid()) return;

    if (QComboBox* e = qobject_cast<QComboBox*>(editor)) {
        QRect r = option.rect;
        int max = 0;
        for(int i=0; i<e->count(); ++i)
            max = qMax(max, e->fontMetrics().width(e->itemText(i)));
        r.setWidth(max);
        e->setGeometry(r.adjusted(0,0,10,0));
    }
}
