#include "converter.h"

#include <QtCore>
#include "logging.h"
#include "dfdfiledescriptor.h"
#include "ufffile.h"
#include "converters.h"
#include "windowing.h"
#include "samplerate.h"
#include "octavefilterbank.h"
#include "algorithms.h"

class Filter
{
public:
    Filter(const Parameters &p) : p(p)
    {
        src_data.src_ratio = 1.0 / (1 << p.bandStrip);

        src_state = src_new(2, 1, &_error);
        src_data.end_of_input = 0;

        const int newBlockSize = p.bufferSize* (1<<p.bandStrip);

        filterOut.resize(newBlockSize+100);
        src_data.output_frames = filterOut.size();
        src_data.data_out = filterOut.data();
    }

    virtual ~Filter()
    {
        if (src_state)
            src_delete(src_state);
    }


    QVector<float> process(QVector<float> &chunk)
    {
        if (p.bandStrip > 0) {//do filtration
            src_data.data_in = chunk.data();
            src_data.input_frames = chunk.size();

            _error = src_process(src_state, &src_data);
            return filterOut.mid(0, p.bufferSize);
        }
        else {
            return chunk;
        }
    }

    QString error() const {return src_strerror(_error);}
private:
    SRC_DATA src_data;
    SRC_STATE *src_state;
    QVector<float> filterOut;
    int _error;
    Parameters p;
};

