#include "converter.h"

#include <QtCore>
#include "logging.h"
#include "dfdfiledescriptor.h"
#include "converters.h"

#include "samplerate.h"

Converter::Converter(QList<DfdFileDescriptor *> &base, const Parameters &p, QObject *parent) :
    QObject(parent), dataBase(base), p(p), process(0)
{DD;
    stop_ = false;

}

Converter::~Converter()
{DD;
    if (process) {
        process->kill();
        process->deleteLater();
    }

}

void Converter::stop()
{DD;
    stop_ = true;
    finalize();
}

void Converter::start()
{DD;
    dt = QDateTime::currentDateTime();
    qDebug()<<"Start converting"<<dt.time();

    QDir d;
    if (!d.exists("C:/DeepSeaBase-temp"))
        d.mkdir("C:/DeepSeaBase-temp");

    QTemporaryDir tempDir("C:\\DeepSeaBase-temp\\temp-XXXXXX");
    tempDir.setAutoRemove(true);
    tempFolderName = tempDir.path();

    if (!p.useDeepSea) {
        foreach (DfdFileDescriptor *dfd, dataBase) {
            if (!convert(dfd, tempFolderName)) return;
        }
    }
    else {
        emit message("Подождите, пока работает DeepSea...\n"
                           "Не забудьте закрыть DeepSea, когда она закончит расчеты");
        QStringList spfFile = getSpfFile(tempFolderName);
        QTemporaryFile file("spffile_XXXXXX.spf");
        file.setAutoRemove(false);
        if (file.open()) {
            QTextStream out(&file);
            out.setCodec("Windows-1251");
            foreach (QString s, spfFile) out << s << endl;
            file.close();
        }
        else return;

        QFileSystemWatcher watcher(QStringList()<<tempFolderName,this);
        connect(&watcher, SIGNAL(directoryChanged(QString)), SIGNAL(tick(QString)));

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
            emit message("DeepSea завершился с ошибкой!");
        }
    }

    finalize();

}

void Converter::finalize()
{DD;
    QFileInfoList newFilesList = QDir(tempFolderName).entryInfoList(QStringList()<<"*.dfd",QDir::Files);
    Q_FOREACH(const QFileInfo &newFile, newFilesList) {
        if (newFile.created()>dt || newFile.lastModified()>dt)
            newFiles_ << newFile.canonicalFilePath();
    }

    foreach (DfdFileDescriptor *dfd, dataBase) {
        moveFilesFromTempDir(tempFolderName, dfd->fileName());
        foreach (DfdChannel *c, dfd->channels) {
            c->floatValues.clear();
        }
    }

    qDebug()<<"End converting"<<QDateTime::currentDateTime().time();
    emit finished();
}

