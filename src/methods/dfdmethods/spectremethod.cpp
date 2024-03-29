#include "spectremethod.h"

#include <QtWidgets>

#include "fileformats/dfdfiledescriptor.h"
#include "fileformats/ufffile.h"
#include "logging.h"
#include "algorithms.h"
#include "methods/windowing.h"
#include "methods/averaging.h"

SpectreMethod::SpectreMethod(QList<FileDescriptor *> &dataBase, QWidget *parent) :
    QWidget(parent), AbstractMethod(dataBase)
{
    resolutionCombo = new QComboBox(this);
    resolutionCombo->setEditable(false);

    activeStripCombo = new QComboBox(this);
    activeStripCombo->setEditable(false);
    // заполняем список частотного диапазона
    if (RawChannel *raw = dynamic_cast<RawChannel *>(dataBase.constFirst()->channel(0))) {
        bandWidth = raw->BandWidth;
        sampleRate = 1.0 / raw->data()->xStep();
    }
    else {
        bandWidth = qRound(1.0 / dataBase.constFirst()->channel(0)->data()->xStep() / 2.56);
        sampleRate = 1.0 / dataBase.constFirst()->channel(0)->data()->xStep();
    }
    double bw = bandWidth;
    for (int i=0; i<12; ++i) {
        activeStripCombo->addItem(QString::number(bw));
        bw /= 2.0;
    }
    connect(activeStripCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(updateResolution(int)));
    activeStripCombo->setCurrentIndex(0);
    updateResolution(0);

    overlap = new QSpinBox(this);
    overlap->setRange(0,75);
    overlap->setValue(0);
    overlap->setSingleStep(5);

    windowParameter = new QLineEdit(this);
    windowParameter->setPlaceholderText("50%");
    windowParameter->setEnabled(false);

    windowCombo = new QComboBox(this);
    for (int i=0; i<6; ++i) //только 6 типов окон поддержвивается в Deepsea
        windowCombo->addItem(Windowing::windowDescription(static_cast<Windowing::WindowType>(i)));
    windowCombo->setCurrentIndex(2);
    windowCombo->setEditable(false);
    connect(windowCombo, QOverload<int>::of(&QComboBox::activated), [=](int index){
        windowParameter->setEnabled(Windowing::windowAcceptsParameter(static_cast<Windowing::WindowType>(index)));
    });



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
//    nAverCombo->setEnabled(false);

    typeCombo = new QComboBox(this);
    typeCombo->addItem("мощности");
    typeCombo->addItem("плотности мощн.");
    typeCombo->addItem("спектр СКЗ");
    typeCombo->setCurrentIndex(0);
//    typeCombo->setEnabled(false);
    typeCombo->setEditable(false);

    scaleCombo = new QComboBox(this);
    scaleCombo->addItem("линейная");
    scaleCombo->addItem("в децибелах");
    scaleCombo->setCurrentIndex(1);
//    scaleCombo->setEnabled(false);
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
    l->addRow("Частотный диапазон", activeStripCombo);
    l->addRow("Буфер (разр. по част.)", resolutionCombo);
    l->addRow("Перекрытие, %", overlap);
    l->addRow("Окно", windowCombo);
    l->addRow("Параметр окна", windowParameter);
    l->addRow("Усреднение", averCombo);
    l->addRow("Кол. усреднений", nAverCombo);
    l->addRow("Тип спектра", typeCombo);
    l->addRow("Шкала", scaleCombo);
    l->addRow("Доп. обработка", addProcCombo);
    setLayout(l);
}

int SpectreMethod::id()
{
    return 1;
}