Converter::Converter(QList<DfdFileDescriptor *> &base, const Parameters &p_, QObject *parent) :
    QObject(parent), dataBase(base), p(p_), process(0)
{DD;
    stop_ = false;

    Windowing w(p);
    p.window = w.windowing();
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

#include "windows.h"

BOOL CALLBACK enumWindowsProc(HWND hWnd, LPARAM lParam)
{
    Q_UNUSED(lParam);
    int length = ::GetWindowTextLength( hWnd );
    if( 0 == length ) return TRUE;

    TCHAR* buffer;
    buffer = new TCHAR[ length + 1 ];
    memset( buffer, 0, ( length + 1 ) * sizeof( TCHAR ) );
    GetWindowText( hWnd, buffer, length + 1 );
    QString windowTitle = QString::fromWCharArray(buffer );
    delete[] buffer;

    if (windowTitle=="Confirm") {
        HWND firstChild = GetWindow(hWnd, GW_CHILD);
        while (firstChild != 0) {
            length = ::GetWindowTextLength( hWnd );
            TCHAR* buffer1;
            buffer1 = new TCHAR[ length + 1 ];
            memset( buffer1, 0, ( length + 1 ) * sizeof( TCHAR ) );
            GetWindowText( firstChild, buffer1, length + 1 );
            windowTitle = QString::fromWCharArray(buffer1);
            delete[] buffer1;
            if (windowTitle.contains("Yes")) {
                SendMessage(firstChild, BM_CLICK, 0, 0);
                break;
            }
            else
                firstChild = GetWindow(firstChild, GW_HWNDNEXT);
        }
    }

    return TRUE;
}

void Converter::start()
{DD;
    dt = QDateTime::currentDateTime();
    qDebug()<<"Start converting"<<dt.time();
    emit message(QString("Запуск расчета: %1").arg(dt.time().toString()));

    QDir d;
    if (!d.exists("C:/DeepSeaBase-temp"))
        d.mkdir("C:/DeepSeaBase-temp");

    QTemporaryDir tempDir("C:\\DeepSeaBase-temp\\temp-XXXXXX");
    tempDir.setAutoRemove(true);
    tempFolderName = tempDir.path();

    // Проверяем размер блока файлов и насильно конвертируем файлы с помощью DeepSea,
    // если размер блока равен 1
    bool blockSizeIs1 = false;
    foreach (DfdFileDescriptor *dfd, dataBase) {
        if (dfd->BlockSize==1) {
            blockSizeIs1 = true;
            break;
        }
    }
    if (blockSizeIs1) {
        p.useDeepSea = true;
        emit message("Размер блока в одном из файлов равен 1. Это затрудняет"
                     " чтение данных, поэтому для расчета будет использован DeepSea.");
    }
    if (p.saveAsComplex && p.useDeepSea) {
        emit message("DeepSea не умеет сохранять комплексные результаты. Уберите одну из галок");
        return;
    }

    if (!p.useDeepSea) {
        p.baseChannel--;
        foreach (DfdFileDescriptor *dfd, dataBase) {
            emit message(QString("Подождите, пока идет расчет для файла\n%1").arg(dfd->fileName()));
            if (!convert(dfd, tempFolderName)) {
                emit message("Не удалось сконвертировать файл " + dfd->fileName());
            }
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

        QTimer *timer = new QTimer(this);
        timer->setInterval(1000);
        connect(timer, SIGNAL(timeout()), this, SLOT(processTimer()));
        timer->start();

        q.exec();
        timer->stop();


        const int code = process->exitCode();
        if (code != 0) {
            emit message("DeepSea завершился с ошибкой!");
        }
    }

    finalize();
}

void Converter::processTimer()
{
    EnumWindows(enumWindowsProc, 0L);
}

void Converter::finalize()
{DD;
    QFileInfoList newFilesList = QDir(tempFolderName).entryInfoList(QStringList()<<"*.dfd",QDir::Files);
    newFilesList.append(QDir(tempFolderName).entryInfoList(QStringList()<<"*.uff",QDir::Files));


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
    emit message(QString("Расчет закончен в %1").arg(QDateTime::currentDateTime().time().toString()));
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

    if (filtered.isEmpty()) {
        filter=QString("%1/%2_%3")
                       .arg(tempFolderName)
                       .arg(QFileInfo(fileName).baseName())
                       .arg(method);
        filter.replace("\\","/");
        filtered = newFiles_.filter(filter);
    }

    if (filtered.isEmpty()) return;

    //QString baseFileName = QFileInfo(filtered.first()).completeBaseName();
    QString baseFileName = QFileInfo(fileName).completeBaseName()+"_"+method;

    if (filtered.first().toLower().endsWith("dfd")) {
        DfdFileDescriptor dfd(filtered.first());
        dfd.read();
        int suffixN = dfd.samplesCount() * dfd.xStep();
        QString suffix = QString::number(suffixN);

        if (suffixN==0) {//третьоктава или файл с неодинаковым шагом по абсциссе
            dfd.populate();
            if (dfd.channelsCount()>0)
            suffixN = dfd.channels.first()->xMax;
            suffix = QString::number(suffixN);
        }

        //baseFileName.chop(3);
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
    }
    if (filtered.first().endsWith("uff")) {
        UffFileDescriptor dfd(filtered.first());
        dfd.read();
        QString suffix = QString::number(dfd.samplesCount() * dfd.xStep());

        //baseFileName.chop(3);
        QString uffFileName = destDir+"/"+baseFileName+"_"+suffix+".uff";

        int index = 0;
        if (QFile::exists(uffFileName)) {
            index++;
            while (QFile::exists(destDir+"/"+baseFileName+"_"+suffix+"("+QString::number(index)+").uff")) {
                index++;
            }
        }

        if (index>0) {
            uffFileName = destDir+"/"+baseFileName+"_"+suffix+"("+QString::number(index)+").uff";
        }

        QFile::rename(filtered.first(), uffFileName);
        newFiles << uffFileName;
    }
    newFiles_.removeAll(filtered.first());
}

QVector<float> getBlock(const QVector<float> &values, const quint32 blockSize, const quint32 stepBack, quint32 &block)
{
    QVector<float> output;
    if (block < values.size()) {
        output = values.mid(block, blockSize);
        block += blockSize - stepBack;
    }
    return output;
}

QVector<float> getBlock(uchar *mapped, quint32 mappedSize,
                        int channel, int chanBlockSize, int channelsCount,
                        const quint32 blockSize, const quint32 stepBack, quint32 &block)
{
    QVector<float> output;
    /*
     * i-й отсчет n-го канала имеет номер
     * n*ChanBlockSize + (i/ChanBlockSize)*ChanBlockSize*ChannelsCount+(i % ChanBlockSize)
     */

    if (block < mappedSize/sizeof(float)) {
        quint32 realBlockSize = qMin(blockSize, mappedSize/sizeof(float) - block);
        output.resize(realBlockSize);
        for (int i = 0; i<realBlockSize; ++i) {
            int blocki = block+i;
            qint64 sampleNumber = channel*chanBlockSize + int(blocki/chanBlockSize)*chanBlockSize*channelsCount
                                  +(blocki % chanBlockSize);
            output[i] = (float)(*(mapped+sampleNumber*sizeof(float)));
        }
        //output = values.mid(block, blockSize);
        block += realBlockSize - stepBack;
    }
    return output;
}

void average(QVector<double> &result, const QVector<double> &input, const Parameters &p, int averagesMade)
{DD;
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

void averageComplex(QVector<QPair<double,double> > &result,
                    const QVector<QPair<double,double> > &input, const Parameters &p, int averagesMade)
{DD;
    //int averagingType; //0 - линейное
    //1 - экспоненциальное
    //2 - хранение максимума

    // предполагаем, что result хранится в линейной шкале
    double rho=1.0 / p.averagesCount;

    for (int i=0; i<p.fCount; ++i) {
        // усреднение по максимуму
        if (p.averagingType==2) {//
            result[i].first = qMax(input[i].first, result[i].first);
            result[i].second = qMax(input[i].second, result[i].second);
        }
        else if (p.averagingType==0) {//линейное
            result[i].first = ((averagesMade-1)*result[i].first+input[i].first)/averagesMade;
            result[i].second = ((averagesMade-1)*result[i].second+input[i].second)/averagesMade;
        }
        else if (p.averagingType==1) {//экспоненциальное
            result[i].first = (1.0-rho)*result[i].first+rho*input[i].first;
            result[i].second = (1.0-rho)*result[i].second+rho*input[i].second;
        }
    }
}

void changeScale(QVector<double> &output, const Parameters &p)
{DD;
    const double t2 = p.threshold * p.threshold;
    if (p.scaleType > 0)
        for (int i=0; i<output.size(); ++i)
            output[i] = 10 * log10(output[i] / t2);
}

QString createUniqueFileName(const QString &tempFolderName, const QString &fileName, QString method,
                             bool saveAsUff)
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
    QString ext = saveAsUff?".uff":".dfd";
    return result+suffix+ext;
}

int stripNumberForBandwidth(double bandwidth, Parameters &p)
{DD;
    if (qAbs(bandwidth - p.bandWidth)<1.0e-3)
        return p.initialBandStripNumber;

    int val = p.initialBandStripNumber - int(log2(p.bandWidth/bandwidth));
    int result = qBound(0, val, 11);
    return result;
}

bool Converter::convert(DfdFileDescriptor *dfd, const QString &tempFolderName)
{DD;
    if (QThread::currentThread()->isInterruptionRequested()) {
        finalize();
        return false;
    }


    p.sampleRate = 1.0 / dfd->XStep;
    p.bandStrip = stripNumberForBandwidth(p.sampleRate / 2.56, p);
    p.fCount = qRound((double)p.bufferSize / 2.56);

    int averages = int(1.0 * dfd->channels[0]->samplesCount() / p.bufferSize / (1<<p.bandStrip) / (1.0 - p.overlap));
    if (p.averagesCount == -1) p.averagesCount = averages;
    else p.averagesCount = qMin(averages, p.averagesCount);
    if (dfd->channels[0]->samplesCount() % p.averagesCount !=0) p.averagesCount++;

    // Если опорный канал с таким номером в файле отсутствует, используем последний канал в файле
    if (p.baseChannel>=dfd->channelsCount()) p.baseChannel = dfd->channelsCount()-1;

    qDebug()<<p;

//    dfd->channels[0]->populateFloat();
//    OctaveFilterBank filtBank;
//    QVector<double> xVals;
//    auto result = filtBank.compute(dfd->channels[0]->floatValues, p.sampleRate, xVals);
//    qDebug()<<"result"<<result << "xVals"<<xVals;
//    return false;

    /** 1. Создаем конечный файл и копируем в него всю информацию из dfd */
    QString fileName = createUniqueFileName(tempFolderName, dfd->fileName(), p.methodDll, p.saveAsComplex);

    DfdFileDescriptor *newDfd = 0;
    if (!p.saveAsComplex) newDfd = p.method->createNewDfdFile(fileName, dfd, p);

    UffFileDescriptor *newUff = 0;
    if (p.saveAsComplex) newUff = p.method->createNewUffFile(fileName, dfd, p);

    QVector<double> xVals;

    if (p.baseChannel>=0) dfd->channels[p.baseChannel]->populateFloat();
    for (int i=0; i<dfd->channelsCount(); ++i) {
        dfd->channels[i]->populateFloat();

        if (QThread::currentThread()->isInterruptionRequested()) {
            delete newDfd;
            delete newUff;
            finalize();
            return false;
        }

        p.threshold = threshold(dfd->channels[i]->yName());

        QVector<double> spectrum(p.fCount);
        QVector<QPair<double,double> > spectrumComplex(p.fCount);
        quint32 block = 0;
        int averagesMade = 1;

        QVector<float> filtered;

        Filter filter(p);

        const int newBlockSize = p.bufferSize* (1<<p.bandStrip);

        const quint32 stepBack = quint32(1.0 * newBlockSize * p.overlap);
qDebug()<<"before methods";
        // Реализация методов
        //TODO: вынести их все в отдельные классы обработки
        if (p.method->id()==0) {//осциллограф
            spectrum.clear();
            while (1) {
                QVector<float> chunk = getBlock(dfd->channels[i]->floatValues, newBlockSize, stepBack, block);
                if (chunk.size() < p.bufferSize) break;
                filtered = filter.process(chunk);
                foreach(float val, filtered)
                spectrum << val;
            }
            spectrum.squeeze();
        }

        if (p.method->id()==1) {//спектр мощности
            while (1) {
                QVector<float> chunk = getBlock(dfd->channels[i]->floatValues, newBlockSize, stepBack, block);
//                QVector<float> chunk = getBlock(mapped, mappedSize,
//                                                i, dfd->channels[i]->ChanBlockSize,
//                                                dfd->channelsCount(),
//                                                newBlockSize, stepBack, block);
//                if (RawChannel *rawc = dynamic_cast<RawChannel*>(dfd->channels[i])) {
//                    for (int k=0; k < chunk.size(); ++k)
//                        chunk[k] = chunk[k]*rawc->coef1+rawc->coef2;
//                }

                if (chunk.size() < p.bufferSize) break;

                filtered = filter.process(chunk);
                applyWindow(filtered, p);
                QVector<double> out = powerSpectre(filtered, p);
                average(spectrum, out, p, averagesMade++);
                // контроль количества усреднений
                if (averagesMade >= p.averagesCount) break;
            }
            changeScale(spectrum, p);
        }

        if (p.method->id()==9 && !p.saveAsComplex) {//передаточная 1, модуль
            if (i == p.baseChannel) continue;

            quint32 baseBlock = 0;
            QVector<float> baseFiltered;
            Filter baseFilter(p);

            QVector<double> averagedBaseSpectre(p.fCount);
            QVector<double> averagedSpectre(p.fCount);

            while (1) {
                QVector<float> chunk1 = getBlock(dfd->channels[i]->floatValues, newBlockSize, stepBack, block);
                QVector<float> chunk2 = getBlock(dfd->channels[p.baseChannel]->floatValues, newBlockSize, stepBack, baseBlock);
                if (chunk1.size() < p.bufferSize || chunk2.size() < p.bufferSize) break;

                filtered = filter.process(chunk1);
                baseFiltered = baseFilter.process(chunk2);

                applyWindow(filtered, p);
                applyWindow(baseFiltered, p);
                QVector<double> baseautoSpectr = autoSpectre(baseFiltered, p);
                QVector<double> coSpectr = coSpectre(baseFiltered, filtered, p);

                average(averagedBaseSpectre, baseautoSpectr, p, averagesMade);
                average(averagedSpectre, coSpectr, p, averagesMade++);
                // контроль количества усреднений
                if (averagesMade >= p.averagesCount) break;
            }
            spectrum = transferFunctionH1(averagedBaseSpectre, averagedSpectre, p);
            if (p.scaleType > 0) {
                const double t2 = threshold(dfd->channels[p.baseChannel]->yName()) / p.threshold;
                for (int i=0; i<spectrum.size(); ++i)
                    spectrum[i] = 20 * log10(spectrum[i] * t2);
            }
        }

        if (p.method->id()==9 && p.saveAsComplex) {//передаточная 1, комплексные
            if (i == p.baseChannel) continue;

            quint32 baseBlock = 0;
            QVector<float> baseFiltered;
            Filter baseFilter(p);

            QVector<double> averagedBaseSpectre(p.fCount);
            QVector<QPair<double,double> > averagedSpectre(p.fCount);

            while (1) {
                QVector<float> chunk1 = getBlock(dfd->channels[i]->floatValues, newBlockSize, stepBack, block);
                QVector<float> chunk2 = getBlock(dfd->channels[p.baseChannel]->floatValues, newBlockSize, stepBack, baseBlock);
                if (chunk1.size() < p.bufferSize || chunk2.size() < p.bufferSize) break;

                filtered = filter.process(chunk1);
                baseFiltered = baseFilter.process(chunk2);

                applyWindow(filtered, p);
                applyWindow(baseFiltered, p);
                QVector<double> baseautoSpectr = autoSpectre(baseFiltered, p);
                QVector<QPair<double,double> > coSpectr = coSpectreComplex(baseFiltered, filtered, p);

                average(averagedBaseSpectre, baseautoSpectr, p, averagesMade);
                averageComplex(averagedSpectre, coSpectr, p, averagesMade++);
                // контроль количества усреднений
                if (averagesMade >= p.averagesCount) break;
            }
            spectrumComplex = transferFunctionH1Complex(averagedBaseSpectre, averagedSpectre, p);

            // нет смысла переводить в децибелы
//            const double t2 = threshold(dfd->channels[p.baseChannel]->yName()) / p.threshold;
//            for (int i=0; i<spectrum.size(); ++i)
//                spectrum[i] = 20 * log10(spectrum[i] * t2);
        }

        if (p.method->id()==18) {//октавный спектр
            OctaveFilterBank filtBank(p);

            spectrum = filtBank.compute(dfd->channels[i]->floatValues, p.sampleRate, xVals);
        }

        // Создаем канал и заполняем его
        if (newDfd) {
            newDfd->channels << p.method->createDfdChannel(newDfd, dfd, spectrum, p, i);
        }
        if (newUff) {
            Function *f = p.method->addUffChannel(newUff, dfd, p.fCount, p, i);
            if (p.saveAsComplex)
                f->valuesComplex = spectrumComplex;
            else
                f->values = spectrum;
        }

        dfd->channels[i]->floatValues.clear();

        emit tick();
        qDebug()<<"tick";
    }
    // подчищаем опорный канал
    if (p.baseChannel>=0 && p.baseChannel < dfd->channelsCount())
        dfd->channels[p.baseChannel]->floatValues.clear();

    if (newDfd) {
        if (p.method->id()==18 && !xVals.isEmpty()) {
            newDfd->channels.prepend(p.method->createDfdChannel(newDfd, dfd, xVals, p, 0));
            newDfd->channels.first()->ChanName = "ось X";
            newDfd->channels.first()->YName = "Гц";
            for (int i=0; i<newDfd->channelsCount(); ++i) {
                newDfd->channels[i]->channelIndex=i;
            }
        }
        newDfd->NumChans = newDfd->channels.size();
        newDfd->setSamplesCount(newDfd->channel(0)->samplesCount());
        newDfd->setChanged(true);
        newDfd->setDataChanged(true);
        newDfd->write();
        newDfd->writeRawFile();
        delete newDfd;
    }
    if (newUff) {
        newUff->setChanged(true);
        newUff->write();
        delete newUff;
    }
    return true;
}

void applyWindow(QVector<float> &values, const Parameters &p)
{DD;
    quint32 i=0;
    while (1) {
        for (int j=0; j<p.window.size(); j++) {
            if (i >= (quint32)values.size()) return;
            values[i] = values[i] * p.window[j];
            i++;
        }
    }
}

const double TwoPi = 6.283185307179586;

QVector<double> FFTAnalysis(const QVector<float> &AVal)
{DD;
    int Nvl = AVal.size();

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
    return Tmvl;
}

// возвращает спектр мощности 2*|complexSpectre|^2/N^2
QVector<double> powerSpectre(const QVector<float> &values, const Parameters &p)
{DD;
    QVector<double> output(p.bufferSize);
    QVector<double> complexSpectre = FFTAnalysis(values);

    int j=0;
    const int Nvl = values.size();
    for (int i = 0; i < Nvl; i++) {
        j = i * 2; output[Nvl - i - 1] = 2* (pow(complexSpectre[j],2) + pow(complexSpectre[j+1],2))/Nvl/Nvl;
    }

    output.resize(p.fCount);

    return output;
}

// возвращает взаимный спектр мощности |Y* Z|
QVector<double> coSpectre(const QVector<float> &values1, const QVector<float> &values2, const Parameters &p)
{DD;
    QVector<double> output(p.bufferSize);
    QVector<QPair<double, double> > compls = coSpectreComplex(values1, values2, p);
    for (int i=0; i< compls.size(); ++i)
        output[i] = sqrt(pow(compls[i].first,2) + pow(compls[i].second,2));

//    QVector<double> complexSpectre1 = FFTAnalysis(values1);
//    QVector<double> complexSpectre2 = FFTAnalysis(values2);


//    int j=0;
//    const int Nvl = values1.size();
//    for (int i = 0; i < Nvl; i++) {
//        j = i * 2;
//        double real = complexSpectre1[j]*complexSpectre2[j] + complexSpectre1[j+1]*complexSpectre2[j+1];
//        double imag = complexSpectre1[j]*complexSpectre2[j+1] - complexSpectre1[j+1]*complexSpectre2[j];
//        output[Nvl - i - 1] = sqrt(pow(real,2) + pow(imag,2));
//    }

    output.resize(p.fCount);

    return output;
}

// возвращает взаимный спектр мощности Y* Z (комплексный)
QVector<QPair<double, double> > coSpectreComplex(const QVector<float> &values1, const QVector<float> &values2, const Parameters &p)
{DD;
    QVector<QPair<double, double> > output(p.bufferSize);

    QVector<double> complexSpectre1 = FFTAnalysis(values1);
    QVector<double> complexSpectre2 = FFTAnalysis(values2);


    int j=0;
    const int Nvl = values1.size();
    for (int i = 0; i < Nvl; i++) {
        j = i * 2;
        double real = complexSpectre1[j]*complexSpectre2[j] + complexSpectre1[j+1]*complexSpectre2[j+1];
        double imag = complexSpectre1[j]*complexSpectre2[j+1] - complexSpectre1[j+1]*complexSpectre2[j];
        output[Nvl - i - 1].first = real;
        output[Nvl - i - 1].second = imag;
    }

    output.resize(p.fCount);

    return output;
}

// возвращает автоспектр сигнала |Y|^2
QVector<double> autoSpectre(const QVector<float> &values, const Parameters &p)
{DD;
    QVector<double> output(p.bufferSize);
    QVector<double> complexSpectre = FFTAnalysis(values);

    int j=0;
    const int Nvl = values.size();
    for (int i = 0; i < Nvl; i++) {
        j = i * 2;
        output[Nvl - i - 1] = complexSpectre[j]*complexSpectre[j] + complexSpectre[j+1]*complexSpectre[j+1];
    }

    output.resize(p.fCount);

    return output;
}

// возвращает передаточную функцию H1
//(амплитуду)
QVector<double> transferFunctionH1(const QVector<double> &values1, const QVector<double> &values2, const Parameters &p)
{DD;
    QVector<double> output(p.fCount);

   // QVector<double> coSpectre_ = coSpectre(values1, values2, p);
   // QVector<double> autoSpectre_ = autoSpectre(values1, p);

    for (int i=0; i<values2.size(); ++i) {
        output[i] = values2[i]/values1[i];
    }

    return output;
}

// возвращает передаточную функцию H1
//(комплексные значения)
QVector<QPair<double, double> > transferFunctionH1Complex(const QVector<double> &values1,
                                                          const QVector<QPair<double, double> > &values2, const Parameters &p)
{DD;
    QVector<QPair<double, double> > output(p.fCount);

   // QVector<double> coSpectre_ = coSpectre(values1, values2, p);
   // QVector<double> autoSpectre_ = autoSpectre(values1, p);

    for (int i=0; i<values2.size(); ++i) {
        output[i].first = values2[i].first/values1[i];
        output[i].second = values2[i].second/values1[i];
    }

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

        RawChannel *rawCh = dynamic_cast<RawChannel *>(dfd->channel(0));
        if (rawCh)
            p.bandStrip = stripNumberForBandwidth(rawCh->BandWidth, p);
        else
            p.bandStrip = stripNumberForBandwidth(qRound(1.0 / dfd->XStep / 2.56), p);

        spfFile << QString("ActChannel=%1").arg(p.activeChannel);
        spfFile << QString("BaseChannel=%1").arg(p.baseChannel);
        spfFile << "MinMax=*,*,*,*";
        spfFile << QString("AStrip=%1").arg(p.bandStrip);
        spfFile << QString("StepBack=%1").arg(floattohex(p.overlap));

        spfFile << "ShiftDat=0"; // TODO: добавить возможность устанавливать смещение
        // длина = число отсчетов в канале
        // TODO: добавить возможность устанавливать правую границу выборки
        quint32 NI;
        if (dfd->BlockSize>0) {
            NI = dfd->channels.at(p.activeChannel>0?p.activeChannel-1:0)->ChanBlockSize / dfd->BlockSize;
            NI *= dfd->samplesCount();
        }
        else
            NI = dfd->samplesCount();

        spfFile << QString("Duration=%1").arg(NI);
        spfFile << "TablName=";
        spfFile << QString("ViewPar=1,0,1,%1,1,1,0").arg(p.bandStrip);
        spfFile << "FacePos=0";
        spfFile << "Strob=0";

        spfFile << QString("[Panel%1\\Wind1\\Method]").arg(i);

        if (p.method)
            spfFile << p.method->methodSettings(dfd, p);
        else qDebug()<<"No method found";

    }
    return spfFile;
}

