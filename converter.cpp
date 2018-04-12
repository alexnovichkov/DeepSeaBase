#include "converter.h"

#include <QtCore>
#include "logging.h"
#include "dfdfiledescriptor.h"
#include "ufffile.h"
#include "converters.h"
#include "windowing.h"
#include "samplerate.h"

class Filter
{
public:
    Filter(const Parameters &p) : p(p)
    {
        src_data.src_ratio = 1.0 / (1 << p.bandStrip);

        src_state = src_new(2, 1, &_error);
        src_data.end_of_input = 0;

        const int newBlockSize = p.blockSize * (1<<p.bandStrip);

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
            return filterOut.mid(0, p.blockSize);
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
    p.fCount = qRound((double)p.blockSize / 2.56);
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
        emit message("Размер блока в одном из файлов равен 1, для расчета будет использован DeepSea.");
    }
    if (p.saveAsComplex && p.useDeepSea) {
        emit message("DeepSea не умеет сохранять комплексные результаты. Уберите одну из галок");
        return;
    }

    if (!p.useDeepSea) {
        p.baseChannel--;
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
    EnumWindows(enumWindowsProc, NULL);
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

DfdFileDescriptor *Converter::createNewDfdFile(const QString &fileName, DfdFileDescriptor *dfd, Parameters &p)
{DD;
    DfdFileDescriptor *newDfd = new DfdFileDescriptor(fileName);

    newDfd->fillPreliminary(dataTypefromDfdDataType(DfdDataType(p.method->dataType())));
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
    newDfd->process->data.append({"Wind", p.windowDescription()});
    newDfd->process->data.append({"TypeAver", averaging(p.averagingType)});
    newDfd->process->data.append({"pTime","(0000000000000000)"});

    // rest
    newDfd->XName = "Гц";
    const double newSampleRate = p.sampleRate / pow(2.0, p.bandStrip);
    newDfd->XStep = newSampleRate / p.blockSize;
    newDfd->XBegin = 0.0;

    return newDfd;
}

UffFileDescriptor *Converter::createNewUffFile(const QString &fileName, DfdFileDescriptor *dfd, Parameters &p)
{DD;
    UffFileDescriptor *newUff = new UffFileDescriptor(fileName);

    newUff->fillPreliminary(dataTypefromDfdDataType(DfdDataType(p.method->dataType())));

    if (dfd->dataDescription) {
        newUff->setDataDescriptor(dfd->dataDescription->data);
    }

    const double newSampleRate = p.sampleRate / pow(2.0, p.bandStrip);
    newUff->setXStep(newSampleRate / p.blockSize);

    return newUff;
}

void Converter::addDfdChannel(DfdFileDescriptor *newDfd, DfdFileDescriptor *dfd, const QVector<double> &spectrum, Parameters &p, int i)
{DD;
    DfdChannel *ch = new DfdChannel(newDfd, newDfd->channelsCount());
    ch->XStep = newDfd->XStep;
    ch->setYValues(spectrum);
    ch->setPopulated(true);
    ch->setName(dfd->channels[i]->name());

    ch->ChanDscr = dfd->channels[i]->ChanDscr;
    ch->ChanAddress = dfd->channels[i]->ChanAddress;

    ch->ChanBlockSize = spectrum.size();
    ch->NumInd = spectrum.size();
    ch->IndType = 3221225476;

    ch->YName = p.scaleType==0?dfd->channels[i]->yName():"дБ";
    ch->YNameOld = dfd->channels[i]->yName();
    ch->XName = "Гц";

//        ch->xMin = 0.0;
//        ch->xMax = newSampleRate / 2.56;
//        ch->XMaxInitial = ch->xMax;
//        ch->YMinInitial = ch->yMin;
//        ch->YMaxInitial = ch->yMax;

    newDfd->channels << ch;
}

void Converter::addUffChannel(UffFileDescriptor *newUff, DfdFileDescriptor *dfd, const QVector<QPair<double, double> > &spectrum, Parameters &p, int i)
{DD;
    Function *ch = new Function(newUff);
    ch->setName(dfd->channels[i]->name()/*+"/Сила"*/);
    ch->setPopulated(true);

    //FunctionHeader header;
    ch->header.type1858[12].value = uffWindowType(p.windowType);


    ch->type58[8].value = QDateTime::currentDateTime();;
    ch->type58[14].value = uffMethodFromDfdMethod(p.method->id());
    ch->type58[15].value = i+1;
    //ch->type58[18].value = dfd->channels[i]->name(); //18  Response Entity Name ("NONE" if unused)
    ch->type58[18].value = QString("p%1").arg(i+1);
    ch->type58[20].value = 3; //20 Response Direction +Z
    //ch->type58[21].value = dfd->channels[p.baseChannel]->name(); //18  Reference Entity Name ("NONE" if unused)
    ch->type58[21].value = QString("p%1").arg(p.baseChannel+1);
    ch->type58[23].value = 3; //20 Reference Direction +Z
    ch->type58[25].value = p.saveAsComplex ? 5 : 2; //25 Ordinate Data Type
    ch->type58[26].value = spectrum.size();
    ch->samples = spectrum.size();
    ch->type58[28].value = 0.0;

    double newSampleRate = p.sampleRate / pow(2.0, p.bandStrip);
    double XStep = newSampleRate / p.blockSize;
    ch->type58[29].value = XStep; //29 Abscissa increment
    ch->type58[32].value = 18; // 18 - frequency
    ch->type58[36].value = "Частота";
    ch->type58[37].value = "Гц";
    ch->type58[44].value = "(m/s2)/N";

    ch->type58[53].value = 1;
    ch->type58[57].value = "Time";
    ch->type58[58].value = "s";

    //                                    Data Values
    //                            Ordinate            Abscissa
    //                Case     Type     Precision     Spacing       Format
    //              -------------------------------------------------------------
    //                  1      real      single        even         6E13.5
    //                  2      real      single       uneven        6E13.5
    //                  3     complex    single        even         6E13.5
    //                  4     complex    single       uneven        6E13.5
    //                  5      real      double        even         4E20.12
    //                  6      real      double       uneven     2(E13.5,E20.12)
    //                  7     complex    double        even         4E20.12
    //                  8     complex    double       uneven      E13.5,2E20.12
    //              --------------------------------------------------------------

    ch->valuesComplex = spectrum;
    newUff->channels << ch;
}

void Converter::addUffChannel(UffFileDescriptor *newUff, DfdFileDescriptor *dfd, const QVector<double> &spectrum, Parameters &p, int i)
{DD;
    Function *ch = new Function(newUff);
    ch->setName(dfd->channels[i]->name()/*+"/Сила"*/);
    ch->setPopulated(true);

    //FunctionHeader header;
    ch->header.type1858[12].value = uffWindowType(p.windowType);


    ch->type58[8].value = QDateTime::currentDateTime();;
    ch->type58[14].value = uffMethodFromDfdMethod(p.method->id());
    ch->type58[15].value = i+1;
    //ch->type58[18].value = dfd->channels[i]->name(); //18  Response Entity Name ("NONE" if unused)
    ch->type58[18].value = QString("p%1").arg(i+1);
    ch->type58[20].value = 3; //20 Response Direction +Z
    //ch->type58[21].value = dfd->channels[p.baseChannel]->name(); //18  Reference Entity Name ("NONE" if unused)
    ch->type58[21].value = QString("p%1").arg(p.baseChannel+1);
    ch->type58[23].value = 3; //20 Reference Direction +Z
    ch->type58[25].value = p.saveAsComplex ? 5 : 2; //25 Ordinate Data Type
    ch->type58[26].value = spectrum.size();
    ch->samples = spectrum.size();
    ch->type58[28].value = 0.0;

    double newSampleRate = p.sampleRate / pow(2.0, p.bandStrip);
    double XStep = newSampleRate / p.blockSize;
    ch->type58[29].value = XStep; //29 Abscissa increment
    ch->type58[32].value = 18; // 18 - frequency
    ch->type58[36].value = "Частота";
    ch->type58[37].value = "Гц";
    ch->type58[44].value = "(m/s2)/N";

    ch->type58[53].value = 1;
    ch->type58[57].value = "Time";
    ch->type58[58].value = "s";

    //                                    Data Values
    //                            Ordinate            Abscissa
    //                Case     Type     Precision     Spacing       Format
    //              -------------------------------------------------------------
    //                  1      real      single        even         6E13.5
    //                  2      real      single       uneven        6E13.5
    //                  3     complex    single        even         6E13.5
    //                  4     complex    single       uneven        6E13.5
    //                  5      real      double        even         4E20.12
    //                  6      real      double       uneven     2(E13.5,E20.12)
    //                  7     complex    double        even         4E20.12
    //                  8     complex    double       uneven      E13.5,2E20.12
    //              --------------------------------------------------------------

    ch->values = spectrum;
    newUff->channels << ch;
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

QString averaging(int avgType)
{
    switch (avgType) {
        case 0: return "линейное";
        case 1: return "экспоненциальное";
        case 2: return "хранение максимума";
    }
    return "";
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

int stripByBandwidth(double bandwidth, Parameters &p)
{DD;
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

    p.averagesCount = int(1.0 * dfd->channels[0]->samplesCount() / p.blockSize / (1.0 - p.overlap));
    if (dfd->channels[0]->samplesCount() % p.averagesCount !=0) p.averagesCount++;

    // Если опорный канал с таким номером в файле отсутствует, используем последний канал в файле
    if (p.baseChannel>=dfd->channelsCount()) p.baseChannel = dfd->channelsCount()-1;



    /** 1. Создаем конечный файл и копируем в него всю информацию из dfd */
    QString fileName = createUniqueFileName(tempFolderName, dfd->fileName(), p.methodDll, p.saveAsComplex);

    DfdFileDescriptor *newDfd = 0;
    if (!p.saveAsComplex) newDfd = createNewDfdFile(fileName, dfd, p);

    UffFileDescriptor *newUff = 0;
    if (p.saveAsComplex) newUff = createNewUffFile(fileName, dfd, p);


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
        quint32 averagesMade = 1;

        QVector<float> filtered;

        Filter filter(p);

        const int newBlockSize = p.blockSize * (1<<p.bandStrip);

        const quint32 stepBack = quint32(1.0*newBlockSize * p.overlap);
        if (p.method->id()==1) {//спектр мощности
            while (1) {
                QVector<float> chunk = getBlock(dfd->channels[i]->floatValues, newBlockSize, stepBack, block);

                if (chunk.size() < p.blockSize) break;

                filtered = filter.process(chunk);
                applyWindow(filtered, p);
                QVector<double> out = powerSpectre(filtered, p);
                average(spectrum, out, p, averagesMade++);
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
                if (chunk1.size() < p.blockSize || chunk2.size() < p.blockSize) break;

                filtered = filter.process(chunk1);
                baseFiltered = baseFilter.process(chunk2);

                applyWindow(filtered, p);
                applyWindow(baseFiltered, p);
                QVector<double> baseautoSpectr = autoSpectre(baseFiltered, p);
                QVector<double> coSpectr = coSpectre(baseFiltered, filtered, p);

                average(averagedBaseSpectre, baseautoSpectr, p, averagesMade);
                average(averagedSpectre, coSpectr, p, averagesMade++);
            }
            spectrum = transferFunctionH1(averagedBaseSpectre, averagedSpectre, p);
            const double t2 = threshold(dfd->channels[p.baseChannel]->yName()) / p.threshold;
            for (int i=0; i<spectrum.size(); ++i)
                spectrum[i] = 20 * log10(spectrum[i] * t2);
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
                if (chunk1.size() < p.blockSize || chunk2.size() < p.blockSize) break;

                filtered = filter.process(chunk1);
                baseFiltered = baseFilter.process(chunk2);

                applyWindow(filtered, p);
                applyWindow(baseFiltered, p);
                QVector<double> baseautoSpectr = autoSpectre(baseFiltered, p);
                QVector<QPair<double,double> > coSpectr = coSpectreComplex(baseFiltered, filtered, p);

                average(averagedBaseSpectre, baseautoSpectr, p, averagesMade);
                averageComplex(averagedSpectre, coSpectr, p, averagesMade++);
            }
            spectrumComplex = transferFunctionH1Complex(averagedBaseSpectre, averagedSpectre, p);
//            const double t2 = threshold(dfd->channels[p.baseChannel]->yName()) / p.threshold;
//            for (int i=0; i<spectrum.size(); ++i)
//                spectrum[i] = 20 * log10(spectrum[i] * t2);
        }

        // Создаем канал и заполняем его
        if (newDfd) {
            addDfdChannel(newDfd, dfd, spectrum, p, i);
        }
        if (newUff) {
            if (p.saveAsComplex)
                addUffChannel(newUff, dfd, spectrumComplex, p, i);
            else
                addUffChannel(newUff, dfd, spectrum, p, i);
        }

        dfd->channels[i]->floatValues.clear();

        emit tick();
    }
    if (p.baseChannel>=0 && p.baseChannel < dfd->channelsCount())
        dfd->channels[p.baseChannel]->floatValues.clear();

    if (newDfd) {
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
    QVector<double> output(p.blockSize);
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
    QVector<double> output(p.blockSize);
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
    QVector<QPair<double, double> > output(p.blockSize);

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
    QVector<double> output(p.blockSize);
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


        p.bandStrip = stripByBandwidth(dynamic_cast<RawChannel *>(dfd->channel(0))->BandWidth, p);

        spfFile << QString("ActChannel=%1").arg(p.activeChannel);
        spfFile << QString("BaseChannel=%1").arg(p.baseChannel);
        spfFile << "MinMax=*,*,*,*";
        spfFile << QString("AStrip=%1").arg(p.bandStrip);
        spfFile << QString("StepBack=%1").arg(floattohex(p.overlap));

        spfFile << "ShiftDat=0"; // TODO: добавить возможность устанавливать смещение
        // длина = число отсчетов в канале
        // TODO: добавить возможность устанавливать правую границу выборки
        quint32 NI = dfd->channels.at(p.activeChannel>0?p.activeChannel-1:0)->ChanBlockSize / dfd->BlockSize;
        NI *= dfd->samplesCount();

        spfFile << QString("Duration=%1").arg(NI);
        spfFile << "TablName=";
        spfFile << QString("ViewPar=1,0,1,%1,1,1,0").arg(p.bandStrip);
        spfFile << "FacePos=0";
        spfFile << "Strob=0";

        spfFile << QString("[Panel%1\\Wind1\\Method]").arg(i);

        if (p.method)
            spfFile << p.method->methodSettings(dfd, p);
        else qDebug()<<"No method found";

//        spfFile << QString("YName=дБ");
//        spfFile << QString("BlockIn=%1").arg(p.blockSize);
//        spfFile << QString("Wind=%1").arg(window(p.windowType));
//        spfFile << QString("TypeAver=%1").arg(p.averagingType);

//        quint32 numberOfInd = dfd->channels.at(p.activeChannel>0?p.activeChannel-1:0)->samplesCount();
//        double blockSize = p.blockSize;
//        double NumberOfAveraging = double(numberOfInd) / blockSize / (1<<p.bandStrip);

//        // at least 2 averaging
//        if (NumberOfAveraging<1) NumberOfAveraging = 2.0;

//        spfFile << QString("NAver=%1").arg(qRound(NumberOfAveraging));
//        spfFile << "TypeProc=мощности";
//        spfFile << "Values=измеряемые";

//        spfFile << (QString("TypeScale=")+(p.scaleType==0?QString("линейная"):QString("в децибелах")));
//        spfFile << "AddProc=нет";
    }
    return spfFile;
}

/** Возвращает тип окна, применяемый в uff заголовке 1858
    wind - тип окна, применяемый в DeepSea*/
int uffWindowType(int dfdWindowType)
{
    //12 window type, 0=no, 1=hanning narrow, 2=hanning broad, 3=flattop,
   //4=exponential, 5=impact, 6=impact and exponential
    switch (dfdWindowType) {
        case 0: return 3;//"Прямоуг.";
        case 1: return 0;//"Бартлетта";
        case 2: return 1;//"Хеннинга";
        case 3: return 0;//"Хемминга";
        case 4: return 0;//"Натолл";
        case 5: return 0;//"Гаусс";
    }
    return 0;
}


int uffMethodFromDfdMethod(int methodId)
{
    switch (methodId) {
        case 9: return 4; //4 - Frequency Response Function
        case 0: return 1; //1 - Time Response
        case 1: return 12; //12 - Spectrum
    }
    return 0;
    // ниже - нереализованные методы
    //                                       0 - General or Unknown
    //                                       2 - Auto Spectrum
    //                                       3 - Cross Spectrum
    //                                       5 - Transmissibility
    //                                       6 - Coherence
    //                                       7 - Auto Correlation
    //                                       8 - Cross Correlation
    //                                       9 - Power Spectral Density (PSD)
    //                                       10 - Energy Spectral Density (ESD)
    //                                       11 - Probability Density Function
    //                                       13 - Cumulative Frequency Distribution
    //                                       14 - Peaks Valley
    //                                       15 - Stress/Cycles
    //                                       16 - Strain/Cycles
    //                                       17 - Orbit
    //                                       18 - Mode Indicator Function
    //                                       19 - Force Pattern
    //                                       20 - Partial Power
    //                                       21 - Partial Coherence
    //                                       22 - Eigenvalue
    //                                       23 - Eigenvector
    //                                       24 - Shock Response Spectrum
    //                                       25 - Finite Impulse Response Filter
    //                                       26 - Multiple Coherence
    //                                       27 - Order Function
}
