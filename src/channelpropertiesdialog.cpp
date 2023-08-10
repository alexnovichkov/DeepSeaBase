#include "channelpropertiesdialog.h"
#include "fileformats/filedescriptor.h"
#include <QtWidgets>

#include "settings.h"
#include "logging.h"

ChannelPropertiesDialog::ChannelPropertiesDialog(const QVector<Channel *> &channels, QWidget *parent)
    : QDialog(parent), channels(channels)
{DD;
    setWindowTitle("Свойства каналов");

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    prevButton = buttonBox->addButton("←", QDialogButtonBox::ActionRole);
    prevButton->setEnabled(false);
    connect(prevButton, &QAbstractButton::clicked, this, &ChannelPropertiesDialog::prev);
    nextButton = buttonBox->addButton("→", QDialogButtonBox::ActionRole);
    nextButton->setEnabled(false);
    connect(nextButton, &QAbstractButton::clicked, this, &ChannelPropertiesDialog::next);
    applyToCurrButton = buttonBox->addButton("Применить к текущему", QDialogButtonBox::ActionRole);
    applyToCurrButton->setFocusPolicy(Qt::NoFocus);
    connect(applyToCurrButton, &QAbstractButton::clicked, this, &ChannelPropertiesDialog::applyToCurrent);
    applyToAllButton = buttonBox->addButton("Применить ко всем", QDialogButtonBox::ActionRole);
    applyToAllButton->setFocusPolicy(Qt::NoFocus);
    connect(applyToAllButton, &QAbstractButton::clicked, this, &ChannelPropertiesDialog::applyToAll);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    QVBoxLayout *l = new QVBoxLayout;
    l->addWidget(splitter);
    l->addWidget(buttonBox);

    channelsTable = new QTreeWidget(this);
    channelsTable->setRootIsDecorated(false);
    channelsTable->setColumnCount(1);
    channelsTable->setHeaderLabel("Каналы");
    channelsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(channelsTable, &QTreeWidget::currentItemChanged, this, &ChannelPropertiesDialog::currentChannelChanged);

    fillFiles();
    splitter->addWidget(channelsTable);


    QTabWidget *tab = new QTabWidget(this);
    QWidget *properties = new QWidget(this);
    QFormLayout *propertiesFL = new QFormLayout;
    dataProperties = {{"Число отсчетов", nullptr},
                      {"Число блоков", nullptr},
                      {"Частота дискретизации", nullptr},
                      {"Ширина полосы", nullptr},
                      {"Формат данных", nullptr},
                      {"Формат отсчета", nullptr}};
    channelProperties = {{"Название", "name", nullptr,  nullptr},
                               {"Описание", "description", nullptr,  nullptr},
                               {"Датчик","sensorName", nullptr,  nullptr},
                               {"ID датчика", "sensorID", nullptr,  nullptr},
                               {"Дата и время записи", "dateTime",  nullptr, nullptr},
                               {"Ось X", "xname", nullptr,  nullptr},
                               {"Ось Y","yname", nullptr,  nullptr},
                               {"Ось Y (исходная)","ynameold",  nullptr, nullptr},
                               {"Ось Z","zname", nullptr,  nullptr}};

    for (int i=0; i<channelProperties.size(); ++i) {
        addProperty(propertiesFL, channelProperties[i]);
    }
    auto line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    propertiesFL->addRow(line);
    for (int i=0; i<dataProperties.size(); ++i) {
        dataProperties[i].label = new QLineEdit(this);
        dataProperties[i].label->setReadOnly(true);
        dataProperties[i].label->setFrame(false);
        propertiesFL->addRow(dataProperties.at(i).displayName, dataProperties.at(i).label);
    }
    properties->setLayout(propertiesFL);
    QScrollArea *scroll1 = new QScrollArea(this);
    scroll1->setWidget(properties);
    scroll1->setWidgetResizable(true);
    tab->addTab(scroll1, "Свойства");
    scroll1->setFrameShape(QFrame::NoFrame);

    QWidget *descriptions = new QWidget(this);
    descriptionsLayout = new QFormLayout;

    auto addDescr = new QToolButton(this);
    addDescr->setText("Добавить");
    addDescr->setAutoRaise(true);
    connect(addDescr, &QToolButton::clicked, [=]() {
        QString key = QInputDialog::getText(this, "Добавить описатель", "Название:");
        if (!key.isEmpty()) {
            descriptionProperties.append({key, "description."+key, nullptr, nullptr});
            auto &p = descriptionProperties.last();
            addProperty(descriptionsLayout, p);
        }
    });
//    auto removeDescr = new QToolButton(this);
//    removeDescr->setText("Удалить");
//    removeDescr->setAutoRaise(true);
//    connect(removeDescr, &QToolButton::clicked, [=]() {
//        for (int i=0; i<descriptionProperties.size(); ++i) {
//            auto &p = descriptionProperties[i];
//            if (p.edit->hasFocus()) {

//                break;
//            }
//        }
//        QString key =
//        if (!key.isEmpty()) {
//            descriptionProperties.append({key, "description."+key, nullptr, nullptr});
//            auto &p = descriptionProperties.last();
//            addProperty(descriptionsLayout, p);
//        }
//    });

    auto ll = new QHBoxLayout;
    ll->addWidget(addDescr);
//    ll->addWidget(removeDescr);
    ll->addStretch();
    auto lll = new QVBoxLayout;
    lll->addLayout(descriptionsLayout);
    lll->addStretch();
    lll->addLayout(ll);
    descriptions->setLayout(lll);
    auto scroll2 = new QScrollArea(this);
    scroll2->setWidget(descriptions);
    scroll2->setWidgetResizable(true);
    scroll2->setFrameShape(QFrame::NoFrame);
    tab->addTab(scroll2, "Описатели");


    QWidget *function = new QWidget(this);
    functionLayout = new QFormLayout(this);
    function->setLayout(functionLayout);
    auto scroll3 = new QScrollArea(this);
    scroll3->setWidget(function);
    scroll3->setWidgetResizable(true);
    scroll3->setFrameShape(QFrame::NoFrame);
    tab->addTab(scroll3, "Функция");




    int currentTab = se->getSetting("channelPropertiesDialog.currentTab").toInt();
    tab->setCurrentIndex(currentTab);
    connect(tab, &QTabWidget::currentChanged, [=](int index){
        se->setSetting("channelPropertiesDialog.currentTab", index);
    });

    splitter->addWidget(tab);

    setLayout(l);
    resize(qApp->primaryScreen()->availableSize().width()/2,
           qApp->primaryScreen()->availableSize().height()/2);
}

