#include "spectremethod.h"

#include <QtWidgets>

#include "dfdfiledescriptor.h"

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
   // nAverCombo->setEditable(false);
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
    spfFile << "TypeAver="+averCombo->currentText();

    quint32 numberOfInd = dfd->channels.at(activeChannel)->samplesCount();
    double blockSize = resolutionCombo->currentText().toDouble();
    double NumberOfAveraging = double(numberOfInd) / blockSize;

    while (strip>0) {
        NumberOfAveraging /= 2.0;
        strip--;
    }

    // at least 2 averaging
    if (NumberOfAveraging<1) NumberOfAveraging = 2.0;

    spfFile << QString("NAver=%1").arg(qRound(NumberOfAveraging));
    spfFile << "TypeProc="+typeCombo->currentText();
    spfFile << "Values="+valuesCombo->currentText();
    spfFile << "TypeScale="+scaleCombo->currentText();
    spfFile << "AddProc="+addProcCombo->currentText();

    return spfFile;
}

QStringList SpectreMethod::settings(DfdFileDescriptor *dfd, int strip)
{
    QStringList spfFile;

    spfFile << "PName="+methodName();
    spfFile << QString("BlockIn=%1").arg(resolutionCombo->currentText());
    spfFile << "Wind="+windowCombo->currentText();
    spfFile << "TypeAver="+averCombo->currentText();
    spfFile << "pTime=(0000000000000000)";

    const quint32 samplesCount = dfd->channels.first()->samplesCount();
    const double blockSize = resolutionCombo->currentText().toDouble();
    double NumberOfAveraging = double(samplesCount) / blockSize;

    while (strip>0) {
        NumberOfAveraging /= 2.0;
        strip--;
    }

    // at least 2 averaging
    if (NumberOfAveraging<1.0) NumberOfAveraging = 2.0;

    spfFile << QString("NAver=%1").arg(qRound(NumberOfAveraging));

    spfFile << "TypeProc="+typeCombo->currentText();
    spfFile << "Values="+valuesCombo->currentText();
    spfFile << "TypeScale="+scaleCombo->currentText();
    spfFile << "AddProc="+addProcCombo->currentText();

    return spfFile;
}

Parameters SpectreMethod::parameters()
{
    Parameters p;
    p.averagingType = averCombo->currentIndex();
    p.blockSize = resolutionCombo->currentText().toInt();
    p.windowType = windowCombo->currentIndex();
    p.scaleType = scaleCombo->currentIndex();

    p.panelType = panelType();
    p.methodName = methodName();
    p.methodDll = methodDll();
    p.dataType = dataType();

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

int SpectreMethod::computeNumberOfAveraging(const QString &aver)
{
    if (aver=="до конца интервала") {

    }
    return aver.toInt();
}
