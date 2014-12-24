#include "convertdialog.h"

#include <QtWidgets>
#include "converters.h"
#include "dfdfiledescriptor.h"
#include "methods/spectremethod.h"
#include "methods/timemethod.h"
#include "methods/xresponch1.h"

#include "logging.h"

#include "samplerate.h"
#include "iirfilter.h"
#include "windowing.h"

ConvertDialog::ConvertDialog(QList<FileDescriptor *> *dataBase, QWidget *parent) :
    QDialog(parent), /*dataBase(dataBase),*/ process(0)
{DD;
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

    useDeepsea = new QCheckBox("Использовать DeepSea для расчета спектров", this);
    connect(useDeepsea, &QCheckBox::clicked, [=](){
        if (useDeepsea->isChecked()) {
            QString s = QStandardPaths::findExecutable("DeepSea");
            if (s.isEmpty()) infoLabel->setText("Не могу найти DeepSea.exe в стандартных путях.\n"
                                                "Добавьте путь к DeepSea.exe в переменную PATH");
        }
        else infoLabel->clear();
    });



    progress = new QProgressBar(this);
    int progressMax = 0;
    foreach(DfdFileDescriptor *dfd, this->dataBase) {
        progressMax += dfd->channelsCount();
    }

    progress->setRange(0, progressMax);
    progress->hide();

    bandWidth = dynamic_cast<RawChannel *>(dataBase->first()->channel(0))->BandWidth;
    for (int i=0; i<12; ++i) {
        if (bands[i]-bandWidth <= 0.01) {
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
    overlap = new QSpinBox(this);
    overlap->setRange(0,75);
    overlap->setValue(0);
    overlap->setSingleStep(5);

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
    l->addWidget(new QLabel("Перекрытие, %", this), 4,0);
    l->addWidget(overlap, 4,1);
    l->addWidget(infoLabel, 5,0,1,2);
    l->addWidget(progress,6,0,1,2);

    l->addWidget(methodsStack,0,2,7,1);
    l->addWidget(useDeepsea,7,0,1,3);
    l->addWidget(buttonBox, 8,0,1,3);


    setLayout(l);
    methodCombo->setCurrentIndex(1);
}

ConvertDialog::~ConvertDialog()
{DD;
    if (process) {
        process->kill();
        process->deleteLater();
    }
}

QStringList ConvertDialog::getSpfFile(/*const QVector<int> &indexes, */QString dir)
{DD;
//    dir = QDir::toNativeSeparators(dir);
    QStringList spfFile;
    spfFile << "[DeepSeaProjectFile]";
    spfFile << "MainWindow=133,139,1280,572";
    spfFile << QString("PanelsNum=%1").arg(dataBase.size()); // число панелей равно числу файлов для обработки
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
    spfFile << QString("SoDatDir=C:\\Program Files (x86)\\DeepSea\\Data");
    spfFile << QString("DSDatDir=%1").arg(dir); // файлы сохраняются в ту же папку
    spfFile << "TpNameDat=2"; // к имени файла добавляется дата и время создания

    for (int i=0; i<dataBase.size(); ++i) {
        DfdFileDescriptor *dfd = dataBase.at(i);
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
        spfFile << QString("StepBack=%1").arg(floattohex(overlap->value())); // TODO: добавить возможность устанавливать перекрытие
//qDebug()<<spfFile;
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

QString ConvertDialog::createUniqueFileName(const QString &tempFolderName, const QString &fileName)
{

    QString method = currentMethod->methodDll();
    method.chop(4);
    QString result = tempFolderName+"\\"+QFileInfo(fileName).baseName()+"_"+method;

    int index = 0;
    QString suffix = QString::number(index);
    suffix = suffix.rightJustified(3,'0');

    while (QFile::exists(result+suffix+".dfd")) {
        index++;
        suffix = QString::number(index).rightJustified(3,'0');
    }
    return result+suffix+".dfd";
}

QVector<float> getBlock(const QVector<double> &values, const Parameters &p, quint32 &block)
{
    const quint32 N = values.size();

    double leftBorder_ = -1.0 * p.blockSize * p.overlap + block;

    QVector<float> output;

    int leftBorder = qRound(leftBorder_);
    if (leftBorder < 0) leftBorder = 0;
    if (leftBorder >= N) return output;

    QVector<double> out = values.mid(leftBorder, p.blockSize);
    output.resize(out.size());

    for (int i=0; i<out.size(); ++i) output[i] = float(out[i]);

    block = leftBorder + p.blockSize;
    return output;
}

void average(QVector<double> &result, const QVector<double> &input, const Parameters &p, int averagesMade)
{
    //int averagingType; //0 - линейное
    //1 - экспоненциальное
    //2 - хранение максимума

    // предполагаем, что result хранится в линейной шкале
    double rho=1.0 / p.averagesCount;

    for (int i=0; i<p.fCount; ++i) {
        // усреднение по максимуму
        if (p.averagingType==2) {//
            result[i] = qMax(input[i], result[i]);
        }
        else if (p.averagingType==0) //линейное
            result[i] = ((averagesMade-1)*result[i]+input[i])/averagesMade;
        else if (p.averagingType==1) //экспоненциальное
            result[i] = (1.0-rho)*result[i]+rho*input[i];
    }
}

void changeScale(QVector<double> &output, const Parameters &p)
{DD;
    const double t2 = p.threshold * p.threshold;
    if (p.scaleType > 0)
        for (int i=0; i<output.size(); ++i)
            output[i] = 10 * log10(output[i] / t2);
}

void ConvertDialog::convert(DfdFileDescriptor *dfd, const QString &tempFolderName)
{DD;
    infoLabel->setText(QString("Подождите, пока идет расчет для файла\n%1").arg(dfd->fileName()));
    Parameters p = currentMethod->parameters(dfd);
    p.sampleRate = 1.0 / dfd->XStep;
    p.bandStrip = stripByBandwidth(p.sampleRate / 2.56);
    p.overlap = 1.0 * overlap->value() / 100;

    Windowing w(p);
    p.window = w.windowing();

    p.fCount = qRound((double)p.blockSize / 2.56);

    const double newSampleRate = p.sampleRate / pow(2.0, p.bandStrip);

    /** 1. Создаем конечный файл и копируем в него всю информацию из dfd */

    QString fileName = createUniqueFileName(tempFolderName, dfd->fileName());

    DfdFileDescriptor *newDfd = new DfdFileDescriptor(fileName);

    newDfd->fillPreliminary(Descriptor::Spectrum);
    newDfd->BlockSize = 0;

    // [DataDescription]
    if (dfd->dataDescription) {
        newDfd->dataDescription = new DataDescription(newDfd);
        newDfd->dataDescription->data = dfd->dataDescription->data;
    }
    newDfd->DescriptionFormat = dfd->DescriptionFormat;

    // [Sources]
    newDfd->source = new Source();
    QStringList l; for (int i=1; i<=dfd->channelsCount(); ++i) l << QString::number(i);
    newDfd->source->sFile = dfd->fileName()+"["+l.join(",")+"]"+dfd->DFDGUID;

    // [Process]
    newDfd->process = new Process();
    QStringList processData = currentMethod->settings(dfd, p.bandStrip);
    foreach (const QString &data, processData)
        newDfd->process->data.append({data.section("=",0,0), data.section("=",1)});
    newDfd->process->data.append({"ProcChansList", l.join(",")});

    // rest
    newDfd->XName = "Гц";
    newDfd->XStep = newSampleRate / p.blockSize;

    p.averagesCount = int(1.0 * dfd->channels[0]->samplesCount() / p.blockSize / (1.0 - p.overlap));
    if (dfd->channels[0]->samplesCount() % p.averagesCount !=0) p.averagesCount++;

    for (int i=0; i<dfd->channelsCount(); ++i) {
        if (!dfd->channel(i)->populated()) dfd->channel(i)->populate();

       //qDebug()<<dfd->channels[i]->YValues.size()<<UINT_MAX;

        p.threshold = threshold(dfd->channels[i]->yName());

        QVector<double> spectrum(p.fCount);


        quint32 block = 0;

        SRC_STATE *src_state = 0;
        SRC_DATA src_data;
        src_data.src_ratio = 1.0 / (2 << (p.bandStrip-1));
        int error;
        if (p.bandStrip > 0) src_state = src_new(2, 1, &error);
        src_data.end_of_input = 0;

        src_data.input_frames = p.blockSize;

        QVector<float> filterOut(p.blockSize+100);
        src_data.output_frames = filterOut.size();
        src_data.data_out = filterOut.data();


        QVector<double> filtered;

        quint32 averagesMade = 1;

        bool reachedEnd = false;

        while (1) {
            while (filtered.size() < p.blockSize) {
                QVector<float> chunk = getBlock(dfd->channels[i]->YValues, p, block);
                const int chunkSize = chunk.size();
                if (chunkSize < p.blockSize) {
                    reachedEnd = true;
                    break;
                }

                if (p.bandStrip > 0) {//do filtration
                    src_data.data_in = chunk.data();

                    if ((error = src_process (src_state, &src_data))) {
                        qDebug()<< "Error :" << src_strerror(error);
                        break;
                    }
                    QVector<double> fil(p.blockSize / (2 << (p.bandStrip-1)));
                    for (int i=0; i<fil.size(); ++i) fil[i] = filterOut[i];
                    filtered << fil;
                }
                else {
                    filtered.resize(p.blockSize);
                    for (int i=0; i<filtered.size(); ++i) filtered[i] = double(chunk[i]);
                }
            }
            if (reachedEnd) break;

            //window
            applyWindow(filtered, p);
            //spectre
            QVector<double> out = computeSpectre(filtered, p);
            //average
            average(spectrum, out, p, averagesMade++);

            filtered.clear();
        }
        changeScale(spectrum, p);

        if (src_state) src_delete(src_state);

        // Создаем канал и заполняем его
        DfdChannel *ch = new DfdChannel(newDfd, newDfd->channelsCount());
        ch->YValues = spectrum;

        ch->yMin = 1.0e100;
        ch->yMax = -1.0e100;

        for (int i=0; i<spectrum.size(); ++i) {
            if (spectrum[i] < ch->yMin) ch->yMin = spectrum[i];
            if (spectrum[i] > ch->yMax) ch->yMax = spectrum[i];
        }

        ch->setPopulated(true);
        ch->setName(dfd->channels[i]->name());

        ch->ChanDscr = dfd->channels[i]->ChanDscr;

        ch->ChanBlockSize = spectrum.size();
        ch->NumInd = spectrum.size();
        ch->IndType = 3221225476;

        ch->YName = p.scaleType==0?dfd->channels[i]->yName():"дБ";
        ch->YNameOld = dfd->channels[i]->yName();
        ch->XName = "Гц";
        ch->XStep = newDfd->XStep;

        ch->xMin = 0.0;
        ch->xMax = newSampleRate / 2.56;
        ch->XMaxInitial = ch->xMax;
        ch->YMinInitial = ch->yMin;
        ch->YMaxInitial = ch->yMax;

        newDfd->channels << ch;

        dfd->channels[i]->clear();

        progress->setValue(progress->value()+1);
        qApp->processEvents();
    }




    newDfd->NumChans = newDfd->channels.size();
    newDfd->setSamplesCount(newDfd->channel(0)->samplesCount());
    newDfd->setChanged(true);
    newDfd->setDataChanged(true);
    newDfd->write();
    newDfd->writeRawFile();
    delete newDfd;
}

QVector<double> ConvertDialog::filter(QVector<double> &input, Parameters &p)
{DD;

}

void ConvertDialog::applyWindow(QVector<double> &values, const Parameters &p)
{DD;
    quint32 i=0;
    while (1) {
        for (int j=0; j<p.window.size(); j++) {
            if (i >= values.size()) return;
            values[i] = values[i] * p.window[j];
            i++;
        }
    }
}

const double TwoPi = 6.283185307179586;

// возвращает квадрат амплитуды * 2 / размер блока^2
void FFTAnalysis(const QVector<double> &AVal, QVector<double> &FTvl)
{
    int Nvl = AVal.size();
    int Nft = FTvl.size();

    int i, j, n, m, Mmax, Istp;
    double Tmpr, Tmpi, Wtmp, Theta;
    double Wpr, Wpi, Wr, Wi;
    QVector<double> Tmvl;

    n = Nvl * 2;
    Tmvl = QVector<double>(n, 0.0);

    for (i = 0; i < n; i+=2) {
        Tmvl[i] = 0;
        Tmvl[i+1] = AVal[i/2];
    }

    i = 1; j = 1;
    while (i < n) {
        if (j > i) {
            Tmpr = Tmvl[i]; Tmvl[i] = Tmvl[j]; Tmvl[j] = Tmpr;
            Tmpr = Tmvl[i+1]; Tmvl[i+1] = Tmvl[j+1]; Tmvl[j+1] = Tmpr;
        }
        i = i + 2; m = Nvl;
        while ((m >= 2) && (j > m)) {
            j = j - m; m = m >> 1;
        }
        j = j + m;
    }

    Mmax = 2;
    while (n > Mmax) {
        Theta = -TwoPi / Mmax; Wpi = sin(Theta);
        Wtmp = sin(Theta / 2); Wpr = Wtmp * Wtmp * 2;
        Istp = Mmax * 2; Wr = 1; Wi = 0; m = 1;

        while (m < Mmax) {
            i = m; m = m + 2; Tmpr = Wr; Tmpi = Wi;
            Wr = Wr - Tmpr * Wpr - Tmpi * Wpi;
            Wi = Wi + Tmpr * Wpi - Tmpi * Wpr;

            while (i < n) {
                j = i + Mmax;
                Tmpr = Wr * Tmvl[j] - Wi * Tmvl[j-1];
                Tmpi = Wi * Tmvl[j] + Wr * Tmvl[j-1];

                Tmvl[j] = Tmvl[i] - Tmpr; Tmvl[j-1] = Tmvl[i-1] - Tmpi;
                Tmvl[i] = Tmvl[i] + Tmpr; Tmvl[i-1] = Tmvl[i-1] + Tmpi;
                i = i + Istp;
            }
        }

        Mmax = Istp;
    }

    for (i = 0; i < Nft; i++) {
        j = i * 2; FTvl[Nft - i - 1] = 2* (pow(Tmvl[j],2) + pow(Tmvl[j+1],2))/Nvl/Nvl;
    }
}

QVector<double> ConvertDialog::computeSpectre(const QVector<double> &values,
                                              const Parameters &p)
{DD;
    QVector<double> output(p.blockSize);
    FFTAnalysis(values, output);
    output.resize(p.fCount);

    return output;
}

void ConvertDialog::methodChanged(int method)
{DD;
    methodsStack->setCurrentIndex(method);
    currentMethod = dynamic_cast<AbstractMethod *>(methodsStack->currentWidget());
}

void ConvertDialog::moveFilesFromTempDir(const QString &tempFolderName, QString fileName)
{DD;

    QString destDir = QFileInfo(fileName).canonicalPath();
    QString method = currentMethod->methodDll();
    method.chop(4);
    QString filter=QString("%1/%2_%3")
                   .arg(tempFolderName)
                   .arg(QFileInfo(fileName).completeBaseName())
                   .arg(method);
    filter.replace("\\","/");
    QStringList filtered = newFiles_.filter(filter);

    //Q_ASSERT(filtered.size()<2);
    if (filtered.isEmpty()) return;

    QString baseFileName = QFileInfo(filtered.first()).completeBaseName();

    DfdFileDescriptor dfd(filtered.first());
    dfd.read();
    QString suffix = QString::number(dfd.samplesCount() * dfd.xStep());

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

    QFile::rename(filtered.first(), dfdFileName);
    QFile::rename(dfd.attachedFileName(), rawFileName);
    newFiles << dfdFileName;
    newFiles_.removeAll(filtered.first());
}

int ConvertDialog::stripByBandwidth(double bandwidth)
{DD;
    const int current = activeStripCombo->currentIndex();

    if (qAbs(bandwidth - bandWidth)<1.0e-3)
        return current;

    int val = current - int(log2(bandWidth/bandwidth));
    int result = qBound(0, val, 11);
    return result;
}

void ConvertDialog::accept()
{DD;
    newFiles.clear();

    QDateTime dt=QDateTime::currentDateTime();
    qDebug()<<"Start converting"<<dt.time();

    QDir d;
    if (!d.exists("C:/DeepSeaBase-temp"))
        d.mkdir("C:/DeepSeaBase-temp");

    QTemporaryDir tempDir("C:\\DeepSeaBase-temp\\temp-XXXXXX");
    tempDir.setAutoRemove(false);
    const QString tempFolderName = tempDir.path();

    progress->show();
    progress->setValue(0);

    if (!useDeepsea->isChecked()) {
        foreach (DfdFileDescriptor *dfd, dataBase) {
            convert(dfd, tempFolderName);
        }
    }
    else {
        infoLabel->setText("Подождите, пока работает DeepSea...\n"
                           "Не забудьте закрыть DeepSea, когда она закончит расчеты");
        QStringList spfFile = getSpfFile(tempFolderName/*it.value(), tempFolderName*/);
        QTemporaryFile file("spffile_XXXXXX.spf");
        file.setAutoRemove(false);
        if (file.open()) {
            QTextStream out(&file);
            out.setCodec("Windows-1251");
            foreach (QString s, spfFile) out << s << endl;
            file.close();
        }
        else QDialog::reject();

        QFileSystemWatcher watcher(QStringList()<<tempFolderName,this);
        connect(&watcher, SIGNAL(directoryChanged(QString)), SLOT(updateProgressIndicator(QString)));

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
        if (code != 0) {
            QMessageBox::critical(this, "DeepSea Base - обработка", "DeepSea завершился с ошибкой!");
            return;
        }
    }

    QFileInfoList newFilesList = QDir(tempFolderName).entryInfoList(QStringList()<<"*.dfd",QDir::Files);
    Q_FOREACH(const QFileInfo &newFile, newFilesList) {
        if (newFile.created()>dt || newFile.lastModified()>dt)
            newFiles_ << newFile.canonicalFilePath();
    }

    foreach (DfdFileDescriptor *dfd, dataBase) {
        moveFilesFromTempDir(tempFolderName, dfd->fileName());
    }

    qDebug()<<"End converting"<<QDateTime::currentDateTime().time();

    QDialog::accept();
}

void ConvertDialog::updateProgressIndicator(const QString &path)
{DD;
    progress->setValue(QDir(path).count()-2);
}

