#include "octavemethod.h"
#include <QtWidgets>

#include "fileformats/dfdfiledescriptor.h"
#include "fileformats/ufffile.h"
#include "logging.h"
#include "algorithms.h"

OctaveMethod::OctaveMethod(QList<FileDescriptor *> &dataBase, QWidget *parent) :
    QWidget(parent), AbstractMethod(dataBase)
{
    resolutionSpin = new QSpinBox(this); //kStrip=1024
    resolutionSpin->setRange(1, 500000);
    resolutionSpin->setValue(1024);
    resolutionSpin->setReadOnly(false);

    placeCombo = new QComboBox(this); //AddProc = 0/1/2/3
    placeCombo->addItem("все отсчеты");
    placeCombo->addItem("начало интервала");
    placeCombo->addItem("конец интервала");
    placeCombo->addItem("синхронные");
    placeCombo->setCurrentIndex(0);
    placeCombo->setEditable(false);

    typeCombo = new QComboBox(this); //TypeProc
    typeCombo->addItem("1/3-октава");
    typeCombo->addItem("октава");
    typeCombo->setCurrentIndex(0);
    typeCombo->setEditable(false);

    valuesCombo = new QComboBox(this); //Values
    valuesCombo->addItem("измеряемые");
    valuesCombo->addItem("вход АЦП");
    valuesCombo->setCurrentIndex(0);
    valuesCombo->setEnabled(false);
    valuesCombo->setEditable(false);

    scaleCombo = new QComboBox(this); //TypeScale
    scaleCombo->addItem("линейная");
    scaleCombo->addItem("в децибелах");
    scaleCombo->setCurrentIndex(1);
//    scaleCombo->setEnabled(false);
    scaleCombo->setEditable(false);

    QFormLayout *l = new QFormLayout;
    l->addRow("Мин. количество отсчетов СКЗ", resolutionSpin);
    l->addRow("Выбор отсчетов СКЗ", placeCombo);
    l->addRow("Тип спектра", typeCombo);
    l->addRow("Величины", valuesCombo);
    l->addRow("Шкала", scaleCombo);
    setLayout(l);
}

int OctaveMethod::id()
{
    return 18;
}

QStringList OctaveMethod::methodSettings(FileDescriptor *dfd, const Parameters &p)
{
    Q_UNUSED(p)
    QStringList spfFile;
    QString yName = "дБ";
    if (scaleCombo->currentText() != "в децибелах") {
        yName = dfd->channel(0)->yName();
    }
    spfFile << QString("YName=%1").arg(yName);
    spfFile << QString("BlockIn=32768"); //Размер буфера чтения, кажется, не меняется
    spfFile << QString("NAver=1"); //количество усреднений, не меняется
    spfFile << "TypeProc="+typeCombo->currentText();
    spfFile << "Values="+valuesCombo->currentText();
    spfFile << "TypeScale="+scaleCombo->currentText();
    spfFile << QString("kStrip=%1").arg(resolutionSpin->value());
    spfFile << QString("AddProc=%1").arg(placeCombo->currentIndex());

    return spfFile;
}

Parameters OctaveMethod::parameters()
{
    Parameters p;

    p.bufferSize = 0;
    p.scaleType = scaleCombo->currentIndex();

    return p;
}

QString OctaveMethod::methodDll()
{
    return "octSpect8.dll";
}

int OctaveMethod::panelType()
{
    return 0;
}

QString OctaveMethod::methodName()
{
    if (typeCombo->currentText()=="1/3-октава") return "1/3-октавный спектр";
    if (typeCombo->currentText()=="октава") return "Октавный спектр";
    return "Октавный спектр";
}

int OctaveMethod::dataType()
{
    if (typeCombo->currentText()=="1/3-октава") return 157;
    if (typeCombo->currentText()=="октава") return 156;
    return 156;
}

