#include "octavemethod.h"
#include <QtWidgets>

#include "dfdfiledescriptor.h"

OctaveMethod::OctaveMethod(QList<DfdFileDescriptor *> &dataBase, QWidget *parent) :
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

QStringList OctaveMethod::methodSettings(DfdFileDescriptor *dfd, const Parameters &p)
{
    Q_UNUSED(p)
    QStringList spfFile;
    QString yName = "дБ";
    if (scaleCombo->currentText() != "в децибелах") {
        yName = dfd->channels.first()->yName();
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


//    p.averagingType = averCombo->currentIndex();
    p.bufferSize = 32768;
//    p.windowType = windowCombo->currentIndex();
    p.scaleType = scaleCombo->currentIndex();
    p.panelType = panelType();
    p.methodName = methodName();
    p.methodDll = methodDll();
    p.dataType = dataType();

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
    //list.append({"ProcChansList",""}); // TODO
    list.append({"BlockIn", "32768"});
    list.append({"TypeProc", typeCombo->currentText()});
    list.append({"Values", valuesCombo->currentText()});
    list.append({"TypeScale", scaleCombo->currentText()});
    list.append({"kStrip", QString::number(resolutionSpin->value())});
    list.append({"AddProc", QString::number(placeCombo->currentIndex())});

    return list;
}