void Converter::moveFilesFromTempDir(const QString &tempFolderName, QString fileName)
{DD;

    QString destDir = QFileInfo(fileName).canonicalPath();
    QString method = p.methodDll;
    method.chop(4);
    QString filter=QString("%1/%2_%3")
                   .arg(tempFolderName)
                   .arg(QFileInfo(fileName).completeBaseName())
                   .arg(method);
    filter.replace("\\","/");
    QStringList filtered = newFiles_.filter(filter);

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

QString window(int wind)
{DD;
    switch (wind) {
        case 0: return "Прямоуг.";
        case 1: return "Бартлетта";
        case 2: return "Хеннинга";
        case 3: return "Хемминга";
        case 4: return "Натолл";
        case 5: return "Гаусс";
    }
    return "";
}

QString averaging(int avgType)
{
    switch (avgType) {
        case 0: return "линейное";
        case 1: return "экспоненциальное";
        case 2: return "хранение максимума";
    }
    return "";
}

QVector<float> getBlock(const QVector<float> &values, quint32 blockSize, quint32 stepBack, quint32 &block)
{
    const quint32 N = values.size();
    QVector<float> output;
    if (block >= N) return output;
    output = values.mid(block, blockSize);
    block += blockSize - stepBack;
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

QString createUniqueFileName(const QString &tempFolderName, const QString &fileName, QString method)
{DD;
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

int stripByBandwidth(double bandwidth, Parameters &p)
{
    if (qAbs(bandwidth - p.bandWidth)<1.0e-3)
        return p.initialBandStrip;

    int val = p.initialBandStrip - int(log2(p.bandWidth/bandwidth));
    int result = qBound(0, val, 11);
    return result;
}

bool Converter::convert(DfdFileDescriptor *dfd, const QString &tempFolderName)
{DD;
    emit message(QString("Подождите, пока идет расчет для файла\n%1").arg(dfd->fileName()));
    if (QThread::currentThread()->isInterruptionRequested()) {
        finalize();
        return false;
    }

    p.sampleRate = 1.0 / dfd->XStep;
    p.bandStrip = stripByBandwidth(p.sampleRate / 2.56, p);

    const double newSampleRate = p.sampleRate / pow(2.0, p.bandStrip);

    /** 1. Создаем конечный файл и копируем в него всю информацию из dfd */

    QString fileName = createUniqueFileName(tempFolderName, dfd->fileName(), p.methodDll);

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
    newDfd->process->data.append({"PName", p.methodName});
    newDfd->process->data.append({"BlockIn", QString::number(p.blockSize)});
    newDfd->process->data.append({"Wind", window(p.windowType)});
    newDfd->process->data.append({"TypeAver", averaging(p.averagingType)});
    newDfd->process->data.append({"pTime","(0000000000000000)"});

    // rest
    newDfd->XName = "Гц";
    newDfd->XStep = newSampleRate / p.blockSize;

    p.averagesCount = int(1.0 * dfd->channels[0]->samplesCount() / p.blockSize / (1.0 - p.overlap));
    if (dfd->channels[0]->samplesCount() % p.averagesCount !=0) p.averagesCount++;

    for (int i=0; i<dfd->channelsCount(); ++i) {
        dfd->channels[i]->populateFloat();

        if (QThread::currentThread()->isInterruptionRequested()) {
            delete newDfd;
            finalize();
            return false;
        }

        p.threshold = threshold(dfd->channels[i]->yName());
        QVector<double> spectrum(p.fCount);
        quint32 block = 0;
        quint32 averagesMade = 1;

        QVector<float> filtered;

        SRC_DATA src_data;
        src_data.src_ratio = 1.0 / (1 << p.bandStrip);
        int error;

        SRC_STATE *src_state = src_new(2, 1, &error);
        src_data.end_of_input = 0;

        const int newBlockSize = p.blockSize * (1<<p.bandStrip);

        const quint32 stepBack = quint32(1.0*newBlockSize * p.overlap); //qDebug()<<stepBack;

        QVector<float> filterOut(newBlockSize+100);
        src_data.output_frames = filterOut.size();
        src_data.data_out = filterOut.data();

        while (1) {
            // получаем блок данных размером blockSize * 2^bandStrip // 2048 4096 и т.д.
            QVector<float> chunk = getBlock(dfd->channels[i]->floatValues, newBlockSize, stepBack, block);

            if (chunk.size() < p.blockSize) break;

            if (p.bandStrip > 0) {//do filtration
                src_data.data_in = chunk.data();
                src_data.input_frames = chunk.size();

                if ((error = src_process(src_state, &src_data))) {
                    qDebug()<< "Error :" << src_strerror(error);
                    break;
                }
                filtered = filterOut.mid(0, p.blockSize);
            }
            else {
                filtered = chunk;
            }

            Q_ASSERT(filtered.size() == p.blockSize);

            //window
            applyWindow(filtered, p);
            //spectre
            QVector<double> out = computeSpectre(filtered, p);
            //average
            average(spectrum, out, p, averagesMade++);
        }
        if (src_state) src_delete(src_state);

        changeScale(spectrum, p);


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

        dfd->channels[i]->floatValues.clear();

        emit tick();
    }

    newDfd->NumChans = newDfd->channels.size();
    newDfd->setSamplesCount(newDfd->channel(0)->samplesCount());
    newDfd->setChanged(true);
    newDfd->setDataChanged(true);
    newDfd->write();
    newDfd->writeRawFile();
    delete newDfd;
    return true;
}

void Converter::applyWindow(QVector<float> &values, const Parameters &p)
{
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
void FFTAnalysis(const QVector<float> &AVal, QVector<double> &FTvl)
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

QVector<double> Converter::computeSpectre(const QVector<float> &values,
                                              const Parameters &p)
{
    QVector<double> output(p.blockSize);
    FFTAnalysis(values, output);
    output.resize(p.fCount);

    return output;
}

QStringList Converter::getSpfFile(QString dir)
{DD;
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
                   .arg(p.panelType) //однооконная графическая панель
                   .arg(i+1);
        spfFile << "tVal=0"; // отображение величин: степенное

        spfFile << QString("[Panel%1\\Wind1]").arg(i); // параметры окна
        spfFile << QString("WinNm=%1").arg(p.methodName); // описание окна
        spfFile << QString("ProcMethod=%1,%2")
                   .arg(p.methodDll)
                   .arg(p.dataType);
        spfFile << QString("FileNm=%1").arg(QDir::toNativeSeparators(dfd->fileName()));

        QStringList channelsList;
        for (int i=1; i<=dfd->channels.size(); ++i) channelsList << QString::number(i);

        spfFile << "Channels="+channelsList.join(',');


        p.bandStrip = stripByBandwidth(dynamic_cast<RawChannel *>(dfd->channel(0))->BandWidth, p);

        spfFile << QString("ActChannel=%1").arg(p.activeChannel);
        spfFile << QString("BaseChannel=%1").arg(p.baseChannel);
        spfFile << "MinMax=*,*,*,*";
        spfFile << QString("AStrip=%1").arg(p.bandStrip);
        spfFile << QString("StepBack=%1").arg(floattohex(p.overlap));

        spfFile << "ShiftDat=0"; // TODO: добавить возможность устанавливать смещение
        // длина = число отсчетов в канале
        // TODO: добавить возможность устанавливать правую границу выборки
        quint32 NI = dfd->channels.at(p.activeChannel)->ChanBlockSize / dfd->BlockSize;
        NI *= dfd->samplesCount();

        spfFile << QString("Duration=%1").arg(NI);
        spfFile << "TablName=";
        spfFile << QString("ViewPar=1,0,1,%1,1,1,0").arg(p.bandStrip);
        spfFile << "FacePos=0";
        spfFile << "Strob=0";

        spfFile << QString("[Panel%1\\Wind1\\Method]").arg(i);

        //spfFile << method->methodSettings(dfd, p.activeChannel, p.bandStrip);

        spfFile << QString("YName=дБ");
        spfFile << QString("BlockIn=%1").arg(p.blockSize);
        spfFile << QString("Wind=%1").arg(window(p.windowType));
        spfFile << QString("TypeAver=%1").arg(p.averagingType);

        quint32 numberOfInd = dfd->channels.at(p.activeChannel)->samplesCount();
        double blockSize = p.blockSize;
        double NumberOfAveraging = double(numberOfInd) / blockSize / (1<<p.bandStrip);

        // at least 2 averaging
        if (NumberOfAveraging<1) NumberOfAveraging = 2.0;

        spfFile << QString("NAver=%1").arg(qRound(NumberOfAveraging));
        spfFile << "TypeProc=мощности";
        spfFile << "Values=измеряемые";

        spfFile << (QString("TypeScale=")+(p.scaleType==0?QString("линейная"):QString("в децибелах")));
        spfFile << "AddProc=нет";
    }
    return spfFile;
}
