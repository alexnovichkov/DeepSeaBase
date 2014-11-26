#include "convertdialog.h"

#include <QtWidgets>

#include "dfdfiledescriptor.h"
#include "methods/spectremethod.h"
#include "methods/timemethod.h"
#include "methods/xresponch1.h"

ConvertDialog::ConvertDialog(QList<FileDescriptor *> *dataBase, QWidget *parent) :
    QDialog(parent), /*dataBase(dataBase),*/ process(0)
{
    foreach (FileDescriptor *d, *dataBase) {
        DfdFileDescriptor *dd = static_cast<DfdFileDescriptor *>(d);
        if (dd)
            this->dataBase << dd;
    }

    methodCombo = new QComboBox(this);
    methodCombo->setEditable(false);
    for (int i=0; i<26; ++i) {
        methodCombo->addItem(QString(methods[i].methodDescription));
    }
    connect(methodCombo,SIGNAL(currentIndexChanged(int)), SLOT(methodChanged(int)));

    static double bands[12] = {12800.0, 6400.0, 3200.0, 1600.0, 800.0, 400.0,
                                200.0,   100.0,  50.0,   25.0,   12.5,  6.25};

    activeStripCombo = new QComboBox(this);
    activeStripCombo->setEditable(false);

    infoLabel = new QLabel(this);
    infoLabel->setWordWrap(true);

    bandWidth = dynamic_cast<RawChannel *>(dataBase->first()->channel(0))->BandWidth;
    for (int i=0; i<12; ++i) {
        if (bands[i] <= bandWidth) {
            bandWidth = bands[i];
            break;
        }
    }
    double bw = bandWidth;
    for (int i=0; i<12; ++i) {
        activeStripCombo->addItem(QString::number(bw));
        bw /= 2.0;
    }
    activeStripCombo->setCurrentIndex(2);

    activeChannelSpin = new QSpinBox(this);
    activeChannelSpin->setRange(1, 256);
    activeChannelSpin->setValue(1);
    baseChannelSpin = new QSpinBox(this);
    baseChannelSpin->setRange(1, 256);
    baseChannelSpin->setValue(2);

    methodsStack = new QStackedWidget(this);
    for (int i=0; i<26; ++i) {
        switch (i) {
            case 0: methodsStack->addWidget(new TimeMethod(this)); break;
            case 1: methodsStack->addWidget(new SpectreMethod(this)); break;
            case 9: methodsStack->addWidget(new XresponcH1Method(this)); break;

            default: methodsStack->addWidget(new SpectreMethod(this));
        }
    }

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                                       QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    QGridLayout *l = new QGridLayout;
    l->addWidget(new QLabel("Метод обработки", this), 0,0);
    l->addWidget(methodCombo,0,1);
    l->addWidget(new QLabel("Активный канал", this), 1,0);
    l->addWidget(activeChannelSpin, 1,1);
    l->addWidget(new QLabel("Опорный канал", this), 2,0);
    l->addWidget(baseChannelSpin, 2,1);
    l->addWidget(new QLabel("Частотный диапазон", this), 3,0);
    l->addWidget(activeStripCombo, 3,1);
    l->addWidget(infoLabel, 4,0,1,2);

    l->addWidget(methodsStack,0,2,6,1);
    l->addWidget(buttonBox, 6,0,1,3);


    setLayout(l);
    methodCombo->setCurrentIndex(1);
}

ConvertDialog::~ConvertDialog()
{
    if (process) {
        process->kill();
        process->deleteLater();
    }
}