void ChannelPropertiesDialog::fillFiles()
{DD;
    for(auto f: channels) {
        new QTreeWidgetItem(channelsTable, {f->name()});
    }
}

void ChannelPropertiesDialog::prev()
{
    auto it = channelsTable->currentItem();
    int index = channelsTable->indexOfTopLevelItem(it);
    if (index > 0) {
        channelsTable->setCurrentItem(channelsTable->itemAbove(it));
    }

}

void ChannelPropertiesDialog::next()
{
    auto it = channelsTable->currentItem();
    int index = channelsTable->indexOfTopLevelItem(it);
    if (index < channels.size()-1) {
        channelsTable->setCurrentItem(channelsTable->itemBelow(it));
    }
}

void ChannelPropertiesDialog::applyToCurrent()
{
    Channel *c = channels.at(current);

    auto list = channelProperties;
    list.append(functionProperties);
    list.append(descriptionProperties);

    for (auto &p: list) {
        if (p.check->isChecked()) {
            if (c->dataDescription().get(p.name).toString() != p.edit->text()) {
                c->dataDescription().put(p.name, p.edit->text());
                c->setChanged(true);
                c->descriptor()->setChanged(true);
            }
        }
    }
}

void ChannelPropertiesDialog::applyToAll()
{
    auto list = channelProperties;
    list.append(functionProperties);
    list.append(descriptionProperties);

    for (auto &c: channels) {
        for (auto &p: list) {
            if (p.check->isChecked()) {
                if (c->dataDescription().get(p.name).toString() != p.edit->text()) {
                    c->dataDescription().put(p.name, p.edit->text());
                    c->setChanged(true);
                    c->descriptor()->setChanged(true);
                }
            }
        }
    }
}