QStringList SpectreMethod::methodSettings(FileDescriptor *dfd, const Parameters &p)
{DD;
    QStringList spfFile;
    QString yName = "дБ";
    if (typeCombo->currentText() != "в децибелах") {
        // TODO: реализовать правильный выбор единицы измерения
    }
    spfFile << QString("YName=%1").arg(yName);
    spfFile << QString("BlockIn=%1").arg(p.bufferSize);
//    spfFile << QString("Wind=%1").arg(Windowing::windowDescription(p.windowType));
    spfFile << QString("TypeAver=%1").arg(Averaging::averagingDescription(p.averagingType+1));

    int numberOfInd = dfd->channel(0)->data()->samplesCount();
    double NumberOfAveraging = double(numberOfInd) / p.bufferSize / (1<<p.bandStrip);
    if (NumberOfAveraging<1) NumberOfAveraging = 1;
    int nAver = qRound(NumberOfAveraging);
    if (p.averagesCount != -1) nAver = qMin(p.averagesCount, nAver);
    spfFile << QString("NAver=%1").arg(nAver);

    spfFile << "TypeProc="+typeCombo->currentText();
    spfFile << "Values=измеряемые";
    spfFile << "TypeScale="+scaleCombo->currentText();
    spfFile << "AddProc="+addProcCombo->currentText();

    return spfFile;
}

