#include "converter.h"

#include <QtCore>
#include "logging.h"
#include "dfdfiledescriptor.h"
#include "ufffile.h"
#include "converters.h"
#include "windowing.h"
#include "averaging.h"
#include "samplerate.h"
#include "octavefilterbank.h"
#include "algorithms.h"

class Resampler
{
public:
    Resampler(int factor, int bufferSize) : factor(factor), bufferSize(bufferSize)
    {
        src_data.src_ratio = 1.0 / factor;

        src_state = src_new(2, 1, &_error);
        src_data.end_of_input = 0;

        const int newBlockSize = bufferSize * factor;

        filterOut.resize(newBlockSize+100);
        src_data.output_frames = filterOut.size();
        src_data.data_out = filterOut.data();
    }

    virtual ~Resampler()
    {
        if (src_state)
            src_delete(src_state);
    }


    QVector<float> process(QVector<float> &chunk)
    {
        if (factor > 1) {//do filtration
            src_data.data_in = chunk.data();
            src_data.input_frames = chunk.size();

            _error = src_process(src_state, &src_data);
            return filterOut.mid(0, bufferSize);
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

    int factor; // 1, 2, 4, 8, 16 etc.
    int bufferSize;
    //Parameters p;
};

Converter::Converter(QList<DfdFileDescriptor *> &base, const Parameters &p_, QObject *parent) :
    QObject(parent), dataBase(base), p(p_), process(0)
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
            //and releasing memory (since Qt 5.7)
            c->floatValues.squeeze();
        }
    }

    qDebug()<<"End converting"<<QDateTime::currentDateTime().time();
    emit message(QString("Расчет закончен в %1").arg(QDateTime::currentDateTime().time().toString()));
    emit finished();
}

void Converter::moveFilesFromTempDir(const QString &tempFolderName, QString fileName)
{DD;
    QString destDir = QFileInfo(fileName).canonicalPath();
    QString method = p.method->methodDll();
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
DebugPrint(filtered);
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

        DebugPrint(baseFileName);
        QString dfdFileName = createUniqueFileName(destDir, baseFileName, suffix, "dfd");
        QString rawFileName = changeFileExt(dfdFileName, "raw");
        DebugPrint(dfdFileName);
        DebugPrint(rawFileName);

        QFile::rename(filtered.first(), dfdFileName);
        QFile::rename(dfd.attachedFileName(), rawFileName);
        newFiles << dfdFileName;
    }
    if (filtered.first().endsWith("uff")) {
        UffFileDescriptor dfd(filtered.first());
        dfd.read();
        QString suffix = QString::number(dfd.samplesCount() * dfd.xStep());

        QString uffFileName = createUniqueFileName(destDir, baseFileName, suffix, "dfd");
        QFile::rename(filtered.first(), uffFileName);
        newFiles << uffFileName;
    }
    newFiles_.removeAll(filtered.first());
}

QVector<float> getBlock(const QVector<float> &values, const int blockSize, const int stepBack, int &block)
{
    QVector<float> output;
    if (block < values.size()) {
        output = values.mid(block, blockSize);
        block += blockSize - stepBack;
    }
    return output;
}

