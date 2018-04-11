#include "octavemethod.h"
#include <QtWidgets>

#include "dfdfiledescriptor.h"

OctaveMethod::OctaveMethod(QWidget *parent):
    QWidget(parent)
{
    resolutionSpin = new QSpinBox(this);
    resolutionSpin->setRange(1, 500000);
    resolutionSpin->setValue(1024);
    resolutionSpin->setReadOnly(false);

    placeCombo = new QComboBox(this);
    placeCombo->addItem("все отсчеты");
    placeCombo->addItem("начало интервала");
    placeCombo->addItem("конец интервала");
    placeCombo->addItem("синхронные");
    placeCombo->setCurrentIndex(0);
    placeCombo->setEditable(false);

    typeCombo = new QComboBox(this);
    typeCombo->addItem("1/3-октава");
    typeCombo->addItem("октава");
    typeCombo->setCurrentIndex(0);
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

//    addProcCombo = new QComboBox(this);
//    addProcCombo->addItem("нет");
//    addProcCombo->addItem("интегрир.");
//    addProcCombo->addItem("дифференц.");
//    addProcCombo->addItem("дв.интергир.");
//    addProcCombo->addItem("дв.дифференц.");
//    addProcCombo->setCurrentIndex(0);
//    addProcCombo->setEnabled(false);
//    addProcCombo->setEditable(false);

    QFormLayout *l = new QFormLayout;
    l->addRow("Мин. количество отсчетов СКЗ", resolutionSpin);
    l->addRow("Выбор отсчетов СКЗ", placeCombo);
    l->addRow("Тип спектра", typeCombo);
    l->addRow("Величины", valuesCombo);
    l->addRow("Шкала", scaleCombo);
    //l->addRow("Доп. обработка", addProcCombo);
    setLayout(l);
}

int OctaveMethod::id()
{
    return 18;
}

QStringList OctaveMethod::methodSettings(DfdFileDescriptor *dfd, const Parameters &p)
{

}

Parameters OctaveMethod::parameters()
{
    Parameters p;
    p.

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

QString OctaveMethod::methodDll()
{

}

int OctaveMethod::panelType()
{

}

QString OctaveMethod::methodName()
{

}

int OctaveMethod::dataType()
{

}

