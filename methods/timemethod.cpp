#include "timemethod.h"

#include <QtWidgets>
#include "dfdfiledescriptor.h"

TimeMethod::TimeMethod(QWidget *parent) : QWidget(parent)
{
    resolutionCombo = new QComboBox(this);
    resolutionCombo->addItem("512");
    resolutionCombo->addItem("1024");
    resolutionCombo->addItem("2048");
    resolutionCombo->addItem("4096");
    resolutionCombo->addItem("8192");
    resolutionCombo->setCurrentIndex(2);
    resolutionCombo->setEditable(false);

    QFormLayout *l = new QFormLayout;
//    l->addRow("Частотный диапазон", rangeCombo);
    l->addRow("Разрешение по частоте", resolutionCombo);
    setLayout(l);
}

int TimeMethod::id()
{
    return 0;
}

QStringList TimeMethod::methodSettings(DfdFileDescriptor *dfd, int activeChannel, int strip)
{
    QStringList spfFile;

    spfFile << "YName="+dfd->channels.at(activeChannel)->YName;
    spfFile << "BlockIn=1024";
    spfFile << "TypeProc=0";
    spfFile << "Values=измеряемые";

    quint32 numberOfInd = dfd->channels.at(activeChannel)->samplesCount();
    double resolution = resolutionCombo->currentText().toDouble();
    double NumberOfAveraging = double(numberOfInd) / resolution;
    while (strip>0) {
        NumberOfAveraging /= 2.0;
        strip--;
    }

    // at least 2 averaging
    if (NumberOfAveraging<1) NumberOfAveraging = 2.0;

    spfFile << QString("NAver=%1").arg(qRound(NumberOfAveraging));

    return spfFile;
}

QStringList TimeMethod::settings(DfdFileDescriptor *dfd, int bandStrip)
{
    return methodSettings(dfd,1,bandStrip);
}

Parameters TimeMethod::parameters()
{
    Parameters p;
    p.blockSize = resolutionCombo->currentText().toInt();

    p.panelType = panelType();
    p.methodName = methodName();
    p.methodDll = methodDll();
    p.dataType = dataType();

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
