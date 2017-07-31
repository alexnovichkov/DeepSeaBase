#include "esoconverterdialog.h"
#include "mainwindow.h"
#include "dfdfiledescriptor.h"
#include <QtWidgets>

class EsoFile
{
public:
    explicit EsoFile(const QString &fileName) : fileName(fileName) {
        QFile file(fileName);
        if (file.open(QFile::ReadOnly | QFile::Text)) {
            QTextStream in(&file);
            double x,y1,y2,y3,y4,y5,y6;
            while (!in.atEnd()) {
                QString s = in.readLine();
                if (s.isEmpty()) break;
                QTextStream st(&s);
                st >> x >> y1 >> y2 >> y3 >> y4 >> y5 >> y6;
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
    buttonBox->buttons().first()->setDisabled(true);

    convertor = new EsoConvertor();

    //edit = new QLineEdit(this);
    //edit->setReadOnly(true);

    button = new QPushButton("Выбрать", this);
    connect(button, &QPushButton::clicked, [=](){
        folder = MainWindow::getSetting("esoFolder").toString();
        QStringList files = QFileDialog::getOpenFileNames(this, "Выберите файлы *.eso",folder,"*.eso");
        if (!files.isEmpty()) {
            convertor->setFilesToConvert(files);
            MainWindow::setSetting("esoFolder", QFileInfo(files.first()).canonicalFilePath());
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
            buttonBox->buttons().first()->setDisabled(files.isEmpty());
        }
    });

    tree = new QTreeWidget(this);
    tree->setAlternatingRowColors(true);
    tree->setColumnCount(3);
    tree->setHeaderLabels(QStringList()<<"№"<< "Файл"<<"Название канала");

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


    setLayout(grid);

    resize(500,400);

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
    QString dfdFolder = MainWindow::getSetting("dfdFolder").toString();
    if (dfdFolder.isEmpty()) dfdFolder = folder;
    QString resultFile = QFileDialog::getSaveFileName(this,"Сохранение файла dfd", dfdFolder,"Файлы dfd (*.dfd)");
    if (resultFile.isEmpty()) return;
    convertor->setResultFile(resultFile);

    buttonBox->buttons().first()->setDisabled(true);
    if (!thread) thread = new QThread;
    convertor->moveToThread(thread);
    QStringList channelNames;
    for (int i=0; i<tree->topLevelItemCount(); ++i) {
        channelNames << (tree->topLevelItem(i)->text(2).isEmpty()?
                             QFileInfo(tree->topLevelItem(i)->text(1)).baseName():
                             tree->topLevelItem(i)->text(2));
    }

    convertor->setChannelNames(channelNames);
    connect(thread, SIGNAL(started()), convertor, SLOT(convert()));
    connect(convertor, SIGNAL(finished()), thread, SLOT(quit()));
    connect(convertor, SIGNAL(finished()), this, SLOT(finalize()));
    connect(convertor, SIGNAL(tick()), SLOT(updateProgressIndicator()));
    //connect(convertor, SIGNAL(tick(QString)), SLOT(updateProgressIndicator(QString)));
    //connect(convertor, SIGNAL(message(QString)), textEdit, SLOT(appendHtml(QString)));

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
    accept();
}



EsoConvertor::EsoConvertor(QObject *parent)
{
    columns.resize(5);
}

bool EsoConvertor::convert()
{
    if (QThread::currentThread()->isInterruptionRequested()) return false;
    bool noErrors = true;

    if (filesToConvert.isEmpty()) {
        emit message("<font color=red>Error!</font> No files to convert.");
        emit finished();
        return false;
    }

    QString rawFileName = dfdFileName;
    rawFileName.replace(".dfd",".raw");

    //writing dfd file
    DfdFileDescriptor dfdFileDescriptor(dfdFileName);
    dfdFileDescriptor.rawFileName = rawFileName;
    dfdFileDescriptor.fillPreliminary(Descriptor::Unknown);
    dfdFileDescriptor.DataType = ToSpectr;

    dfdFileDescriptor.BlockSize = 0;
    dfdFileDescriptor.XName="Гц";

   // dfdFileDescriptor.DescriptionFormat = "lms2dfd.DF";
    dfdFileDescriptor.CreatedBy = "Конвертер eso2dfd by Алексей Новичков";

    dfdFileDescriptor.process = new Process();
    dfdFileDescriptor.process->data.append({"pName","1/3-октавный спектр"});
    dfdFileDescriptor.process->data.append({"pTime","(0000000000000000)"});
    dfdFileDescriptor.process->data.append({"TypeProc","1/3-октава"});
    dfdFileDescriptor.process->data.append({"Values","измеряемые"});
    dfdFileDescriptor.process->data.append({"TypeScale","в децибелах"});

    //Converting
    QString fi = filesToConvert.first();
    EsoFile esoFile(fi);
    DfdChannel *newCh = new DfdChannel(&dfdFileDescriptor,0);
    newCh->YName="Гц";
    newCh->ChanName="ось X";
    newCh->ChanBlockSize=esoFile.xValues.size();
    newCh->IndType=3221225476;
    newCh->XName = dfdFileDescriptor.XName;
    newCh->setYValues(esoFile.xValues);
    newCh->setXValues(esoFile.xValues);
    newCh->NumInd=esoFile.xValues.size();
    newCh->setPopulated(true);

    dfdFileDescriptor.channels.append(newCh);
    dfdFileDescriptor.setSamplesCount(esoFile.xValues.size());

    QStringList columnsNames;
    columnsNames << "ЭСОУ" << "МЗПХ" << "ЭСОТ" <<"MaxT"<<"ЭСОМ";

    int index=1;
    for (int col=0; col<5; ++col) {
        if (!columns.at(col)) continue;
        if (QThread::currentThread()->isInterruptionRequested()) return false;

        for (int i=0; i<filesToConvert.size(); ++i) {
            QString fi = filesToConvert.at(i);
            if (QThread::currentThread()->isInterruptionRequested()) return false;

            EsoFile esoFile(fi);
            if (esoFile.xValues != dfdFileDescriptor.channels.first()->XValues) {
                emit message("<font color=red>Error!</font> Шкала по X отличается. Пропускаем файл.");
                continue;
            }

            DfdChannel *newCh = new DfdChannel(&dfdFileDescriptor,index++);


            //newCh->ChanAddress=ch->ChanAddress;
            newCh->ChanName=channelNames.at(i)+"-"+columnsNames.at(col);
            newCh->YNameOld=newCh->YName;
            newCh->ChanBlockSize=esoFile.xValues.size();
            newCh->IndType=3221225476;
            //BandWidth=1162346496
            switch (col) {
                case 0: //esou
                case 1: //mzph
                    newCh->InputType="U";
                    newCh->YName="дБ";
                    newCh->YNameOld="Па";
                    break;
                case 2: //esot
                case 3: //maxt
                    newCh->InputType="U";
                    newCh->YName="m";
                    newCh->YNameOld="Па";
                    break;
                case 4: //esom
                    newCh->InputType="U";
                    newCh->YName="номер";
                    newCh->YNameOld="Па";
                    break;
                default:
                    break;
            }

            newCh->ChanDscr=newCh->ChanName;
            newCh->NumInd=newCh->ChanBlockSize;
            newCh->XName = dfdFileDescriptor.XName;

            switch (col) {
                case 0: newCh->setYValues(esoFile.esou); break;
                case 1: newCh->setYValues(esoFile.mzph); break;
                case 2: newCh->setYValues(esoFile.esot); break;
                case 3: newCh->setYValues(esoFile.maxt); break;
                case 4: newCh->setYValues(esoFile.esom); break;
                default: break;
            }
            newCh->setPopulated(true);
            dfdFileDescriptor.channels.append(newCh);
            emit tick();
        }
    }

    dfdFileDescriptor.NumChans = dfdFileDescriptor.channels.size();
    dfdFileDescriptor.XBegin = 0.0;
    dfdFileDescriptor.XStep = 0.0;

    dfdFileDescriptor.setChanged(true);
    dfdFileDescriptor.setDataChanged(true);
    dfdFileDescriptor.write();
    dfdFileDescriptor.writeRawFile();


    emit message("<font color=blue>Конвертация закончена.</font>");

    /*if (noErrors) */emit finished();
    return true;
}