QVector<float> getBlock(uchar *mapped, quint64 mappedSize,
                        int channel, int chanBlockSize, int channelsCount,
                        int blockSize, int stepBack, int &block)
{
    QVector<float> output;
    /*
     * i-й отсчет n-го канала имеет номер
     * n*ChanBlockSize + (i/ChanBlockSize)*ChanBlockSize*ChannelsCount+(i % ChanBlockSize)
     */

    if (block < int(mappedSize/sizeof(float))) {
        int realBlockSize = qMin(blockSize, int(mappedSize/sizeof(float) - block));
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


void changeScale(QVector<double> &output, const Parameters &p)
{DD;
    const double t2 = p.threshold * p.threshold;
    if (p.scaleType > 0)
        for (int i=0; i<output.size(); ++i)
            output[i] = 10 * log10(output[i] / t2);
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

//    qDebug()<<p;

    /** 1. Создаем конечный файл и копируем в него всю информацию из dfd */
    QString method = p.method->methodDll();
    method.chop(4);
    QString fileName = createUniqueFileName(tempFolderName, dfd->fileName(), method,
                                            p.saveAsComplex ? "uff":"dfd", true);
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

        QVector<double> spectrum;
        QVector<cx_double> spectrumComplex;
        int block = 0;
        const int newBlockSize = p.bufferSize* (1<<p.bandStrip);

        const int stepBack = int(1.0 * newBlockSize * p.overlap);

        // Реализация методов
        //TODO: вынести их все в отдельные классы обработки
        if (p.method->id()==0) {//осциллограф
            spectrum.clear();
            Resampler filter(1<<p.bandStrip, p.bufferSize);
            QVector<float> filtered;
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
            Resampler filter(1<<p.bandStrip, p.bufferSize);
            QVector<float> filtered;

            Windowing window(p.bufferSize, p.windowType);
            Averaging averaging(p.averagingType, p.averagesCount);

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
                window.applyTo(filtered);

                QVector<double> out = powerSpectre(filtered, p.fCount);
                averaging.average(out);
                // контроль количества усреднений
                if (averaging.averagingDone()) break;
            }
            spectrum = averaging.get();
            changeScale(spectrum, p);
        }

        if (p.method->id()==9) {//передаточная 1
            if (i == p.baseChannel) continue;

            int baseBlock = 0;
            QVector<float> baseFiltered;
            QVector<float> filtered;
            Resampler filter(1<<p.bandStrip, p.bufferSize);
            Resampler baseFilter(1<<p.bandStrip, p.bufferSize);
            Windowing window(p.bufferSize, p.windowType);

            Averaging averagingBase(p.averagingType, p.averagesCount);
            Averaging averaging(p.averagingType, p.averagesCount);

            while (1) {
                QVector<float> chunk1 = getBlock(dfd->channels[i]->floatValues, newBlockSize, stepBack, block);
                QVector<float> chunk2 = getBlock(dfd->channels[p.baseChannel]->floatValues, newBlockSize, stepBack, baseBlock);
                if (chunk1.size() < p.bufferSize || chunk2.size() < p.bufferSize) break;

                filtered = filter.process(chunk1);
                baseFiltered = baseFilter.process(chunk2);

                window.applyTo(filtered);
                window.applyTo(baseFiltered);
                QVector<double> baseautoSpectre = autoSpectre(baseFiltered, p.fCount);
                QVector<cx_double> coSpectr = covariantSpectre(baseFiltered, filtered, p.fCount);

                averagingBase.average(baseautoSpectre);
                averaging.average(coSpectr);
                // контроль количества усреднений
                if (averagingBase.averagingDone()) break;
            }
            spectrumComplex = transferFunction(averagingBase.getComplex(), averaging.getComplex());

            if (!p.saveAsComplex) {
                spectrum = absolutes(spectrumComplex);
                if (p.scaleType > 0) {
                    const double t2 = threshold(dfd->channels[p.baseChannel]->yName()) / p.threshold;
                    for (int i=0; i<spectrum.size(); ++i)
                        spectrum[i] = 20 * log10(spectrum[i] * t2);
                }
            }
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
        //and releasing memory (since Qt 5.7)
        dfd->channels[i]->floatValues.squeeze();
        emit tick();
    }
    // подчищаем опорный канал
    if (p.baseChannel>=0 && p.baseChannel < dfd->channelsCount()) {
        dfd->channels[p.baseChannel]->floatValues.clear();
        dfd->channels[p.baseChannel]->floatValues.squeeze();
    }

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

QVector<cx_double> fft(const QVector<float> &AVal)
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

    QVector<cx_double> result = QVector<cx_double>(Nvl);
    for (i = 0; i < n; i+=2) {
        result[i] = {Tmvl[i], Tmvl[i+1]};
    }
    return result;
}

// возвращает спектр мощности 2*|complexSpectre|^2/N^2
QVector<double> powerSpectre(const QVector<float> &values, int outputSize)
{DD;
    QVector<cx_double> complexSpectre = fft(values);

    const int Nvl = complexSpectre.size();
    const double factor = 2.0 / Nvl / Nvl;

    QVector<double> output(Nvl);

    for (int i = 0; i < Nvl; i++) {
        output[Nvl - i - 1] = factor * std::norm(complexSpectre[i]);
    }
    output.resize(outputSize);

    return output;
}

// возвращает взаимный спектр мощности Y * Z (комплексный)
QVector<cx_double> covariantSpectre(const QVector<float> &values1, const QVector<float> &values2, int outputSize)
{
    QVector<cx_double> output(values1.size());

    QVector<cx_double> complexSpectre1 = fft(values1);
    QVector<cx_double> complexSpectre2 = fft(values2);

    const int Nvl = values1.size();
    for (int i = 0; i < Nvl; i++) {
        output[Nvl - i - 1] = complexSpectre1[i] * complexSpectre2[i];
    }

    output.resize(outputSize);
    return output;
}

// возвращает автоспектр сигнала |Y|^2
QVector<double> autoSpectre(const QVector<float> &values, int outputSize)
{DD;
    QVector<double> output(values.size());
    QVector<cx_double> complexSpectre = fft(values);

    const int Nvl = values.size();
    for (int i = 0; i < Nvl; i++) {
        output[Nvl - i - 1] = std::norm(complexSpectre[i]);
    }

    output.resize(outputSize);

    return output;
}

// возвращает передаточную функцию H1 = values2 ./ values1
//(комплексные значения)
QVector<cx_double> transferFunction(const QVector<cx_double> &values1, const QVector<cx_double> &values2)
{DD;
    const int size = qMin(values1.size(), values2.size());
    QVector<cx_double> output(size);

    for (int i=0; i<size; ++i) {
        output[i] = values2[i]/values1[i];
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
                   .arg(p.method->panelType()) //однооконная графическая панель
                   .arg(i+1);
        spfFile << "tVal=0"; // отображение величин: степенное

        spfFile << QString("[Panel%1\\Wind1]").arg(i); // параметры окна
        spfFile << QString("WinNm=%1").arg(p.method->methodName()); // описание окна
        spfFile << QString("ProcMethod=%1,%2")
                   .arg(p.method->methodDll())
                   .arg(p.method->dataType());
        spfFile << QString("FileNm=%1").arg(QDir::toNativeSeparators(dfd->fileName()));

        QStringList channelsList;
        for (int i=1; i<=dfd->channels.size(); ++i) channelsList << QString::number(i);

        spfFile << "Channels="+channelsList.join(',');

        RawChannel *rawCh = dynamic_cast<RawChannel *>(dfd->channel(0));
        if (rawCh)
            p.bandStrip = stripNumberForBandwidth(rawCh->BandWidth, p);
        else
            p.bandStrip = stripNumberForBandwidth(qRound(1.0 / dfd->XStep / 2.56), p);

        spfFile << "ActChannel=1";
        spfFile << QString("BaseChannel=%1").arg(p.baseChannel);
        spfFile << "MinMax=*,*,*,*";
        spfFile << QString("AStrip=%1").arg(p.bandStrip);
        spfFile << QString("StepBack=%1").arg(floattohex(p.overlap));

        spfFile << "ShiftDat=0"; // TODO: добавить возможность устанавливать смещение
        // длина = число отсчетов в канале
        // TODO: добавить возможность устанавливать правую границу выборки
        int NI;
        if (dfd->BlockSize>0) {
            NI = dfd->channels.at(0)->ChanBlockSize / dfd->BlockSize;
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