DescriptionList OctaveMethod::processData(const Parameters &p)
{
    Q_UNUSED(p)
    DescriptionList list;
    list.append({"PName", methodName()});
    list.append({"pTime","(0000000000000000)"});
    //list.append({"ProcChansList",""});
    list.append({"BlockIn", "32768"});
    list.append({"TypeProc", typeCombo->currentText()});
    list.append({"Values", valuesCombo->currentText()});
    list.append({"TypeScale", scaleCombo->currentText()});
    list.append({"kStrip", QString::number(resolutionSpin->value())});
    list.append({"AddProc", QString::number(placeCombo->currentIndex())});

    return list;
}



DfdFileDescriptor *OctaveMethod::createNewDfdFile(const QString &fileName, FileDescriptor *dfd, Parameters &p)
{DD;
    DfdFileDescriptor *newDfd = AbstractMethod::createNewDfdFile(fileName, dfd, p);

    // rest
    newDfd->XName = "Гц";
    newDfd->XStep = 0;
    newDfd->XBegin = 0.0;

    return newDfd;
}

UffFileDescriptor *OctaveMethod::createNewUffFile(const QString &fileName, FileDescriptor *dfd, Parameters &p)
{
    Q_UNUSED(p);
    UffFileDescriptor *newUff = new UffFileDescriptor(fileName);

    newUff->updateDateTimeGUID();

    if (!dfd->dataDescriptor().isEmpty()) {
        newUff->setDataDescriptor(dfd->dataDescriptor());
    }

    newUff->setXStep(0.0);

    return newUff;
}

DfdChannel *OctaveMethod::createDfdChannel(DfdFileDescriptor *newDfd, FileDescriptor *dfd, const QVector<double> &spectrum, Parameters &p, int i)
{
    Q_UNUSED(p);
    DfdChannel *ch = new DfdChannel(newDfd, newDfd->channelsCount());
//    ch->data()->setXValues(XStep = newDfd->XStep;
    ch->data()->setThreshold(p.threshold);
    ch->data()->setYValues(spectrum, DataHolder::YValuesAmplitudesInDB);
    ch->setPopulated(true);
    ch->setName(dfd->channel(i)->name());

    ch->ChanDscr = dfd->channel(i)->description();
//    ch->ChanAddress = dfd->channels[i]->ChanAddress;

    ch->ChanBlockSize = spectrum.size();
    ch->IndType = 3221225476;

    ch->YName = "дБ";
    ch->YNameOld = dfd->channel(i)->yName();

    newDfd->channels << ch;
    return ch;
}

Function *OctaveMethod::addUffChannel(UffFileDescriptor *newUff, FileDescriptor *dfd, int spectrumSize, Parameters &p, int i)
{
    Function *ch = new Function(newUff);
    ch->setName(dfd->channel(i)->name()/*+"/Сила"*/);
    ch->setPopulated(true);

    //FunctionHeader header;
    ch->header.type1858[12].value = uffWindowType(p.windowType);
    ch->header.type1858[5].value = 3; // 1/3-октава


    ch->type58[8].value = QDateTime::currentDateTime();;
    ch->type58[14].value = 12; // 12 = Spectrum
    ch->type58[15].value = i+1;
    //ch->type58[18].value = dfd->channels[i]->name(); //18  Response Entity Name ("NONE" if unused)
    ch->type58[18].value = QString("p%1").arg(i+1);
    ch->type58[20].value = 3; //20 Response Direction +Z
    //ch->type58[21].value = dfd->channels[p.baseChannel]->name(); //18  Reference Entity Name ("NONE" if unused)

    ch->type58[25].value = p.saveAsComplex ? 5 : 2; //25 Ordinate Data Type
    ch->type58[26].value = spectrumSize;
    ch->type58[28].value = 0.0;
    ch->type58[29].value = 0.0; //29 Abscissa increment
    ch->type58[30].value = dfd->channel(i)->samplesCount()*dfd->channel(i)->xStep(); //30 Z-axis value (length in seconds)

    ch->type58[32].value = 18; // 18 - frequency
    ch->type58[36].value = "Частота";
    ch->type58[37].value = "Гц";

    ch->type58[39].value = 1; //39 Ordinate (or ordinate numerator) Data Characteristics // 1 = General
    ch->type58[44].value = "дБ"; // 44 Ordinate Axis units label ("NONE" if not used)

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

    newUff->channels << ch;
    return ch;
}
