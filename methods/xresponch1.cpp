#include "xresponch1.h"

#include <QtWidgets>

#include "dfdfiledescriptor.h"
#include "ufffile.h"
#include "algorithms.h"
#include "logging.h"

XresponcH1Method::XresponcH1Method(QList<DfdFileDescriptor *> &dataBase, QWidget *parent) :
    QWidget(parent), AbstractMethod(dataBase)
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

    saveAsComplexCheckBox = new QCheckBox("Сохранять комплексные значения", this);


    QFormLayout *l = new QFormLayout;
    //    l->addRow("Частотный диапазон", rangeCombo);
    l->addRow("Разрешение по частоте", resolutionCombo);
    l->addRow("Окно", windowCombo);
    l->addRow("Усреднение", averCombo);
    l->addRow("Кол. усреднений", nAverCombo);
    l->addRow("Величины", valuesCombo);
    l->addRow("Шкала", scaleCombo);
    l->addWidget(saveAsComplexCheckBox);
    setLayout(l);
}


int XresponcH1Method::id()
{
    return 9;
}

QStringList XresponcH1Method::methodSettings(DfdFileDescriptor *dfd, const Parameters &p)
{
    QStringList spfFile;
    QString yName = "дБ";
    if (scaleCombo->currentText() != "в децибелах") {
        // TODO: реализовать правильный выбор единицы измерения
    }
    spfFile << QString("YName=%1").arg(yName);
    spfFile << QString("BlockIn=%1").arg(resolutionCombo->currentText());
    spfFile << "Wind="+windowCombo->currentText();
    spfFile << "TypeAver="+averCombo->currentText();

    quint32 numberOfInd = dfd->channels.at(p.activeChannel)->samplesCount();
    double NumberOfAveraging = double(numberOfInd) / p.bufferSize / (1<<p.bandStrip);

    // at least 2 averaging
    if (NumberOfAveraging<1) NumberOfAveraging = 2.0;

    spfFile << QString("NAver=%1").arg(qRound(NumberOfAveraging));
    spfFile << "Values="+valuesCombo->currentText();
    spfFile << "TypeScale="+scaleCombo->currentText();

    return spfFile;
}

//QStringList XresponcH1Method::settings(DfdFileDescriptor *dfd, int bandStrip)
//{
//    QStringList spfFile;

//    spfFile << "PName="+methodName();
//    spfFile << "pTime=(0000000000000000)";

//    spfFile << QString("BlockIn=%1").arg(resolutionCombo->currentText());
//    spfFile << "Wind="+windowCombo->currentText();
//    spfFile << "TypeAver="+averCombo->currentText();


//    const quint32 samplesCount = dfd->channels.first()->samplesCount();
//    const double blockSize = resolutionCombo->currentText().toDouble();
//    double NumberOfAveraging = double(samplesCount) / blockSize;

//    while (bandStrip>0) {
//        NumberOfAveraging /= 2.0;
//        bandStrip--;
//    }

//    // at least 2 averaging
//    if (NumberOfAveraging<1.0) NumberOfAveraging = 2.0;

//    spfFile << QString("NAver=%1").arg(qRound(NumberOfAveraging));
//    spfFile << "Values="+valuesCombo->currentText();
//    spfFile << "TypeScale="+scaleCombo->currentText();

//    return spfFile;
//}

Parameters XresponcH1Method::parameters()
{
    Parameters p;
    p.averagingType = averCombo->currentIndex();
    p.bufferSize = resolutionCombo->currentText().toInt();
    p.windowType = windowCombo->currentIndex();
    p.scaleType = scaleCombo->currentIndex();

    p.panelType = panelType();
    p.methodName = methodName();
    p.methodDll = methodDll();
    p.dataType = dataType();
    p.saveAsComplex = saveAsComplexCheckBox->isChecked();

    return p;
}

QString XresponcH1Method::methodDll()
{
    return "XresponcH1.dll";
}

int XresponcH1Method::panelType()
{
    return 0;
}

QString XresponcH1Method::methodName()
{
    return "Передаточная ф-я H1";
}

int XresponcH1Method::dataType()
{
    return 147;
}

DescriptionList XresponcH1Method::processData(const Parameters &p)
{
    DescriptionList list;
    list.append({"PName", p.methodName});
    list.append({"BlockIn", QString::number(p.bufferSize)});
    list.append({"Wind", p.windowDescription()});
    list.append({"TypeAver", p.averaging(p.averagingType)});
    list.append({"pTime","(0000000000000000)"});
    return list;
}


DfdFileDescriptor *XresponcH1Method::createNewDfdFile(const QString &fileName, DfdFileDescriptor *dfd, Parameters &p)
{
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
    newDfd->process->data = p.method->processData(p);

    // rest
    newDfd->XName = "Гц";
    const double newSampleRate = p.sampleRate / pow(2.0, p.bandStrip);
    newDfd->XStep = newSampleRate / p.bufferSize;
    newDfd->XBegin = 0.0;

    return newDfd;
}

UffFileDescriptor *XresponcH1Method::createNewUffFile(const QString &fileName, DfdFileDescriptor *dfd, Parameters &p)
{
    UffFileDescriptor *newUff = new UffFileDescriptor(fileName);

    newUff->updateDateTimeGUID();

    if (dfd->dataDescription) {
        newUff->setDataDescriptor(dfd->dataDescription->data);
    }

    const double newSampleRate = p.sampleRate / pow(2.0, p.bandStrip);
    newUff->setXStep(newSampleRate / p.bufferSize);

    return newUff;
}

DfdChannel *XresponcH1Method::createDfdChannel(DfdFileDescriptor *newDfd, DfdFileDescriptor *dfd, const QVector<double> &spectrum, Parameters &p, int i)
{
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

    return ch;
}

Function *XresponcH1Method::addUffChannel(UffFileDescriptor *newUff, DfdFileDescriptor *dfd, quint32 spectrumSize, Parameters &p, int i)
{DD;
    Function *ch = new Function(newUff);
    ch->setName(dfd->channels[i]->name()+"/Сила");
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
    ch->type58[26].value = p.fCount;
    ch->samples = p.fCount;
    ch->type58[28].value = 0.0;

    double newSampleRate = p.sampleRate / pow(2.0, p.bandStrip);
    double XStep = newSampleRate / p.bufferSize;
    ch->type58[29].value = XStep; //29 Abscissa increment
    ch->type58[32].value = 18; // 18 - frequency
    ch->type58[36].value = "Частота";
    ch->type58[37].value = "Гц";

    ch->type58[44].value = dfd->channels[i]->yName()+"/N"; // 44 Ordinate Axis units label ("NONE" if not used)

    //46-52 Ordinate Denominator Data Characteristics
    //skip

    ch->type58[53].value = 1; //53 Z-axis Data Characteristics
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

    newUff->channels << ch;
    return ch;
}