QStringList ConvertDialog::getSpfFile(const QVector<int> &indexes, QString dir)
{
    dir = QDir::toNativeSeparators(dir);
    QStringList spfFile;
    spfFile << "[DeepSeaProjectFile]";
    spfFile << "MainWindow=133,139,1280,572";
    spfFile << QString("PanelsNum=%1").arg(indexes.size()); // число панелей равно числу файлов для обработки
    spfFile << "GWColors=12632256,12632256,8421504,8388736";
    spfFile << "GColor0=180";
    spfFile << "GColor1=8388608";
    spfFile << "GColor2=8421376";
    spfFile << "GColor3=16256";
    spfFile << "GColor4=33023";
    spfFile << "GColor5=16711680";
    spfFile << "GColor6=32896";
    spfFile << "GColor7=16776960";
    spfFile << "GColor8=12644592";
    spfFile << "GColor9=8388736";
    spfFile << "GColor10=16711935";
    spfFile << "GColor11=30720";
    spfFile << "GColor12=0";
    spfFile << "GColor13=8421631";
    spfFile << "GColor14=16744576";
    spfFile << "GColor15=10789024";
    spfFile << "SonColor=0";
    spfFile << QString("SoDatDir=%1").arg(dir); // файлы отсортированы по папкам
    spfFile << QString("DSDatDir=%1").arg(dir); // файлы сохраняются в ту же папку
    spfFile << "TpNameDat=1"; // к имени файла добавляется дата и время создания

    for (int i=0; i<indexes.size(); ++i) {
        DfdFileDescriptor *dfd = dataBase.at(indexes.at(i));
        spfFile << QString("[Panel%1]").arg(i);
        spfFile << QString("NmPan=п.%1").arg(i);
        spfFile << QString("PanelPar=%1,%2,%3")
                   .arg(i)
                   .arg(currentMethod->panelType()) //однооконная графическая панель
                   .arg(i+1);
        spfFile << "tVal=0"; // отображение величин: степенное

        spfFile << QString("[Panel%1\\Wind1]").arg(i); // параметры окна
        spfFile << QString("WinNm=%1").arg(currentMethod->methodName()); // описание окна
        spfFile << QString("ProcMethod=%1,%2")
                   .arg(currentMethod->methodDll())
                   .arg(currentMethod->dataType());
        spfFile << QString("FileNm=%1").arg(QDir::toNativeSeparators(dfd->fileName()));

        QStringList channelsList;
        for (int i=1; i<=dfd->channels.size(); ++i) channelsList << QString::number(i);
        //foreach(int ch, channels) channelsList << QString::number(ch);
        spfFile << "Channels="+channelsList.join(',');


        int bandStrip = stripByBandwidth(dynamic_cast<RawChannel *>(dfd->channel(0))->BandWidth);

        spfFile << QString("ActChannel=%1").arg(activeChannelSpin->value());
        spfFile << QString("BaseChannel=%1").arg(baseChannelSpin->value());
        spfFile << "MinMax=*,*,*,*";
        spfFile << QString("AStrip=%1").arg(bandStrip);
        spfFile << "StepBack=(00000000)"; // TODO: добавить возможность устанавливать перекрытие
        spfFile << "ShiftDat=0"; // TODO: добавить возможность устанавливать смещение
        // длина = число отсчетов в канале
        // TODO: добавить возможность устанавливать правую границу выборки
        quint32 NI = dfd->channels.at(activeChannelSpin->value())->ChanBlockSize
                     / dfd->BlockSize;
        NI *= dfd->samplesCount();

        spfFile << QString("Duration=%1").arg(NI);
        spfFile << "TablName=";
        spfFile << QString("ViewPar=1,0,1,%1,1,1,0").arg(bandStrip);
        spfFile << "FacePos=0";
        spfFile << "Strob=0";

        spfFile << QString("[Panel%1\\Wind1\\Method]").arg(i);
        spfFile << currentMethod->methodSettings(dfd, activeChannelSpin->value(), bandStrip);
    }
    return spfFile;
}

void ConvertDialog::methodChanged(int method)
{
    methodsStack->setCurrentIndex(method);
    currentMethod = dynamic_cast<AbstractMethod *>(methodsStack->currentWidget());
}

bool ConvertDialog::copyFilesToTempDir(const QVector<int> &indexes, const QString &tempFolderName)
{
    bool result = true;
    infoLabel->setText("Копирование файлов во временную папку...");
    qApp->processEvents();
    foreach(int index, indexes) {
        QString dfdFileName = QFileInfo(dataBase.at(index)->fileName()).fileName();
        result &= QFile::copy(dataBase.at(index)->fileName(), tempFolderName+"/"+dfdFileName);

        if (dataBase.at(index)->hasAttachedFile()) {
            QString rawFileName = QFileInfo(dataBase.at(index)->attachedFileName()).fileName();
            result &= QFile::copy(dataBase.at(index)->attachedFileName(), tempFolderName+"/"+rawFileName);
        }
        qApp->processEvents();
    }

    return result;
}

