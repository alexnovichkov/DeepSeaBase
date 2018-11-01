#include "spectremethod.h"

#include <QtWidgets>

#include "dfdfiledescriptor.h"
#include "ufffile.h"
#include "logging.h"
#include "algorithms.h"
#include "windowing.h"
#include "averaging.h"

SpectreMethod::SpectreMethod(QList<DfdFileDescriptor *> &dataBase, QWidget *parent) :
    QWidget(parent), AbstractMethod(dataBase)
{
    saveAsComplexCheckBox = new QCheckBox("Сохранять комплексные значения", this);

    resolutionCombo = new QComboBox(this);
    resolutionCombo->setEditable(false);

    activeStripCombo = new QComboBox(this);
    activeStripCombo->setEditable(false);
    // заполняем список частотного диапазона
    if (RawChannel *raw = dynamic_cast<RawChannel *>(dataBase.first()->channel(0))) {
        bandWidth = raw->BandWidth;
        sampleRate = 1.0 / raw->xStep();
    }
    else {
        bandWidth = qRound(1.0 / dataBase.first()->channel(0)->xStep() / 2.56);
        sampleRate = 1.0 / dataBase.first()->channel(0)->xStep();
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
//    nAverCombo->setEnabled(false);

    typeCombo = new QComboBox(this);
    typeCombo->addItem("мощности");
    typeCombo->addItem("плотности мощн.");
    typeCombo->addItem("спектр СКЗ");
    typeCombo->setCurrentIndex(0);
    typeCombo->setEnabled(false);
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
    l->addRow("Усреднение", averCombo);
    l->addRow("Кол. усреднений", nAverCombo);
    l->addRow("Тип спектра", typeCombo);
    l->addRow("Шкала", scaleCombo);
    l->addRow("Доп. обработка", addProcCombo);
    l->addRow(saveAsComplexCheckBox);
    setLayout(l);
}

int SpectreMethod::id()
{
    return 1;
}

QStringList SpectreMethod::methodSettings(DfdFileDescriptor *dfd, const Parameters &p)
{DD;
    QStringList spfFile;
    QString yName = "дБ";
    if (typeCombo->currentText() != "в децибелах") {
        // TODO: реализовать правильный выбор единицы измерения
    }
    spfFile << QString("YName=%1").arg(yName);
    spfFile << QString("BlockIn=%1").arg(p.bufferSize);
    spfFile << QString("Wind=%1").arg(Windowing::windowDescription(p.windowType));
    spfFile << QString("TypeAver=%1").arg(Averaging::averagingDescription(p.averagingType));

    int numberOfInd = dfd->channels.at(0)->samplesCount();
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
    Parameters p;
    p.sampleRate = sampleRate;
    p.averagingType = averCombo->currentIndex();

    const double po = pow(2.0, resolutionCombo->currentIndex());
    p.bufferSize = qRound(sampleRate / po); // размер блока
    p.windowType = windowCombo->currentIndex();
    p.scaleType = scaleCombo->currentIndex();
    bool ok;
    p.averagesCount = nAverCombo->currentText().toInt(&ok);
    if (!ok) p.averagesCount = -1;
    p.baseChannel = 1;
    p.overlap = 1.0 * overlap->value() / 100;
    p.bandWidth = bandWidth;
    p.initialBandStripNumber = activeStripCombo->currentIndex();
    p.saveAsComplex = saveAsComplexCheckBox->isChecked();

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
    list.append({"PName", methodName()});
    list.append({"BlockIn", QString::number(p.bufferSize)});
    list.append({"Wind", Windowing::windowDescription(p.windowType)});
    list.append({"TypeAver", Averaging::averagingDescription(p.averagingType)});
    list.append({"pTime","(0000000000000000)"});
    return list;
}

void SpectreMethod::updateResolution(int bandStrip)
{
    double sR = sampleRate;
    resolutionCombo->clear();
    for (int i=0; i<5; ++i) {
        double p = pow(2.0, i);
        double p1 = pow(2.0, i-bandStrip);
        resolutionCombo->addItem(QString("%1 (%2 Гц)").arg(qRound(sR / p)).arg(p1));
    }
    resolutionCombo->setCurrentIndex(0);
}


DfdFileDescriptor *SpectreMethod::createNewDfdFile(const QString &fileName, DfdFileDescriptor *dfd, Parameters &p)
{
    DD;
    DfdFileDescriptor *newDfd = new DfdFileDescriptor(fileName);

    newDfd->rawFileName = fileName.left(fileName.length()-4)+".raw";
    newDfd->updateDateTimeGUID();
    newDfd->BlockSize = 0;
    newDfd->DataType = DfdDataType(dataType());

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
    newDfd->process->data = processData(p);

    // rest
    newDfd->XName = "Гц";
    const double newSampleRate = p.sampleRate / pow(2.0, p.bandStrip);
    newDfd->XStep = newSampleRate / p.bufferSize;
    newDfd->XBegin = 0.0;

    return newDfd;
}

UffFileDescriptor *SpectreMethod::createNewUffFile(const QString &fileName, DfdFileDescriptor *dfd, Parameters &p)
{DD;
    UffFileDescriptor *newUff = new UffFileDescriptor(fileName);

    newUff->fillPreliminary((Descriptor::DataType)0);

    if (dfd->dataDescription) {
        newUff->setDataDescriptor(dfd->dataDescription->data);
    }

    const double newSampleRate = p.sampleRate / pow(2.0, p.bandStrip);
    newUff->setXStep(newSampleRate / p.bufferSize);

    return newUff;
}

DfdChannel *SpectreMethod::createDfdChannel(DfdFileDescriptor *newDfd, DfdFileDescriptor *dfd, const QVector<double> &spectrum, Parameters &p, int i)
{DD;
    DfdChannel *ch = new DfdChannel(newDfd, newDfd->channelsCount());
    ch->data()->setXValues(0.0, newDfd->XStep, spectrum.size());
    ch->data()->setThreshold(p.threshold);
    ch->data()->setYValues(spectrum, p.scaleType == 0 ? DataHolder::YValuesAmplitudes : DataHolder::YValuesAmplitudesInDB);
    ch->setPopulated(true);
    ch->setName(dfd->channels[i]->name());

    ch->ChanDscr = dfd->channels[i]->ChanDscr;
    ch->ChanAddress = dfd->channels[i]->ChanAddress;

    ch->ChanBlockSize = spectrum.size();
    ch->IndType = 3221225476;

    ch->YName = p.scaleType==0?dfd->channels[i]->yName():"дБ";
    ch->YNameOld = dfd->channels[i]->yName();

    newDfd->channels << ch;
    return ch;
}

Function * SpectreMethod::addUffChannel(UffFileDescriptor *newUff, DfdFileDescriptor *dfd, int spectrumSize, Parameters &p, int i)
{DD;
    Function *ch = new Function(newUff);
    ch->setName(dfd->channels[i]->name());
    ch->setPopulated(true);

    //FunctionHeader header;
    ch->header.type1858[12].value = uffWindowType(p.windowType);


    ch->type58[8].value = QDateTime::currentDateTime();;
    ch->type58[14].value = uffMethodFromDfdMethod(id());
    ch->type58[15].value = i+1;
    //ch->type58[18].value = dfd->channels[i]->name(); //18  Response Entity Name ("NONE" if unused)
    ch->type58[18].value = QString("p%1").arg(i+1);
    ch->type58[20].value = 3; //20 Response Direction +Z
    //ch->type58[21].value = dfd->channels[p.baseChannel]->name(); //18  Reference Entity Name ("NONE" if unused)
    ch->type58[21].value = QString("p%1").arg(p.baseChannel+1);
    ch->type58[23].value = 3; //20 Reference Direction +Z
    ch->type58[25].value = p.saveAsComplex ? 5 : 2; //25 Ordinate Data Type
    ch->type58[26].value = spectrumSize;
    ch->type58[28].value = 0.0; //28 Abscissa minimum

    double newSampleRate = p.sampleRate / pow(2.0, p.bandStrip);
    double XStep = newSampleRate / p.bufferSize;
    ch->type58[29].value = XStep; //29 Abscissa increment
    ch->type58[32].value = 18; // 18 - frequency //32 Abscissa type
    ch->type58[36].value = "Частота"; //32 Abscissa type description
    ch->type58[37].value = "Гц"; //37 Abscissa name

    ch->type58[39].value = 1; //39 Ordinate (or ordinate numerator) Data Characteristics // 1 = General
    ch->type58[44].value = dfd->channels[i]->yName(); //44 Ordinate name

    ch->type58[53].value = 0; //53 Z axis data characteristics // 0 = Unknown
    ch->type58[57].value = "Time"; //57 Z-axis label
    ch->type58[58].value = "s"; //58 Z-Axis units label ("NONE" if not used)

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

    ch->data()->setXValues(0, XStep, spectrumSize);
    newUff->channels << ch;
    return ch;
}
