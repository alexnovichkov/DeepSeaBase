#include "channelpropertiesdialog.h"
#include "fileformats/filedescriptor.h"
#include <QtWidgets>
#include "app.h"
#include "logging.h"

ChannelPropertiesDialog::ChannelPropertiesDialog(const QVector<Channel *> &channels, QWidget *parent)
    : QDialog(parent), channels(channels)
{DD;
    setWindowTitle("Свойства каналов");

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
//    prevButton = buttonBox->addButton("Предыдущий", QDialogButtonBox::ActionRole);
//    connect(prevButton, &QAbstractButton::clicked, this, &DescriptorPropertiesDialog::prev);
//    nextButton = buttonBox->addButton("Следующий", QDialogButtonBox::ActionRole);
//    connect(nextButton, &QAbstractButton::clicked, this, &DescriptorPropertiesDialog::next);
//    applyToCurrButton = buttonBox->addButton("Применить к текущему", QDialogButtonBox::ActionRole);
//    connect(applyToCurrButton, &QAbstractButton::clicked, this, &DescriptorPropertiesDialog::applyToCurrent);
//    applyToAllButton = buttonBox->addButton("Применить ко всем", QDialogButtonBox::ActionRole);
//    connect(applyToAllButton, &QAbstractButton::clicked, this, &DescriptorPropertiesDialog::applyToAll);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    QVBoxLayout *l = new QVBoxLayout;
    l->addWidget(splitter);
    l->addWidget(buttonBox);

    channelsTable = new QTreeWidget(this);
    channelsTable->setAlternatingRowColors(true);
    channelsTable->setRootIsDecorated(false);
    channelsTable->setColumnCount(1);
    channelsTable->setHeaderLabel("Каналы");
//    channelsTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    channelsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    //files->setItemDelegateForColumn(0, new FilesItemDelegate());
    connect(channelsTable, &QTreeWidget::currentItemChanged, this, &ChannelPropertiesDialog::currentChannelChanged);
    //connect(channelsTable, &QTreeWidget::itemSelectionChanged, this, &ChannelPropertiesDialog::selectedChannelsChanged);

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
    channelProperties = {{"Название", "name", nullptr},
                               {"Описание", "description", nullptr},
                               {"Датчик","sensorName", nullptr},
                               {"ID датчика", "sensorID", nullptr},
                               {"Дата и время записи", "dateTime", nullptr},
                               {"Ось X", "xname", nullptr},
                               {"Ось Y","yname", nullptr},
                               {"Ось Y (исходная)","ynameold", nullptr},
                               {"Ось Z","zname", nullptr}};
    for (int i=0; i<channelProperties.size(); ++i) {
        channelProperties[i].edit = new QLineEdit(this);

        connect(channelProperties[i].edit, &QLineEdit::editingFinished, [=]() {
            if (current < 0 || current >= channels.size()
                || currentEdited.index < 0 || currentEdited.index != i) return;

            QList<Channel*> selected;
            //заблокировано изменение в нескольких каналах
//            selected = selectedChannels();
//            if (selected.isEmpty())
                selected << channels.at(current);
            if (selected.size()>1) {
                QMessageBox box;
                box.setText(QString("Значение \"%1\" будет записано во все \nвыделенные каналы").arg(currentEdited.value));
                box.setInformativeText("Продолжить?");
                box.setStandardButtons(QMessageBox::Yes | QMessageBox::No);

                bool alreadyShown = App->getSetting("alreadyShown1", false).toBool();
                bool alreadyShownState = App->getSetting("alreadyShownState1", false).toBool();
                if (!alreadyShown) {
                    QCheckBox *cb = new QCheckBox("Больше не спрашивать");
                    cb->setChecked(false);
                    box.setCheckBox(cb);
                }

                int res = box.exec();
                if (box.checkBox()->isChecked()) {
                    App->setSetting("alreadyShown1", true);
                    App->setSetting("alreadyShownState1", res == QMessageBox::Yes);
                }
                if (res != QMessageBox::Yes) return;
                if (alreadyShown && !alreadyShownState) return; //ранее выбрали Всегда нет
            }

            for (auto c: selected) {
                if (c->dataDescription().get(channelProperties.at(i).name).toString() != currentEdited.value) {
                    c->dataDescription().put(channelProperties.at(i).name, currentEdited.value);
                    c->setChanged(true);
                    c->descriptor()->setChanged(true);
                }
            }
        });
        connect(channelProperties.at(i).edit, &QLineEdit::textEdited, [=](const QString &newVal) {
            if (current < 0 || current >= channels.size()) return;
            currentEdited.index = i;
            currentEdited.value = newVal;
        });
        if (i==4 || i==7) {
            channelProperties.at(i).edit->setReadOnly(true);
            channelProperties.at(i).edit->setFrame(false);
        }
//        else {
//            channelProperties.at(i).edit->setClearButtonEnabled(true);
//        }
        propertiesFL->addRow(channelProperties.at(i).displayName, channelProperties.at(i).edit);
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


//    descriptionsTable = new QTableWidget(this);
//    descriptionsTable->setColumnCount(1);
//    descriptionsTable->setHorizontalHeaderLabels({"Параметр", "Значение"});
//    descriptionsTable->horizontalHeader()->setStretchLastSection(true);

//    QVBoxLayout *descriptionsL = new QVBoxLayout;
//    descriptionsL->setContentsMargins(0,0,0,0);
//    descriptionsL->addWidget(descriptionsTable);
//    descriptions->setLayout(descriptionsL);

    descriptionsLayout = new QFormLayout(this);
    descriptions->setLayout(descriptionsLayout);
    auto scroll2 = new QScrollArea(this);
    scroll2->setWidget(descriptions);
    scroll2->setWidgetResizable(true);
    scroll2->setFrameShape(QFrame::NoFrame);
    tab->addTab(scroll2, "Функция");
    int currentTab = App->getSetting("channelPropertiesDialog.currentTab").toInt();
    tab->setCurrentIndex(currentTab);
    connect(tab, &QTabWidget::currentChanged, [=](int index){
        App->setSetting("channelPropertiesDialog.currentTab", index);
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

void ChannelPropertiesDialog::currentChannelChanged(QTreeWidgetItem *cur, QTreeWidgetItem *previous)
{DD;
    Q_UNUSED(previous);
//    static QStringList dataPropertiesNames {"Число отсчетов", "Число блоков", "Частота дискретизации",
//                                        "Ширина полосы", "Формат данных", "Формат отсчета"};
//    static QStringList channelPropertiesNames {"Название", "Описание", "Датчик",
//                                        "ID датчика", "Дата и время записи", "Ось X",
//                                              "Ось Y","Ось Y (исходная)","Ось Z"};


    if (cur) {
        current = channelsTable->indexOfTopLevelItem(cur);
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

        auto data = d->dataDescription().filter("function");
        data.remove("format");
        data.remove("precision");
//        descriptionsTable->clearContents();
//        descriptionsTable->setRowCount(data.size());
//        int i=0;
//        for (auto [key, val]: asKeyValueRange(data)) {
//            if (auto item = descriptionsTable->verticalHeaderItem(i))
//                item->setText(key);
//            else
//                descriptionsTable->setVerticalHeaderItem(i, new QTableWidgetItem(key));
//            if (auto item = descriptionsTable->item(i,0))
//                item->setText(val.toString());
//            else
//                descriptionsTable->setItem(i,0, new QTableWidgetItem(val.toString()));
//            i++;
//        }

        while (descriptionsLayout->rowCount()>0) descriptionsLayout->removeRow(0);
//        int i=0;
        for (const auto [key, val]: asKeyValueRange(data)) {
            auto l = new QLineEdit(val.toString(), this);
            l->setReadOnly(true);
            l->setFrame(false);
            descriptionsLayout->addRow(key, l);
//            if (auto item = descriptionsTable->verticalHeaderItem(i))
//                item->setText(key);
//            else
//                descriptionsTable->setVerticalHeaderItem(i, new QTableWidgetItem(key));
//            if (auto item = descriptionsTable->item(i,0))
//                item->setText(val.toString());
//            else
//                descriptionsTable->setItem(i,0, new QTableWidgetItem(val.toString()));
//            i++;
        }
    }
    else {
        current = -1;
        for (auto l: channelProperties) l.edit->clear();
        for (auto l: dataProperties) l.label->clear();

        descriptionsTable->clearContents();
    }
}

void ChannelPropertiesDialog::selectedChannelsChanged()
{DD;

}

QList<Channel *> ChannelPropertiesDialog::selectedChannels()
{DD;
    QMap<int, Channel *> result;
    for (auto item: channelsTable->selectedItems()) {
        auto i = channelsTable->indexOfTopLevelItem(item);
        result.insert(i, channels.at(i));
    }
    return result.values();
}
