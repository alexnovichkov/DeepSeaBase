#include "convertdialog.h"

#include <QtWidgets>

ConvertDialog::ConvertDialog(QList<DfdFileDescriptor *> *dataBase, QWidget *parent) :
    QDialog(parent), dataBase(dataBase), process(0)
{
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
    double bandWidth = dynamic_cast<RawChannel *>(dataBase->first()->channels.first())->BandWidth;
    for (int i=0; i<12; ++i) {
        if (bands[i] <= bandWidth) {
            bandWidth = bands[i];
            break;
        }
    }
    for (int i=0; i<12; ++i) {
        activeStripCombo->addItem(QString::number(bandWidth));
        bandWidth /= 2.0;
    }
    activeStripCombo->setCurrentIndex(2);

    activeChannelSpin = new QSpinBox(this);
    activeChannelSpin->setRange(1, 256);
    activeChannelSpin->setValue(1);
    baseChannelSpin = new QSpinBox(this);
    baseChannelSpin->setRange(1, 256);
    baseChannelSpin->setValue(2);

    methodsStack = new QStackedWidget(this);
    for (int i=0; i<26; ++i)
        methodsStack->addWidget(new SpectreMethod(this));

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
        DfdFileDescriptor *dfd = dataBase->at(indexes.at(i));
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
        spfFile << QString("FileNm=%1").arg(QDir::toNativeSeparators(dfd->dfdFileName));

        QStringList channelsList;
        for (int i=1; i<=dfd->channels.size(); ++i) channelsList << QString::number(i);
        //foreach(int ch, channels) channelsList << QString::number(ch);
        spfFile << "Channels="+channelsList.join(',');

        spfFile << QString("ActChannel=%1").arg(activeChannelSpin->value());
        spfFile << QString("BaseChannel=%1").arg(baseChannelSpin->value());
        spfFile << "MinMax=*,*,*,*";
        spfFile << QString("AStrip=%1").arg(activeStripCombo->currentIndex());
        spfFile << "StepBack=(00000000)"; // TODO: добавить возможность устанавливать перекрытие
        spfFile << "ShiftDat=0"; // TODO: добавить возможность устанавливать смещение
        // длина = число отсчетов в канале
        // TODO: добавить возможность устанавливать правую границу выборки
        quint32 NI = dfd->NumInd *
                     dfd->channels.at(activeChannelSpin->value())->ChanBlockSize
                     / dfd->BlockSize;
        spfFile << QString("Duration=%1").arg(NI);
        spfFile << "TablName=";
        spfFile << QString("ViewPar=1,0,1,%1,1,1,0").arg(activeStripCombo->currentIndex());
        spfFile << "FacePos=0";
        spfFile << "Strob=0";

        spfFile << QString("[Panel%1\\Wind1\\Method]").arg(i);
        spfFile << currentMethod->methodSettings(dfd, activeChannelSpin->value(), activeStripCombo->currentIndex());
    }
    return spfFile;
}

void ConvertDialog::methodChanged(int method)
{
    this->method = method;
    methodsStack->setCurrentIndex(method);
    currentMethod = dynamic_cast<AbstractMethod *>(methodsStack->currentWidget());
}

void ConvertDialog::accept()
{
    newFiles.clear();

    QMap<QString, QVector<int> > sortedFiles;
    for (int i=0; i< dataBase->size(); ++i) {
        DfdFileDescriptor *dfd = dataBase->at(i);
        QString dir = QFileInfo(dfd->dfdFileName).absolutePath();
        QVector<int> indexes = sortedFiles.value(dir);
        indexes.append(i);
        sortedFiles.insert(dir, indexes);
    }
    //qDebug()<<sortedFiles;

    QMapIterator<QString, QVector<int> > it(sortedFiles);
    while (it.hasNext()) {
        it.next();
        QStringList spfFile = getSpfFile(it.value(), it.key());
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

        QStringList arguments = QString("%1.-E.-S")
                                .split(".");
        arguments[0] = file.fileName();
        QEventLoop q;
        connect(process,SIGNAL(finished(int)),&q,SLOT(quit()));
        connect(process,SIGNAL(error(QProcess::ProcessError)),&q,SLOT(quit()));

        process->start("DeepSea",arguments);
        q.exec();

        int code = process->exitCode();
        if (code == 0) {
            QFileInfoList newFilesList = QDir(it.key()).entryInfoList(QStringList()<<"*.dfd",QDir::Files);
            Q_FOREACH(const QFileInfo &newFile, newFilesList) {
                if (newFile.created()>dt || newFile.lastModified()>dt)
                    newFiles << newFile.canonicalFilePath();
            }
        }
    }
    QDialog::accept();
}




