#include "xresponch1.h"

#include <QtWidgets>

#include "dfdfiledescriptor.h"

XresponcH1Method::XresponcH1Method(QWidget *parent)
    : QWidget(parent)
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


    QFormLayout *l = new QFormLayout;
    //    l->addRow("Частотный диапазон", rangeCombo);
    l->addRow("Разрешение по частоте", resolutionCombo);
    l->addRow("Окно", windowCombo);
    l->addRow("Усреднение", averCombo);
    l->addRow("Кол. усреднений", nAverCombo);
    l->addRow("Величины", valuesCombo);
    l->addRow("Шкала", scaleCombo);
    setLayout(l);
}


int XresponcH1Method::id()
{
    return 9;
}

QStringList XresponcH1Method::methodSettings(DfdFileDescriptor *dfd, int activeChannel, int strip)
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
    spfFile << "Values="+valuesCombo->currentText();
    spfFile << "TypeScale="+scaleCombo->currentText();

    return spfFile;
}

QString XresponcH1Method::methodDll()
{
    return "xresponcH1.dll";
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
