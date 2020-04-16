#include "converter.h"

#include <QtCore>
#include "logging.h"
#include "fileformats/dfdfiledescriptor.h"
#include "fileformats/ufffile.h"
#include "converters.h"
#include "methods/windowing.h"
#include "averaging.h"
#include "resampler.h"
#include "methods/octavefilterbank.h"
#include "algorithms.h"
#include "fft.h"
#include "channelselector.h"
#include "framecutter.h"

Converter::Converter(QList<FileDescriptor *> &base, const Parameters &p_, QObject *parent) :
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
    bool someFilesAreUff = false;
    foreach (FileDescriptor *file, dataBase) {
        DfdFileDescriptor *dfd = dynamic_cast<DfdFileDescriptor *>(file);
        if (dfd && dfd->BlockSize==1) {
            blockSizeIs1 = true;
        }
        UffFileDescriptor *uff = dynamic_cast<UffFileDescriptor *>(file);
        if (uff) someFilesAreUff = true;
    }
    if (blockSizeIs1) {
        p.useDeepSea = true;
        emit message("Размер блока в одном из файлов равен 1. Это затрудняет"
                     " чтение данных, поэтому для расчета будет использован DeepSea.");
    }
    if (p.saveAsComplex && p.useDeepSea) {
        emit message("DeepSea не умеет сохранять комплексные результаты. Уберите одну из галок.");
        return;
    }
    if (someFilesAreUff) {
        emit message("Один из файлов имеет тип Uff. Использование DeepSea для расчетов будет блокировано.");
        p.useDeepSea = false;
    }

    if (!p.useDeepSea) {
        p.baseChannel--;
        foreach (FileDescriptor *file, dataBase) {
            emit message(QString("Подождите, пока идет расчет для файла\n%1").arg(file->fileName()));
            if (!convert(file, tempFolderName)) {
                emit message("Не удалось сконвертировать файл " + file->fileName());
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

    foreach (FileDescriptor *file, dataBase) {
        moveFilesFromTempDir(tempFolderName, file->fileName());
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

    QString baseFileName = QFileInfo(fileName).completeBaseName()+"_"+method;

    if (filtered.first().toLower().endsWith("dfd")) {
        DfdFileDescriptor dfd(filtered.first());
        dfd.read();
        int suffixN = dfd.samplesCount() * dfd.xStep();
        QString suffix = QString::number(suffixN);

        if (suffixN==0) {//третьоктава или файл с неодинаковым шагом по абсциссе
            if (dfd.channelsCount()>0) {
                dfd.channel(0)->populate();
                suffixN = dfd.channels.first()->xMax();
                suffix = QString::number(suffixN);
            }
        }


        QString dfdFileName = createUniqueFileName(destDir, baseFileName, suffix, "dfd");
        QString rawFileName = changeFileExt(dfdFileName, "raw");

        QFile::rename(filtered.first(), dfdFileName);
        QFile::rename(dfd.rawFileName, rawFileName);
        newFiles << dfdFileName;
    }
    if (filtered.first().endsWith("uff")) {
        UffFileDescriptor dfd(filtered.first());
        dfd.read();
        int suffixN = dfd.samplesCount() * dfd.xStep();
        QString suffix = QString::number(suffixN);

        if (suffixN==0) {//третьоктава или файл с неодинаковым шагом по абсциссе
            if (dfd.channelsCount()>0) {
                dfd.channel(0)->populate();
                suffixN = dfd.channels.first()->xMax();
                suffix = QString::number(suffixN);
            }
        }

        QString uffFileName = createUniqueFileName(destDir, baseFileName, suffix, "uff");
        QFile::rename(filtered.first(), uffFileName);
        newFiles << uffFileName;
    }
    newFiles_.removeAll(filtered.first());
}

QVector<float> getBlock(const QVector<double> &values, const int blockSize, const int stepBack, int &block)
{
    int realLength = blockSize;
    if (block+blockSize > values.length())
        realLength = values.length() - block;
    if (realLength <=0) return QVector<float>();
    QVector<float> output(realLength);
    if (block < values.size()) {
        std::copy(values.begin()+block, values.begin()+block+realLength, output.begin());
//        output = values.mid(block, blockSize);
        block += blockSize - stepBack;
    }
    return output;
}

/// Эта функция не используется, но содержит полезный алгоритм для определения положения каждого отсчета
/// файла для заданного номера канала, размера блока отсчетов, номера отсчета с нуля и величины перекрытия
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
        block += realBlockSize - stepBack;
    }
    return output;
}


void changeScale(QVector<double> &output, const Parameters &p)
{DD;
    const double t2 = p.threshold * p.threshold;
    if (p.scaleType > 0) {
        for (int i=0; i<output.size(); ++i)
            output[i] = 10 * log10(output[i] / t2);}
}

int stripNumberForBandwidth(double bandwidth, Parameters &p)
{DD;
    if (qAbs(bandwidth - p.bandWidth)<1.0e-3)
        return p.initialBandStripNumber;

    int val = p.initialBandStripNumber - int(log2(p.bandWidth/bandwidth));
    int result = qBound(0, val, 11);
    return result;
}

bool Converter::convert(FileDescriptor *file, const QString &tempFolderName)
{DD;
    if (QThread::currentThread()->isInterruptionRequested()) {
        finalize();
        return false;
    }
    if (file->channelsCount()==0) return false;

    p.sampleRate = 1.0 / file->xStep();
    if (p.bandWidth == 0) p.bandWidth = qRound(1.0 / file->xStep() / 2.56);
    if (p.bufferSize == 0) p.bufferSize = p.sampleRate;

    p.bandStrip = stripNumberForBandwidth(p.sampleRate / 2.56, p);
    p.fCount = qRound((double)p.bufferSize / 2.56);

    int factor = 1<<p.bandStrip;

    const int samplesCount = file->channel(0)->samplesCount();
    int averages = int(1.0 * samplesCount / p.bufferSize / factor / (1.0 - p.overlap));
    if (p.averagesCount == -1) p.averagesCount = averages;
    else p.averagesCount = qMin(averages, p.averagesCount);
    if (samplesCount % p.averagesCount !=0) p.averagesCount++;

    // Если опорный канал с таким номером в файле отсутствует, используем последний канал в файле
    if (p.baseChannel>=file->channelsCount()) p.baseChannel = file->channelsCount()-1;

//    qDebug()<<p;

    /** 1. Создаем конечный файл и копируем в него всю информацию из dfd */
    QString method = p.method->methodDll();
    method.chop(4);
    QString fileName = createUniqueFileName(tempFolderName, file->fileName(), method,
                                            p.saveAsComplex ? "uff":"dfd", true);
    DfdFileDescriptor *newDfd = 0;
    if (!p.saveAsComplex) newDfd = p.method->createNewDfdFile(fileName, file, p);

    UffFileDescriptor *newUff = 0;
    if (p.saveAsComplex) newUff = p.method->createNewUffFile(fileName, file, p);

    QVector<double> xVals;

    bool baseWasPopulated = true; // do nothing afterwards
    if (p.baseChannel>=0) {
        baseWasPopulated = file->channel(p.baseChannel)->populated();
        file->channel(p.baseChannel)->populate();
    }

    ChannelSelector selector(p.channelFilter);


    for (int i=0; i<file->channelsCount(); ++i) {
//        QTime time;
//        time.start();
        if (!selector.includes(i)) continue;
        if (i == p.baseChannel) continue;

        bool wasPopulated = file->channel(i)->populated();
        file->channel(i)->populate();

        if (QThread::currentThread()->isInterruptionRequested()) {
            delete newDfd;
            delete newUff;
            finalize();
            return false;
        }

        int units = DataHolder::UnitsLinear;

        p.threshold = threshold(file->channel(i)->yName());

        QVector<double> spectrum;
        QVector<cx_double> spectrumComplex;
        int block = 0;
        const int newBlockSize = p.bufferSize * factor;

        const int stepBack = int(1.0 * newBlockSize * p.overlap);

        // Реализация методов
        //TODO: вынести их все в отдельные классы обработки
        if (p.method->id()==0) {//осциллограф
            spectrum.clear();
            int buffer = 1024;
            Resampler filter(factor, buffer);
            QVector<float> filtered;
            while (1) {
                QVector<float> chunk = getBlock(file->channel(i)->yValues(), buffer, stepBack, block);
                if (chunk.size() < buffer)
                    filter.setLastChunk();
                filtered = filter.process(chunk);
                if (filtered.isEmpty()) break;
                foreach(float val, filtered)
                    spectrum << val;
            }
            // дополняем нулями вектор, чтобы длина в точности равнялась
            // длине исходной записи, деленной на factor
            spectrum.append(QVector<double>(file->channel(i)->yValues().size()/factor - spectrum.size()) );
            spectrum.squeeze();
        }

        if (p.method->id()==1) {//спектр мощности
            units = DataHolder::UnitsQuadratic;
            FrameCutter sampling;
            sampling.setType(p.overlap==0?FrameCutter::Continuous:FrameCutter::Overlap);
            sampling.setBlockSize(newBlockSize);
            sampling.setDelta(1.0 * newBlockSize * p.overlap);
            sampling.setSource(file->channel(i)->yValues());

            Resampler filter(factor, p.bufferSize);
            QVector<double> filtered;

            Windowing window(p.bufferSize, p.windowType, p.windowPercent);
            Averaging averaging(p.averagingType+1, p.averagesCount);

            bool ok;
            while (1) {
                QVector<double> chunk = sampling.get(&ok);
                if (!ok) break;

                if (chunk.size() < p.bufferSize) filter.setLastChunk();

                filtered = filter.process(chunk);
                window.applyTo(filtered);

                if (p.saveAsComplex) {
                    QVector<cx_double> out = spectreFunction(filtered, p.fCount);
                    averaging.average(out);
                }
                else {
                    QVector<double> out = powerSpectre(filtered, p.fCount);
                    averaging.average(out);
                }

                // контроль количества усреднений
                if (averaging.averagingDone()) break;
            }
            if (p.saveAsComplex) {
                spectrumComplex = averaging.getComplex();
            }
            else {
                spectrum = averaging.get();
                changeScale(spectrum, p);
            }
        }

        if (p.method->id()==9) {//передаточная 1
            QVector<double> baseFiltered;
            QVector<double> filtered;

            FrameCutter sampling1;
            sampling1.setType(p.overlap==0?FrameCutter::Continuous:FrameCutter::Overlap);
            sampling1.setBlockSize(newBlockSize);
            sampling1.setDelta(1.0 * newBlockSize * p.overlap);
            sampling1.setSource(file->channel(i)->yValues());


            FrameCutter sampling2;
            sampling2.setType(p.overlap==0?FrameCutter::Continuous:FrameCutter::Overlap);
            sampling2.setBlockSize(newBlockSize);
            sampling2.setDelta(1.0 * newBlockSize * p.overlap);
            sampling2.setSource(file->channel(p.baseChannel)->yValues());

            Resampler filter(factor, p.bufferSize);
            Resampler baseFilter(factor, p.bufferSize);
            Windowing window(p.bufferSize, p.windowType, p.windowPercent);
            Windowing windowBase(p.bufferSize, p.forceWindowType, p.forceWindowPercent);
            Averaging averagingBase(p.averagingType+1, p.averagesCount);
            Averaging averaging(p.averagingType+1, p.averagesCount);



            while (1) {
                bool ok;
                QVector<double> chunk1 = sampling1.get(&ok);
                if (!ok) break;
                QVector<double> chunk2 = sampling2.get(&ok);
                if (!ok) break;
//                QVector<float> chunk1 = getBlock(file->channel(i)->yValues(), newBlockSize, stepBack, block);
//                QVector<float> chunk2 = getBlock(file->channel(p.baseChannel)->yValues(), newBlockSize, stepBack, baseBlock);
                if (chunk1.size() < p.bufferSize || chunk2.size() < p.bufferSize) break;

                filtered = filter.process(chunk1);
                baseFiltered = baseFilter.process(chunk2);

                window.applyTo(filtered);
                windowBase.applyTo(baseFiltered);
                QVector<double> baseautoSpectre = autoSpectre(baseFiltered, p.fCount);
                QVector<cx_double> coSpectr = covariantSpectre(baseFiltered, filtered, p.fCount);

                averagingBase.average(baseautoSpectre);
                averaging.average(coSpectr);
                // контроль количества усреднений
                if (averagingBase.averagingDone()) break;
            }
            spectrumComplex = transferFunction(averagingBase.get(), averaging.getComplex());

            //fixing units of measurement
            double convertF = convertFactor(file->channel(i)->yName()) /
                              convertFactor(file->channel(p.baseChannel)->yName());
            for (int sample = 0; sample < spectrumComplex.size(); ++sample)
                spectrumComplex[sample] *= convertF;

            if (!p.saveAsComplex) {
                spectrum = absolutes(spectrumComplex);
                if (p.scaleType > 0) {
                    //в DeepSea ошибочно используется пороговое значение для виброускорения. Убираем порог полностью,
                    //чтобы результаты совпадали с Test.Xpress
                    const double t2 = 1.0;//threshold(file->channel(p.baseChannel)->yName()) / p.threshold;
                    for (int i=0; i<spectrum.size(); ++i)
                        spectrum[i] = 20 * log10(spectrum[i] * t2);
                }
            }
        }

        if (p.method->id()==18) {//октавный спектр
            OctaveFilterBank filtBank(p);
            spectrum = filtBank.compute(file->channel(i)->yValues(), xVals);
        }

        // Создаем канал и заполняем его
        Channel *ch = 0;
        if (newDfd) {
            ch = p.method->createDfdChannel(newDfd, file, spectrum, p, i);
            ch->data()->setYValuesUnits(units);
        }
        if (newUff) {
            ch = p.method->addUffChannel(newUff, file, p.fCount, p, i);
            ch->data()->setThreshold(p.threshold);
            ch->data()->setYValuesUnits(units);
            if (p.saveAsComplex)
                ch->data()->setYValues(spectrumComplex);
            else
                ch->data()->setYValues(spectrum, p.scaleType > 0 ? DataHolder::YValuesAmplitudesInDB : DataHolder::YValuesAmplitudes);
        }
        if (!xVals.isEmpty() && ch) ch->data()->setXValues(xVals);

        if (!wasPopulated) {
            file->channel(i)->clear();
        }
        emit tick();
//        qDebug()<<time.elapsed();
    }
    // подчищаем опорный канал
    if (!baseWasPopulated) {
        file->channel(p.baseChannel)->clear();
    }

    if (newDfd) {
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

// возвращает спектр
QVector<cx_double> spectreFunction(const QVector<double> &values, int outputSize)
{DD;
    QVector<cx_double> complexSpectre = Fft::compute(values);
    const int Nvl = complexSpectre.size();
    const double factor = 2.0 / Nvl / Nvl;

    complexSpectre.resize(outputSize);
    for (int i = 0; i < outputSize; i++) {
        complexSpectre[i] *= factor;
    }
    return complexSpectre;
}

// возвращает спектр мощности 2*|complexSpectre|^2/N^2
QVector<double> powerSpectre(const QVector<double> &values, int N)
{DD;
    QVector<cx_double> complexSpectre = Fft::compute(values);

    const int Nvl = complexSpectre.size();
    const double factor = 2.0 / Nvl / Nvl;

    QVector<double> output(N);

    for (int i = 0; i < N; i++) {
        output[i] = factor * std::norm(complexSpectre[i]);
    }

    return output;
}

// возвращает взаимный спектр мощности Y * Z (комплексный)
QVector<cx_double> covariantSpectre(const QVector<double> &baseValues, const QVector<double> &values, int outputSize)
{
    QVector<cx_double> output(outputSize);

    QVector<cx_double> complexSpectre1 = Fft::compute(baseValues);
    QVector<cx_double> complexSpectre2 = Fft::compute(values);

    for (int i = 0; i < outputSize; i++) {
        output[i] = std::conj(complexSpectre1[i]) * complexSpectre2[i];
    }

    return output;
}

// возвращает автоспектр сигнала |Y|^2
QVector<double> autoSpectre(const QVector<double> &values, int outputSize)
{DD;
    QVector<double> output(outputSize);
    QVector<cx_double> complexSpectre = Fft::compute(values);

    for (int i = 0; i < outputSize; i++) {
        output[i] = std::norm(complexSpectre[i]);
    }

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

QVector<cx_double> transferFunction(const QVector<double> &values1, const QVector<cx_double> &values2)
{
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
        FileDescriptor *dfd = dataBase.at(i);
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
        for (int i=1; i<=dfd->channelsCount(); ++i) channelsList << QString::number(i);

        spfFile << "Channels="+channelsList.join(',');

        RawChannel *rawCh = dynamic_cast<RawChannel *>(dfd->channel(0));
        if (rawCh)
            p.bandStrip = stripNumberForBandwidth(rawCh->BandWidth, p);
        else
            p.bandStrip = stripNumberForBandwidth(qRound(1.0 / dfd->xStep() / 2.56), p);

        spfFile << "ActChannel=1";
        spfFile << QString("BaseChannel=%1").arg(p.baseChannel);
        spfFile << "MinMax=*,*,*,*";
        spfFile << QString("AStrip=%1").arg(p.bandStrip);
        spfFile << QString("StepBack=%1").arg(floattohex(p.overlap));

        spfFile << "ShiftDat=0"; // TODO: добавить возможность устанавливать смещение
        // длина = число отсчетов в канале
        // TODO: добавить возможность устанавливать правую границу выборки
        int NI = dfd->samplesCount();

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







