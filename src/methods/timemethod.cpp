#include "timemethod.h"

#include <QtWidgets>
#include "fileformats/dfdfiledescriptor.h"
#include "fileformats/ufffile.h"
#include "logging.h"
#include "algorithms.h"

TimeMethod::TimeMethod(QList<FileDescriptor *> &dataBase, QWidget *parent) :
    QWidget(parent), AbstractMethod(dataBase)
{DD;
    resolutionCombo = new QComboBox(this);
    resolutionCombo->setEditable(false);

    xStep = dataBase.first()->channel(0)->xStep();

    // заполняем список частотного диапазона
    if (RawChannel *raw = dynamic_cast<RawChannel *>(dataBase.first()->channel(0))) {
        bandWidth = raw->BandWidth;
        sampleRate = 1.0 / raw->xStep();
        xStep = raw->xStep();
    }
    else {
        bandWidth = qRound(1.0 / xStep / 2.56);
        sampleRate = 1.0 / xStep;
    }
    double bw = bandWidth;
    for (int i=0; i<5; ++i) {
        resolutionCombo->addItem(QString::number(bw));
        bw /= 2.0;
    }


    minTimeLabel  = new QLabel(this);
    minTimeLabel->setText("00 s 000 ms");

    maxTimeLabel = new QLabel(this);
    maxTimeLabel->setText("00 s 000 ms");

    minTimeSlider = new QSlider(Qt::Horizontal, this);
    minTimeSlider->setRange(0, dataBase.first()->samplesCount());

    maxTimeSlider = new QSlider(Qt::Horizontal, this);
    maxTimeSlider->setRange(0, dataBase.first()->samplesCount());

    connect(minTimeSlider, &QSlider::valueChanged, [=](int value){
        minTimeLabel->setText(QString("%1 s %2 ms")
                              .arg(int(value * xStep * 1000) / 1000)
                              .arg(int(value * xStep * 1000) % 1000));
    });
    connect(maxTimeSlider, &QSlider::valueChanged, [=](int value){
        maxTimeLabel->setText(QString("%1 s %2 ms")
                              .arg(int(value * xStep * 1000) / 1000)
                              .arg(int(value * xStep * 1000) % 1000));
    });

    QFormLayout *l = new QFormLayout;
//    l->addRow("Частотный диапазон", rangeCombo);
    l->addRow("Частотный диапазон", resolutionCombo);
    QHBoxLayout *hbl1 = new QHBoxLayout;
    hbl1->addWidget(minTimeSlider);
    hbl1->addWidget(minTimeLabel);
    QHBoxLayout *hbl2 = new QHBoxLayout;
    hbl2->addWidget(maxTimeSlider);
    hbl2->addWidget(maxTimeLabel);
    l->addRow("От отсчета", hbl1);
    l->addRow("До отсчета", hbl2);
    setLayout(l);
}

int TimeMethod::id()
{
    return 0;
}

QStringList TimeMethod::methodSettings(FileDescriptor *dfd, const Parameters &p)
{DD;
    QStringList spfFile;

    spfFile << "YName="+dfd->channel(0)->yName();
    spfFile << QString("BlockIn=%1").arg(p.bufferSize);
    spfFile << "TypeProc=0";
    spfFile << "Values=измеряемые";

    int numberOfInd = dfd->channel(0)->samplesCount();
    double NumberOfAveraging = double(numberOfInd) / p.bufferSize / (1<<p.bandStrip);

    // at least 2 averaging
    if (NumberOfAveraging<=1) NumberOfAveraging = 2.0;

    spfFile << QString("NAver=%1").arg(qRound(NumberOfAveraging));

    return spfFile;
}

Parameters TimeMethod::parameters()
{DD;
    Parameters p;
    p.averagesCount = 1;
    p.averagingType = 0;
    p.bandStrip = 0;
    p.bandWidth = bandWidth;
    const double po = pow(2.0, resolutionCombo->currentIndex());
    p.bufferSize = qRound(sampleRate / po); // размер блока
    p.fCount = 0;
    p.initialBandStripNumber = resolutionCombo->currentIndex();
    p.overlap = 0;
    p.saveAsComplex = false;
    p.scaleType = 0;
    p.sampleRate = sampleRate;

    return p;
}

QString TimeMethod::methodDll()
{
    return "TimeWv.dll";
}

int TimeMethod::panelType()
{
    return 0;
}

QString TimeMethod::methodName()
{
    return "Осциллограф";
}

int TimeMethod::dataType()
{
    return 1;
}