void ConvertDialog::moveFilesFromTempDir(const QString &destDir)
{
    for (int i=0; i<newFiles.size(); ++i) {
        DfdFileDescriptor dfd(newFiles.at(i));
        dfd.read();
        QString suffix = QString::number(dfd.samplesCount() * dfd.xStep());

        QString baseFileName = QFileInfo(newFiles.at(i)).completeBaseName();
        const QString oldRawFileName = QFileInfo(newFiles.at(i)).canonicalPath()+"/"+baseFileName+".raw";

        baseFileName.chop(3);
        QString dfdFileName = destDir+"/"+baseFileName+"_"+suffix+".dfd";
        QString rawFileName = destDir+"/"+baseFileName+"_"+suffix+".raw";


        int index = 0;
        if (QFile::exists(dfdFileName)) {
            index++;
            while (QFile::exists(destDir+"/"+baseFileName+"_"+suffix+"("+QString::number(index)+").dfd")) {
                index++;
            }
        }

        if (index>0) {
            dfdFileName = destDir+"/"+baseFileName+"_"+suffix+"("+QString::number(index)+").dfd";
            rawFileName = destDir+"/"+baseFileName+"_"+suffix+"("+QString::number(index)+").raw";
        }

        QFile::rename(newFiles.at(i), dfdFileName);
        QFile::rename(oldRawFileName, rawFileName);
        newFiles[i] = dfdFileName;
    }
}

int ConvertDialog::stripByBandwidth(double bandwidth)
{
    const int current = activeStripCombo->currentIndex();

    if (qAbs(bandwidth - bandWidth)<1.0e-7)
        return current;

    int val = current - int(log2(bandWidth/bandwidth));
    int result = qBound(0, val, 11);
    return result;
}

void ConvertDialog::accept()
{
    newFiles.clear();

    // сортируем записи по директории
    QMap<QString, QVector<int> > sortedFiles;
    for (int i=0; i< dataBase.size(); ++i) {
        DfdFileDescriptor *dfd = dataBase.at(i);
        QString dir = QFileInfo(dfd->fileName()).absolutePath();
        QVector<int> indexes = sortedFiles.value(dir);
        indexes.append(i);
        sortedFiles.insert(dir, indexes);
    }


    // теперь копируем все файлы из опр. папки во временную
    QDir d;
    if (!d.exists("C:/DeepSeaBase-temp"))
        d.mkdir("C:/DeepSeaBase-temp");

    QTemporaryDir tempDir("C:\\DeepSeaBase-temp\\temp-XXXXXX");
    //tempDir.setAutoRemove(false);
    if (tempDir.isValid()) {
        const QString tempFolderName = tempDir.path();

        QMapIterator<QString, QVector<int> > it(sortedFiles);
        while (it.hasNext()) {
            it.next();

            if (copyFilesToTempDir(it.value(), tempFolderName)) {
                infoLabel->setText("Подождите, пока работает DeepSea...");
                QStringList spfFile = getSpfFile(it.value(), tempFolderName);
                QTemporaryFile file("spffile_XXXXXX.spf");
                file.setAutoRemove(false);
                if (file.open()) {
                    QTextStream out(&file);
                    out.setCodec("Windows-1251");
                    foreach (QString s, spfFile) out << s << endl;
                    file.close();
                }
                else continue;

                QDateTime dt=QDateTime::currentDateTime();

                if (!process) {
                    process = new QProcess(this);
                    process->disconnect();
                    process->setProcessChannelMode(QProcess::MergedChannels);
                    process->setReadChannel(QProcess::StandardOutput);
                }

                QStringList arguments;
                arguments << file.fileName() << "-E" << "-S";

                QEventLoop q;
                connect(process,SIGNAL(finished(int)),&q,SLOT(quit()));
                connect(process,SIGNAL(error(QProcess::ProcessError)),&q,SLOT(quit()));

                process->start("DeepSea",arguments);
                q.exec();

                const int code = process->exitCode();
                if (code == 0) {
                    QFileInfoList newFilesList = QDir(tempFolderName).entryInfoList(QStringList()<<"*.dfd",QDir::Files);
                    Q_FOREACH(const QFileInfo &newFile, newFilesList) {
                        if (newFile.created()>dt || newFile.lastModified()>dt)
                            newFiles << newFile.canonicalFilePath();
                    }
                    moveFilesFromTempDir(it.key());
                }
            }
        }
    }



    QDialog::accept();
}

