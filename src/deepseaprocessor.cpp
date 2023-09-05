#include "deepseaprocessor.h"

#include <QtCore>
#include "logging.h"
#include "fileformats/dfdfiledescriptor.h"
#include "fileformats/ufffile.h"
#include "methods/windowing.h"
#include "methods/averaging.h"
#include "methods/resampler.h"
#include "methods/octavefilterbank.h"
#include "algorithms.h"
#include "methods/fft.h"
#include "methods/channelselector.h"
#include "methods/framecutter.h"
#include "unitsconverter.h"
#include "settings.h"

DeepseaProcessor::DeepseaProcessor(const QList<FileDescriptor *> &base, const Parameters &p_, QObject *parent) :
    QObject(parent), dataBase(base), p(p_), process(nullptr)
{DD;
    stop_ = false;
}

DeepseaProcessor::~DeepseaProcessor()
{DD;
    if (process) {
        process->kill();
        process->deleteLater();
    }
}

void DeepseaProcessor::stop()
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
            firstChild = GetWindow(firstChild, GW_HWNDNEXT);
        }
    }

    return TRUE;
}

void DeepseaProcessor::start()
{DD;
    dt = QDateTime::currentDateTime();
    LOG(INFO)<<"Start converting"<<dt.time().toString();
    emit message(QString("Запуск расчета: %1").arg(dt.time().toString()));

    QDir d;
    if (!d.exists("C:/DeepSeaBase-temp"))
        d.mkdir("C:/DeepSeaBase-temp");

    QTemporaryDir tempDir("C:\\DeepSeaBase-temp\\temp-XXXXXX");
    tempDir.setAutoRemove(true);
    tempFolderName = tempDir.path();

    bool someFilesAreUff = false;
    for (FileDescriptor *file: dataBase) {
        auto dfd = dynamic_cast<DfdFileDescriptor *>(file);
        if (!dfd) {
            someFilesAreUff = true;
            break;
        }
    }

    if (p.saveAsComplex) {
        emit message("DeepSea не умеет сохранять комплексные результаты.");
        return;
    }
    if (someFilesAreUff) {
        emit message("Не все файлы имеют формат DFD.");
        return;
    }

    emit message("Подождите, пока работает DeepSea...\n"
                 "Не забудьте закрыть DeepSea, когда она закончит расчеты");
    QStringList spfFile = getSpfFile(tempFolderName);
    QTemporaryFile file(QDir::tempPath()+"/spffile_XXXXXX.spf");
    if (file.open()) {
        QTextStream out(&file);
        out.setCodec("Windows-1251");
        foreach (QString s, spfFile) out << s << endl;
        file.close();
    }
    else return;

    temporaryFiles->add(file.fileName());

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

    process->start("C:\\Program Files (x86)\\DeepSea\\DeepSea.exe",arguments);

    auto timer = new QTimer(this);
    const int deepseaTimer = 1000;
    timer->setInterval(deepseaTimer);
    connect(timer, SIGNAL(timeout()), this, SLOT(processTimer()));
    timer->start();

    q.exec();
    timer->stop();


    const int code = process->exitCode();
    if (code != 0) {
        emit message("DeepSea завершился с ошибкой!");
    }

    finalize();
}

void DeepseaProcessor::processTimer()
{
    EnumWindows(enumWindowsProc, 0L);
}

void DeepseaProcessor::finalize()
{DD;
    QFileInfoList newFilesList = QDir(tempFolderName).entryInfoList(QStringList()<<"*.dfd",QDir::Files);
    newFilesList.append(QDir(tempFolderName).entryInfoList(QStringList()<<"*.uff",QDir::Files));


    for (const QFileInfo &newFile: newFilesList) {
        if (newFile.birthTime()>dt || newFile.lastModified()>dt)
            newFiles_ << newFile.canonicalFilePath();
    }

    for (FileDescriptor *file: dataBase) {
        moveFilesFromTempDir(tempFolderName, file->fileName());
    }

    LOG(INFO)<<"End converting"<<QDateTime::currentDateTime().time().toString();
    emit message(QString("Расчет закончен в %1").arg(QDateTime::currentDateTime().time().toString()));
    emit finished();
}

