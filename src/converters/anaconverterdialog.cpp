#include "anaconverterdialog.h"


#include <QtWidgets>

#include "anaconverter.h"
#include "settings.h"

AnaConverterDialog::AnaConverterDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Конвертер файлов ANA/ANP");
    thread = 0;

    progress = new QProgressBar(this);
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(start()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(stop()));
    buttonBox->buttons().constFirst()->setDisabled(true);

    converter = new AnaConverter();

    //edit = new QLineEdit(this);
    //edit->setReadOnly(true);

    button = new QPushButton("Выбрать", this);
    connect(button, &QPushButton::clicked, [=](){
        folder = Settings::getSetting("anaFolder").toString();
        QStringList files = QFileDialog::getOpenFileNames(this, "Выберите файлы *.anp",folder,"*.anp");
        if (!files.isEmpty()) {
            converter->setFilesToConvert(files);
            Settings::setSetting("anaFolder", QFileInfo(files.constFirst()).canonicalFilePath());
            tree->clear();
            int i=1;
            for (const QString &f: qAsConst(files)) {
                QTreeWidgetItem *item = new QTreeWidgetItem(tree);
                item->setText(0, QString::number(i++));
                item->setText(1, QFileInfo(f).fileName());
                item->setFlags(item->flags() | Qt::ItemIsEditable);
                //item->setText(2, QFileInfo(f).fileName());
            }
            tree->resizeColumnToContents(0);
            tree->resizeColumnToContents(1);
            progress->setRange(0, i-1);
            buttonBox->buttons().constFirst()->setDisabled(files.isEmpty());
        }
    });

    tree = new QTreeWidget(this);
    tree->setAlternatingRowColors(true);
    tree->setColumnCount(3);
    tree->setHeaderLabels(QStringList()<<"№"<< "Файл"<<"Название канала");
    tree->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

    QStringList columns;
    columns << "ЭСОУ" << "МЗПХ" << "ЭСОТ" <<"MaxT"<<"ЭСОМ";

    //QSplitter *splitter = new QSplitter(Qt::Vertical, this);
    //QWidget *first = new QWidget(this);

    QGridLayout *grid = new QGridLayout;
    grid->addWidget(new QLabel("Файлы", this),0,0);
    grid->addWidget(button,0,1);
    grid->addWidget(tree, 1,0,6,2);
    grid->addWidget(new QLabel("Столбцы", this),1,2,1,1,Qt::AlignBottom);

    for (int i=0; i<5; ++i) {
        QCheckBox *checkBox = new QCheckBox(columns.at(i), this);
        connect(checkBox, &QCheckBox::clicked, [=](){
//            converter->setColumn(i,checkBox->isChecked());
        });
        checkBox->setChecked(true);
//        converter->setColumn(i,checkBox->isChecked());
        grid->addWidget(checkBox, i+2,2);
    }

    grid->addWidget(progress,7,0,1,3);
    grid->addWidget(buttonBox,8,0,1,3);
    grid->setColumnStretch(0,1);


    setLayout(grid);

    resize(qApp->primaryScreen()->availableSize().width()/3,
           qApp->primaryScreen()->availableSize().height()/2);
}

AnaConverterDialog::~AnaConverterDialog()
{

}

void AnaConverterDialog::accept()
{

}

void AnaConverterDialog::reject()
{

}

void AnaConverterDialog::updateProgressIndicator()
{

}

void AnaConverterDialog::start()
{

}

void AnaConverterDialog::stop()
{

}

void AnaConverterDialog::finalize()
{

}
