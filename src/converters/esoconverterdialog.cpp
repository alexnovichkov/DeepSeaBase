#include "esoconverterdialog.h"
#include "settings.h"
#include "fileformats/dfdfiledescriptor.h"
#include <QtWidgets>
#include "unitsconverter.h"

class EsoFile
{
public:
    explicit EsoFile(const QString &fileName) : fileName(fileName) {
        QFile file(fileName);
        if (file.open(QFile::ReadOnly | QFile::Text)) {
            QTextStream in(&file);
            double x,y1,y2,y3,y4,y5/*,y6*/;
            while (!in.atEnd()) {
                QString s = in.readLine();
                if (s.isEmpty()) break;
                QTextStream st(&s);
                st >> x >> y1 >> y2 >> y3 >> y4 >> y5;
                xValues.append(x);
                esou.append(y1);
                mzph.append(y2);
                esot.append(y3);
                maxt.append(y4);
                esom.append(y5);
            }
        }
    }
    QVector<double> xValues;
    QVector<double> esou;
    QVector<double> mzph;
    QVector<double> esot;
    QVector<double> maxt;
    QVector<double> esom;
private:
    QString fileName;
};

EsoConverterDialog::EsoConverterDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Конвертер файлов ESO");
    thread = 0;

    progress = new QProgressBar(this);
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(start()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(stop()));
    buttonBox->buttons().constFirst()->setDisabled(true);

    convertor = new EsoConvertor();

    //edit = new QLineEdit(this);
    //edit->setReadOnly(true);

    button = new QPushButton("Выбрать", this);
    connect(button, &QPushButton::clicked, [=](){
        folder = se->getSetting("esoFolder").toString();
        QStringList files = QFileDialog::getOpenFileNames(this, "Выберите файлы *.eso",folder,"*.eso");
        if (!files.isEmpty()) {
            convertor->setFilesToConvert(files);
            se->setSetting("esoFolder", QFileInfo(files.constFirst()).canonicalFilePath());
            //QStringList matFiles = convertor->getMatFiles();
            tree->clear();
            int i=1;
            foreach (const QString &f, files) {
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
            convertor->setColumn(i,checkBox->isChecked());
        });
        checkBox->setChecked(true);
        convertor->setColumn(i,checkBox->isChecked());
        grid->addWidget(checkBox, i+2,2);
    }

    grid->addWidget(progress,7,0,1,3);
    grid->addWidget(buttonBox,8,0,1,3);
    grid->setColumnStretch(0,1);


    setLayout(grid);

    resize(qApp->primaryScreen()->availableSize().width()/3,
           qApp->primaryScreen()->availableSize().height()/2);
}

EsoConverterDialog::~EsoConverterDialog()
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

void EsoConverterDialog::accept()
{
    convertedFiles << convertor->getNewFile();
    QDialog::accept();
}

void EsoConverterDialog::reject()
{
    stop();
    QDialog::reject();
}

void EsoConverterDialog::updateProgressIndicator()
{
    progress->setValue(progress->value()+1);
}

void EsoConverterDialog::start()
{
    QString dfdFolder = se->getSetting("dfdFolder").toString();
    if (dfdFolder.isEmpty()) dfdFolder = folder;
    QString resultFile = QFileDialog::getSaveFileName(this,"Сохранение файла dfd", dfdFolder,"Файлы dfd (*.dfd)");
    if (resultFile.isEmpty()) return;
    convertor->setResultFile(resultFile);

    buttonBox->buttons().constFirst()->setDisabled(true);

    QStringList channelNames;
    for (int i=0; i<tree->topLevelItemCount(); ++i) {
        channelNames << (tree->topLevelItem(i)->text(2).isEmpty()?
                             QFileInfo(tree->topLevelItem(i)->text(1)).baseName():
                             tree->topLevelItem(i)->text(2));
    }
    convertor->setChannelNames(channelNames);

    if (!thread) {
        thread = new QThread;
        convertor->moveToThread(thread);
        connect(thread, SIGNAL(started()), convertor, SLOT(convert()));
        connect(convertor, SIGNAL(finished()), thread, SLOT(quit()));
        connect(convertor, SIGNAL(finished()), this, SLOT(finalize()));
        connect(convertor, SIGNAL(tick()), SLOT(updateProgressIndicator()));
        //connect(convertor, SIGNAL(tick(QString)), SLOT(updateProgressIndicator(QString)));
        //connect(convertor, SIGNAL(message(QString)), textEdit, SLOT(appendHtml(QString)));
    }

    progress->setValue(0);

    thread->start();
}

void EsoConverterDialog::stop()
{
    if (thread)
        thread->requestInterruption();
    QDialog::accept();
}

void EsoConverterDialog::finalize()
{
    //emit filesConverted(convertor->getNewFile());
    accept();
}



EsoConvertor::EsoConvertor(QObject *parent) : QObject(parent)
{
    columns.resize(5);
}