void DeepseaProcessor::moveFilesFromTempDir(const QString &tempFolderName, const QString &fileName)
{DD;
    QString destDir = QFileInfo(fileName).canonicalPath();
    QString method = p.method->methodDll();
    method.chop(4);
    QString filter=QString("%1/%2_%3")
                   .arg(tempFolderName, QFileInfo(fileName).completeBaseName(), method);
    filter.replace("\\","/");
    QStringList filtered = newFiles_.filter(filter);

    if (filtered.isEmpty()) {
        filter=QString("%1/%2_%3")
                       .arg(tempFolderName, QFileInfo(fileName).baseName(), method);
        filter.replace("\\","/");
        filtered = newFiles_.filter(filter);
    }

    if (filtered.isEmpty()) return;

    QString baseFileName = QFileInfo(fileName).completeBaseName()+"_"+method;

    if (filtered.constFirst().toLower().endsWith("dfd")) {
        DfdFileDescriptor dfd(filtered.constFirst());
        dfd.read();
        auto suffixN = static_cast<int>(dfd.samplesCount() * dfd.xStep());
        QString suffix = QString::number(suffixN);

        if (suffixN==0) {//третьоктава или файл с неодинаковым шагом по абсциссе
            if (dfd.channelsCount()>0) {
                dfd.channel(0)->populate();
                suffixN = dfd.channels.constFirst()->data()->xMax();
                suffix = QString::number(suffixN);
            }
        }


        QString dfdFileName = createUniqueFileName(destDir, baseFileName, suffix, "dfd");
        QString rawFileName = changeFileExt(dfdFileName, "raw");

        QFile::rename(filtered.constFirst(), dfdFileName);
        QFile::rename(dfd.rawFileName, rawFileName);
        newFiles << dfdFileName;
    }
    newFiles_.removeAll(filtered.constFirst());
}

/// Эта функция не используется, но содержит полезный алгоритм для определения положения каждого отсчета
/// файла для заданного номера канала, размера блока отсчетов, номера отсчета с нуля и величины перекрытия
//QVector<float> getBlock(uchar *mapped, quint64 mappedSize,
//                        int channel, int chanBlockSize, int channelsCount,
//                        int blockSize, int stepBack, int &block)
//{
//    QVector<float> output;
//    /*
//     * i-й отсчет n-го канала имеет номер
//     * n*ChanBlockSize + (i/ChanBlockSize)*ChanBlockSize*ChannelsCount+(i % ChanBlockSize)
//     */

//    if (block < int(mappedSize/sizeof(float))) {
//        int realBlockSize = qMin(blockSize, int(mappedSize/sizeof(float) - block));
//        output.resize(realBlockSize);
//        for (int i = 0; i<realBlockSize; ++i) {
//            int blocki = block+i;
//            qint64 sampleNumber = channel*chanBlockSize + int(blocki/chanBlockSize)*chanBlockSize*channelsCount
//                                  +(blocki % chanBlockSize);
//            output[i] = (float)(*(mapped+sampleNumber*sizeof(float)));
//        }
//        block += realBlockSize - stepBack;
//    }
//    return output;
//}

int stripNumberForBandwidth(double bandwidth, Parameters &p)
{DD;
    if (qAbs(bandwidth - p.bandWidth)<1.0e-3)
        return p.initialBandStripNumber;

    int val = p.initialBandStripNumber - int(log2(p.bandWidth/bandwidth));
    int result = qBound(0, val, 11);
    return result;
}

QStringList DeepseaProcessor::getSpfFile(const QString &dir)
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
    spfFile << "SoDatDir=C:\\Program Files (x86)\\DeepSea\\Data";
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
        spfFile << QString("StepBack=%1").arg(floattohex(float(p.overlap)));

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
        else
            LOG(WARNING)<<"No method found";
    }
    return spfFile;
}