DescriptionList TimeMethod::processData(const Parameters &p)
{DD;
    DescriptionList list;
    list.append({"PName", methodName()});
    list.append({"pTime","(0000000000000000)"});
    list.append({"BlockIn", QString::number(p.bufferSize)});
    list.append({"TypeProc", "0"});
    list.append({"NAver", "1"});
    list.append({"Values", "измеряемые"});
    return list;
}


DfdFileDescriptor *TimeMethod::createNewDfdFile(const QString &fileName, FileDescriptor *dfd, Parameters &p)
{DD;
    DfdFileDescriptor *newDfd = AbstractMethod::createNewDfdFile(fileName, dfd, p);

    // rest
    newDfd->DataType = DfdDataType::CuttedData;
    newDfd->XName = "с";
    const double newSampleRate = p.sampleRate / pow(2.0, p.bandStrip);
    newDfd->XStep = 1.0 / newSampleRate;
    newDfd->XBegin = 0.0;

    return newDfd;
}

UffFileDescriptor *TimeMethod::createNewUffFile(const QString &fileName, FileDescriptor *dfd, Parameters &p)
{DD;
    UffFileDescriptor *newUff = new UffFileDescriptor(fileName);

    newUff->updateDateTimeGUID();

    if (!dfd->dataDescriptor().isEmpty()) {
        newUff->setDataDescriptor(dfd->dataDescriptor());
    }

    const double newSampleRate = p.sampleRate / pow(2.0, p.bandStrip);
    newUff->setXStep(newSampleRate / p.bufferSize);

    return newUff;
}

DfdChannel *TimeMethod::createDfdChannel(DfdFileDescriptor *newDfd, FileDescriptor *dfd, const QVector<double> &spectrum, Parameters &p, int i)
{DD;
    Q_UNUSED(p);
    DfdChannel *ch = new DfdChannel(newDfd, newDfd->channelsCount());
    ch->data()->setXValues(0.0, newDfd->XStep, spectrum.size());
    ch->data()->setYValues(spectrum, DataHolder::YValuesReals);
    ch->setPopulated(true);
    ch->setName(dfd->channel(i)->name());

    ch->ChanDscr = dfd->channel(i)->description();
//    ch->ChanAddress = dfd->channel(i)->ChanAddress;

    ch->ChanBlockSize = spectrum.size();
    ch->IndType = 3221225476;

    ch->YName = dfd->channel(i)->yName();
    ch->YNameOld = dfd->channel(i)->yName();

    newDfd->channels << ch;
    return ch;
}

Function *TimeMethod::addUffChannel(UffFileDescriptor *newUff, FileDescriptor *dfd, int spectrumSize, Parameters &p, int i)
{DD;
    Function *ch = new Function(newUff);
    ch->setName(dfd->channel(i)->name());
    ch->setPopulated(true);

    //FunctionHeader header;
    ch->header.type1858[12].value = uffWindowType(p.windowType);


    ch->type58[8].value = QDateTime::currentDateTime();

    // строка 1
    ch->type58[14].value = uffMethodFromDfdMethod(id());
    ch->type58[15].value = i+1;
    //ch->type58[18].value = dfd->channels[i]->name(); //18  Response Entity Name ("NONE" if unused)
    ch->type58[18].value = QString("p%1").arg(i+1);
    ch->type58[20].value = 3; //20 Response Direction +Z
    //ch->type58[21].value = dfd->channels[p.baseChannel]->name(); //18  Reference Entity Name ("NONE" if unused)
    ch->type58[21].value = p.baseChannel>=0?QString("p%1").arg(p.baseChannel+1):"NONE";
    ch->type58[23].value = 0; //20 Reference Direction +Z

    // строка 2
    ch->type58[25].value = p.saveAsComplex ? 5 : 2; //25 Ordinate Data Type
    ch->type58[26].value = spectrumSize;
    ch->type58[27].value = 1; //27 Abscissa Spacing (1=even, 0=uneven,
    ch->type58[28].value = 0.0;
    double newSampleRate = p.sampleRate / pow(2.0, p.bandStrip);
    double XStep = newSampleRate / p.bufferSize;
    ch->type58[29].value = XStep; //29 Abscissa increment

    ch->data()->setXValues(0, XStep, spectrumSize);

    // строка 3
    ch->type58[32].value = 17; // 17 - Time
    ch->type58[36].value = "Время";
    ch->type58[37].value = "с";

    // строка 4
    ch->type58[39].value = 1; //39 Ordinate (or ordinate numerator) Data Characteristics
    ch->type58[44].value = dfd->channel(i)->yName();


    ch->type58[53].value = 1;
    ch->type58[57].value = "NONE";
    ch->type58[58].value = "NONE";

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