SpectreMethod::SpectreMethod(QWidget *parent) :
    QWidget(parent)
{
    resolutionCombo = new QComboBox(this);
    resolutionCombo->addItem("512");
    resolutionCombo->addItem("1024");
    resolutionCombo->addItem("2048");
    resolutionCombo->addItem("4096");
    resolutionCombo->addItem("8192");
    resolutionCombo->setCurrentIndex(2);
    resolutionCombo->setEditable(false);

    windowCombo = new QComboBox(this);
    windowCombo->addItem("Прямоуг.");
    windowCombo->addItem("Бартлетта");
    windowCombo->addItem("Хеннинга");
    windowCombo->addItem("Хемминга");
    windowCombo->addItem("Натолл");
    windowCombo->addItem("Гаусс");
    windowCombo->setCurrentIndex(2);
    windowCombo->setEditable(false);

    averCombo = new QComboBox(this);
    averCombo->addItem("линейное");
    averCombo->addItem("экспоненциальное");
    averCombo->addItem("хранение максимума");
    averCombo->setCurrentIndex(0);
    averCombo->setEditable(false);

    nAverCombo = new QComboBox(this);
    nAverCombo->addItem("1");
    nAverCombo->addItem("2");
    nAverCombo->addItem("4");
    nAverCombo->addItem("8");
    nAverCombo->addItem("16");
    nAverCombo->addItem("32");
    nAverCombo->addItem("64");
    nAverCombo->addItem("128");
    nAverCombo->addItem("256");
    nAverCombo->addItem("1024");
    nAverCombo->addItem("до конца интервала");
    nAverCombo->setCurrentIndex(10);
    nAverCombo->setEditable(false);
    nAverCombo->setEnabled(false);

    typeCombo = new QComboBox(this);
    typeCombo->addItem("мощности");
    typeCombo->addItem("плотности мощн.");
    typeCombo->addItem("спектр СКЗ");
    typeCombo->setCurrentIndex(0);
    typeCombo->setEnabled(false);
    typeCombo->setEditable(false);

    valuesCombo = new QComboBox(this);
    valuesCombo->addItem("измеряемые");
    valuesCombo->addItem("вход АЦП");
    valuesCombo->setCurrentIndex(0);
    valuesCombo->setEnabled(false);
    valuesCombo->setEditable(false);

    scaleCombo = new QComboBox(this);
    scaleCombo->addItem("линейная");
    scaleCombo->addItem("в децибелах");
    scaleCombo->setCurrentIndex(1);
    scaleCombo->setEnabled(false);
    scaleCombo->setEditable(false);

    addProcCombo = new QComboBox(this);
    addProcCombo->addItem("нет");
    addProcCombo->addItem("интегрир.");
    addProcCombo->addItem("дифференц.");
    addProcCombo->addItem("дв.интергир.");
    addProcCombo->addItem("дв.дифференц.");
    addProcCombo->setCurrentIndex(0);
    addProcCombo->setEnabled(false);
    addProcCombo->setEditable(false);

    QFormLayout *l = new QFormLayout;
//    l->addRow("Частотный диапазон", rangeCombo);
    l->addRow("Разрешение по частоте", resolutionCombo);
    l->addRow("Окно", windowCombo);
    l->addRow("Усреднение", averCombo);
    l->addRow("Кол. усреднений", nAverCombo);
    l->addRow("Тип спектра", typeCombo);
    l->addRow("Величины", valuesCombo);
    l->addRow("Шкала", scaleCombo);
    l->addRow("Доп. обработка", addProcCombo);
    setLayout(l);
}

int SpectreMethod::id()
{
    return 1;
}

QStringList SpectreMethod::methodSettings(DfdFileDescriptor *dfd, int activeChannel, int strip)
{
    QStringList spfFile;
    QString yName = "дБ";
    if (typeCombo->currentText() != "в децибелах") {
        // TODO: реализовать правильный выбор единицы измерения
    }
    spfFile << QString("YName=%1").arg(yName);
    spfFile << QString("BlockIn=%1").arg(resolutionCombo->currentText());
    spfFile << "Wind="+windowCombo->currentText();
    spfFile << "TypeAver="+typeCombo->currentText();

    quint32 numberOfInd = dfd->channels.at(activeChannel)->NumInd;
    double resolution = resolutionCombo->currentText().toDouble();
    double NumberOfAveraging = double(numberOfInd) / resolution;
    while (strip>0) {
        NumberOfAveraging /= 2.0;
        strip--;
    }

    // at least 2 averaging
    if (NumberOfAveraging<1) NumberOfAveraging = 2.0;

    spfFile << QString("NAver=%1").arg(qRound(NumberOfAveraging));
    spfFile << "TypeProc="+typeCombo->currentText();
    spfFile << "Values="+valuesCombo->currentText();
    spfFile << "TypeScale="+typeCombo->currentText();
    spfFile << "AddProc="+addProcCombo->currentText();

    return spfFile;
}

QString SpectreMethod::methodDll()
{
    return "spectr.dll";
}

int SpectreMethod::panelType()
{
    return 0;
}

QString SpectreMethod::methodName()
{
    if (typeCombo->currentText()=="спектр СКЗ") return "Спектр СКЗ";
    if (typeCombo->currentText()=="мощности") return "Спектр мощности";
    if (typeCombo->currentText()=="плотности мощн.") return "Плотн.спектра мощности";
    return "Спектроанализатор";
}

int SpectreMethod::dataType()
{
    if (typeCombo->currentText()=="спектр СКЗ") return 130;
    if (typeCombo->currentText()=="мощности") return 128;
    if (typeCombo->currentText()=="плотности мощн.") return 129;
    return 128;
}

int SpectreMethod::computeNumberOfAveraging(const QString &aver)
{
    if (aver=="до конца интервала") {

    }
    return aver.toInt();
}