Parameters SpectreMethod::parameters()
{
    bool ok;
    Parameters p;
    p.sampleRate = sampleRate;
    p.averagingType = averCombo->currentIndex();

    const double po = pow(2.0, resolutionCombo->currentIndex());
    p.bufferSize = qRound(32768.0 / po); // размер блока

    p.windowType = windowCombo->currentIndex();
    QString percentString = windowParameter->text().trimmed();
    if (percentString.endsWith("%")) percentString.chop(1);
    double percent = percentString.toDouble(&ok);
    if (ok && percent != 0.0) p.windowPercent = percent;

    p.scaleType = scaleCombo->currentIndex();
    p.averagesCount = nAverCombo->currentText().toInt(&ok);
    if (!ok) p.averagesCount = -1;

    p.overlap = 1.0 * overlap->value() / 100;
    p.bandWidth = bandWidth;
    p.initialBandStripNumber = activeStripCombo->currentIndex();
    p.saveAsComplex = false;

    return p;
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

DescriptionList SpectreMethod::processData(const Parameters &p)
{
    DescriptionList list;
    list.append(qMakePair(QStringLiteral("PName"), methodName()));
    list.append(qMakePair(QStringLiteral("BlockIn"), QString::number(p.bufferSize)));
//    list.append(qMakePair(QStringLiteral("Wind"), Windowing::windowDescription(p.windowType)));
    list.append(qMakePair(QStringLiteral("TypeAver"), Averaging::averagingDescription(p.averagingType+1)));
    list.append(qMakePair(QStringLiteral("pTime"),QStringLiteral("(0000000000000000)")));
    return list;
}

void SpectreMethod::updateResolution(int bandStrip)
{
    Q_UNUSED(bandStrip);
    double sR = sampleRate;
    double bS = 32768.0;
    resolutionCombo->clear();
    for (int i=0; i<8; ++i) {
        double p = pow(2.0, i);
        resolutionCombo->addItem(QString("%1 (%2 Гц)").arg(qRound(bS / p)).arg(sR / bS * p));
    }
    resolutionCombo->setCurrentIndex(2);
}


/*DfdFileDescriptor *SpectreMethod::createNewDfdFile(const QString &fileName, FileDescriptor *dfd, Parameters &p)
{DD;
    DfdFileDescriptor *newDfd = AbstractMethod::createNewDfdFile(fileName, dfd, p);

    // rest
//    newDfd->XName = "Гц";
//    const double newSampleRate = p.sampleRate / pow(2.0, p.bandStrip);
//    newDfd->XStep = newSampleRate / p.bufferSize;
//    newDfd->XBegin = 0.0;

    return newDfd;
}*/

//UffFileDescriptor *SpectreMethod::createNewUffFile(const QString &fileName, FileDescriptor *dfd, Parameters &p)
//{DD;
//    UffFileDescriptor *newUff = new UffFileDescriptor(fileName);

//    newUff->setDataDescription(dfd->dataDescription());
//    newUff->updateDateTimeGUID();

//    const double newSampleRate = p.sampleRate / pow(2.0, p.bandStrip);
//    newUff->setXStep(newSampleRate / p.bufferSize);

//    return newUff;
//}

/*Channel *SpectreMethod::createDfdChannel(DfdFileDescriptor *newDfd, FileDescriptor *dfd, const QVector<double> &spectrum, Parameters &p, int i)
{DD;
    DataDescription descr;
    DataHolder *d = new DataHolder();
    double XStep = p.sampleRate / pow(2.0, p.bandStrip) / p.bufferSize;
    d->setXValues(0.0, XStep, spectrum.size());
    d->setThreshold(p.threshold);
    d->setYValues(spectrum, p.scaleType == 0 ? DataHolder::YValuesAmplitudes : DataHolder::YValuesAmplitudesInDB);
    //ch->setPopulated(true);
    descr.put("name", dfd->channel(i)->name());
    descr.put("description", dfd->channel(i)->description());
    descr.put("sensorId", dfd->channel(i)->dataDescription().get("sensorID"));
    descr.put("yname", p.scaleType==0?dfd->channel(i)->yName():"дБ");
    descr.put("ynameold", dfd->channel(i)->yName());

    newDfd->addChannelWithData(d, descr);

    return newDfd->channel(newDfd->channelsCount()-1);
}*/

/*Channel *SpectreMethod::addUffChannel(UffFileDescriptor *newUff, FileDescriptor *dfd, int spectrumSize, Parameters &p, int i)
{DD;
    Function *ch = new Function(newUff);
//    ch->setName(dfd->channel(i)->name());
//    ch->setPopulated(true);

//    //FunctionHeader header;
//    ch->header.type1858[12].value = uffWindowType(p.windowType);


//    ch->type58[8].value = QDateTime::currentDateTime();
//    ch->type58[14].value = uffMethodFromDfdMethod(id());
//    ch->type58[15].value = i+1;
//    //ch->type58[18].value = dfd->channels[i]->name(); //18  Response Entity Name ("NONE" if unused)
//    ch->type58[18].value = QString("p%1").arg(i+1);
//    ch->type58[20].value = 3; //20 Response Direction +Z
//    //ch->type58[21].value = dfd->channels[p.baseChannel]->name(); //18  Reference Entity Name ("NONE" if unused)
//    ch->type58[21].value = p.baseChannel>=0?QString("p%1").arg(p.baseChannel+1):"NONE";
//    ch->type58[23].value = 3; //20 Reference Direction +Z
//    ch->type58[25].value = p.saveAsComplex ? 5 : 2; //25 Ordinate Data Type
//    ch->type58[26].value = spectrumSize;
//    ch->type58[28].value = 0.0; //28 Abscissa minimum

//    double newSampleRate = p.sampleRate / pow(2.0, p.bandStrip);
//    double XStep = newSampleRate / p.bufferSize;
//    ch->type58[29].value = XStep; //29 Abscissa increment
//    ch->type58[32].value = 18; // 18 - frequency //32 Abscissa type
//    ch->type58[36].value = "Частота"; //32 Abscissa type description
//    ch->type58[37].value = "Гц"; //37 Abscissa name

//    ch->type58[39].value = 1; //39 Ordinate (or ordinate numerator) Data Characteristics // 1 = General
//    ch->type58[44].value = dfd->channel(i)->yName(); //44 Ordinate name

//    ch->type58[53].value = 0; //53 Z axis data characteristics // 0 = Unknown
//    ch->type58[57].value = "Time"; //57 Z-axis label
//    ch->type58[58].value = "s"; //58 Z-Axis units label ("NONE" if not used)

//    ch->data()->setXValues(0, XStep, spectrumSize);
    return ch;
}*/