void ChannelPropertiesDialog::currentChannelChanged(QTreeWidgetItem *cur, QTreeWidgetItem *previous)
{DD;
    auto font = channelsTable->font();

    if (previous) previous->setFont(0, font);
    font.setBold(true);
    cur->setFont(0, font);


    if (cur) {
        current = channelsTable->indexOfTopLevelItem(cur);
        nextButton->setEnabled(current < channels.size()-1);
        prevButton->setEnabled(current > 0);

        QString s;
        Channel *d = channels.at(current);


        channelProperties.at(0).edit->setText(d->name());
        channelProperties.at(1).edit->setText(d->description());
        channelProperties.at(2).edit->setText(d->dataDescription().get("sensorName").toString());
        channelProperties.at(3).edit->setText(d->dataDescription().get("sensorID").toString());
        QDateTime dt = d->dataDescription().dateTime("dateTime");
        if (dt.isValid())
            s = dt.toString("d MMMM yyyy, hh:mm:ss");
        else
            s.clear();
        channelProperties.at(4).edit->setText(s);
        channelProperties.at(5).edit->setText(d->dataDescription().get("xname").toString());
        channelProperties.at(6).edit->setText(d->dataDescription().get("yname").toString());
        channelProperties.at(7).edit->setText(d->dataDescription().get("ynameold").toString());
        channelProperties.at(8).edit->setText(d->dataDescription().get("zname").toString());

        dataProperties.at(0).label->setText(QString::number(d->data()->samplesCount()));
        dataProperties.at(1).label->setText(QString::number(d->data()->blocksCount()));
        {
            const double sr = d->dataDescription().get("samplerate").toDouble();
            if (qFuzzyIsNull(sr)) dataProperties.at(2).label->setText("нет данных");
            else dataProperties.at(2).label->setText(QString::number(sr));
        }
        {
            const double bw = d->dataDescription().get("bandwidth").toDouble();
            if (qFuzzyIsNull(bw)) dataProperties.at(3).label->setText("нет данных");
            else dataProperties.at(3).label->setText(QString::number(bw));
        }
        dataProperties.at(4).label->setText(d->dataDescription().get("function.format").toString());
        dataProperties.at(5).label->setText(d->dataDescription().get("function.precision").toString());

        {
            auto data = d->dataDescription().filter("function");
            data.remove("format");
            data.remove("precision");

            while (functionLayout->rowCount()>0) functionLayout->removeRow(0);
            functionProperties.clear();
            for (const auto [key, val]: asKeyValueRange(data)) {
                Q_UNUSED(val);
                functionProperties.append({displayName(key), "function."+key, nullptr, nullptr});
            }
            for (int i=0; i<functionProperties.size(); ++i) {
                auto &p = functionProperties[i];
                addProperty(functionLayout, p);
            }
        }
        {
            auto data = d->dataDescription().filter("description");

            while (descriptionsLayout->rowCount()>0) descriptionsLayout->removeRow(0);
            descriptionProperties.clear();
            for (const auto [key, val]: asKeyValueRange(data)) {
                Q_UNUSED(val);
                descriptionProperties.append({key, "description."+key, nullptr, nullptr});
            }
            for (int i=0; i<descriptionProperties.size(); ++i) {
                auto &p = descriptionProperties[i];
                addProperty(descriptionsLayout, p);
            }
        }
    }
    else {
        current = -1;
        for (auto l: channelProperties) l.edit->clear();
        for (auto l: dataProperties) l.label->clear();
    }
}

void ChannelPropertiesDialog::addProperty(QFormLayout *l, ChannelProperty &p)
{
    Channel *d = channels.at(current);
    p.edit = new QLineEdit(d->dataDescription().get(p.name).toString(), this);
    p.edit->setEnabled(false);
    p.check = new QCheckBox(this);
    connect(p.check, &QCheckBox::stateChanged, [=](int state){
        p.edit->setEnabled(state == Qt::Checked);
    });

    QHBoxLayout *ll = new QHBoxLayout;
    ll->addWidget(p.check);
    ll->addWidget(p.edit);
    l->addRow(p.displayName, ll);
}

QString ChannelPropertiesDialog::displayName(const QString &property)
{
    if (property == "name") return "Название";
    if (property == "type") return "Тип";
    if (property == "description") return "Описание";
    if (property == "logref") return "Пороговое значение";
    if (property == "logscale") return "Шкала";
    if (property == "responseName") return "Название канала отклика";
    if (property == "responseDirection") return "Направление канала отклика";
    if (property == "responseNode") return "Номер канала отклика";
    if (property == "referenceName") return "Название опорного канала";
    if (property == "referenceNode") return "Номер опорного канала";
    if (property == "referenceDescription") return "Описание опорного канала";
    if (property == "referenceDirection") return "Направление опорного канала";
    if (property == "format") return "Формат данных";
    if (property == "precision") return "Точность данных";
    if (property == "octaveFormat") return "Тип октавы";
    if (property == "weighting") return "Взвешивание";
    if (property == "averaging") return "Усреднение";
    if (property == "averagingCount") return "Количество усреднений";
    if (property == "window") return "Окно";
    if (property == "windowParameter") return "Параметр окна";
    if (property == "blockSize") return "Размер блока";
    if (property == "channels") return "Каналы источника";
    if (property == "frameCutterType") return "Выборка блоков";
    if (property == "windowCorrection") return "Коррекция окна";
    return property;
}