bool EsoConvertor::convert()
{
    if (QThread::currentThread()->isInterruptionRequested()) return false;
//    bool noErrors = true;

    if (filesToConvert.isEmpty()) {
        emit message("<font color=red>Error!</font> No files to convert.");
        emit finished();
        return false;
    }

    //writing dfd file
    DfdDataType type = ToSpectr;
    bool octave = true;

    //Converting
    EsoFile esoFile(filesToConvert.constFirst());
    QVector<double> xValues = esoFile.xValues;


    if (esoFile.xValues.size()>=3) {
        //проверяем шаг между отсчетами
        if (qFuzzyIsNull(esoFile.xValues.at(0)-2.0*esoFile.xValues.at(1) + esoFile.xValues.at(2))) {
            type = Spectr;
            octave = false;
        }
    }
    DfdFileDescriptor *dfdFileDescriptor = DfdFileDescriptor::newFile(dfdFileName, type);
//    dfdFileDescriptor->setSamplesCount(xValues.size());

    dfdFileDescriptor->dataDescription().put("createdBy", "Конвертер eso2dfd by Алексей Новичков");
    dfdFileDescriptor->dataDescription().put("dateTime", dfdFileDescriptor->dataDescription().get("fileCreationTime"));

    QStringList columnsNames;
    columnsNames << "ЭСОУ" << "МЗПХ" << "ЭСОТ" <<"MaxT"<<"ЭСОМ";

    int index=0;
    if (octave) {
        //     добавляем нулевой канал с осью Х
        DfdChannel *ch = new DfdChannel(dfdFileDescriptor, index++);
        ch->setName("ось X"); //
        ch->setYName("Гц");
        ch->setXName("Гц");
        ch->channelIndex = 0; // нумерация с 0
        ch->ChanBlockSize=xValues.size();
        ch->IndType=3221225476;
        ch->data()->setThreshold(1.0);
        ch->data()->setXValues(xValues);
        ch->data()->setYValues(xValues, DataHolder::YValuesReals);
        ch->setPopulated(true);
    }

    for (int col=0; col<5; ++col) {
        if (!columns.at(col)) continue;
        if (QThread::currentThread()->isInterruptionRequested()) return false;

        for (int i=0; i<filesToConvert.size(); ++i) {
            if (QThread::currentThread()->isInterruptionRequested()) return false;

            EsoFile esoFile(filesToConvert.at(i));
            if (esoFile.xValues != xValues) {
                emit message("<font color=red>Error!</font> Шкала по X отличается. Пропускаем файл.");
                continue;
            }

            DfdChannel *newCh = new DfdChannel(dfdFileDescriptor,index++);

            newCh->setName(channelNames.at(i)+"-"+columnsNames.at(col));
            newCh->dataDescription().put("ynameold", newCh->yName());
            newCh->ChanBlockSize=esoFile.xValues.size();
            newCh->IndType=3221225476;
            newCh->setXName("Гц");

            switch (col) {
                case 0: //esou
                case 1: //mzph
                    //newCh->InputType="U";
                    newCh->setYName("дБ");
                    newCh->dataDescription().put("ynameold", "Па");
                    break;
                case 2: //esot
                case 3: //maxt
                    //newCh->InputType="U";
                    newCh->setYName("m");
                    newCh->dataDescription().put("ynameold", "Па");
                    break;
                case 4: //esom
                    //newCh->InputType="U";
                    newCh->setYName("номер");
                    break;
                default:
                    break;
            }

            newCh->setDescription(newCh->name());
            if (octave)
                newCh->data()->setXValues(xValues);
            else
                newCh->data()->setXValues(xValues.constFirst(), xValues.at(1)-xValues.at(0), xValues.size());

            switch (col) {
                case 0:
                    newCh->data()->setThreshold(PhysicalUnits::Units::logref("pa"));
                    newCh->data()->setYValues(esoFile.esou, DataHolder::YValuesAmplitudesInDB); break;
                case 1:
                    newCh->data()->setThreshold(PhysicalUnits::Units::logref("pa"));
                    newCh->data()->setYValues(esoFile.mzph, DataHolder::YValuesAmplitudesInDB); break;
                case 2:
                    newCh->data()->setThreshold(PhysicalUnits::Units::logref("m"));
                    newCh->data()->setYValues(esoFile.esot, DataHolder::YValuesReals); break;
                case 3:
                    newCh->data()->setThreshold(PhysicalUnits::Units::logref("m"));
                    newCh->data()->setYValues(esoFile.maxt, DataHolder::YValuesReals); break;
                case 4:
                    newCh->data()->setThreshold(0.0);
                    newCh->data()->setYValues(esoFile.esom, DataHolder::YValuesReals); break;
                default: break;
            }
            newCh->setPopulated(true);
            emit tick();
        }
    }

    dfdFileDescriptor->setChanged(true);
    dfdFileDescriptor->setDataChanged(true);
    dfdFileDescriptor->write();
    delete dfdFileDescriptor;


    emit message("<font color=blue>Конвертация закончена.</font>");

    /*if (noErrors) */emit finished();
    return true;
}
