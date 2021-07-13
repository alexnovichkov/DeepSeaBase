#include "channelpropertiesdialog.h"
#include "fileformats/filedescriptor.h"
#include <QtWidgets>

static struct EditableProperties
{
    QLineEdit* labels[9];
} channelPropertiesLabels;

static struct NonEditableProperties
{
    QLabel* labels[6];
} dataPropertiesLabels;

ChannelPropertiesDialog::ChannelPropertiesDialog(const QVector<Channel *> &channels, QWidget *parent)
    : QDialog(parent), channels(channels)
{
    setWindowTitle("Свойства каналов");

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                                       | QDialogButtonBox::Cancel);
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
    channelsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    //files->setItemDelegateForColumn(0, new FilesItemDelegate());
    connect(channelsTable, &QTreeWidget::currentItemChanged, this, &ChannelPropertiesDialog::currentChannelChanged);
    fillFiles();
    splitter->addWidget(channelsTable);

    QTabWidget *tab = new QTabWidget(this);
    QWidget *properties = new QWidget(this);
    tab->addTab(properties, "Свойства");
    QFormLayout *propertiesFL = new QFormLayout;
    static QStringList dataPropertiesNames {"Число отсчетов", "Число блоков", "Частота дискретизации",
                                        "Ширина полосы", "Формат данных", "Формат отсчета"};
    static QStringList channelPropertiesNames {"Название", "Описание", "Датчик",
                                        "ID датчика", "Дата и время записи", "Ось X",
                                              "Ось Y","Ось Y (исходная)","Ось Z"};
    for (int i=0; i<channelPropertiesNames.size(); ++i) {
        channelPropertiesLabels.labels[i] = new QLineEdit(this);
        if (i==4 || i==7)
            channelPropertiesLabels.labels[i]->setEnabled(false);
        propertiesFL->addRow(channelPropertiesNames[i], channelPropertiesLabels.labels[i]);
    }
    for (int i=0; i<dataPropertiesNames.size(); ++i) {
        dataPropertiesLabels.labels[i] = new QLabel(this);
        propertiesFL->addRow(dataPropertiesNames[i], dataPropertiesLabels.labels[i]);
    }
    properties->setLayout(propertiesFL);

    QWidget *descriptions = new QWidget(this);
    tab->addTab(descriptions, "Функция");

    descriptionsTable = new QTableWidget(this);
    descriptionsTable->setColumnCount(1);
    descriptionsTable->setHorizontalHeaderLabels({"Параметр", "Значение"});
    descriptionsTable->horizontalHeader()->setStretchLastSection(true);

    QVBoxLayout *descriptionsL = new QVBoxLayout;
    descriptionsL->setContentsMargins(0,0,0,0);
    descriptionsL->addWidget(descriptionsTable);
    descriptions->setLayout(descriptionsL);

    splitter->addWidget(tab);

    setLayout(l);
    resize(qApp->primaryScreen()->availableSize().width()/2,
           qApp->primaryScreen()->availableSize().height()/2);
}

void ChannelPropertiesDialog::fillFiles()
{
    for(auto f: channels) {
        new QTreeWidgetItem(channelsTable, {f->name()});
    }
}

void ChannelPropertiesDialog::currentChannelChanged(QTreeWidgetItem *cur, QTreeWidgetItem *previous)
{
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
        channelPropertiesLabels.labels[0]->setText(d->name());
        channelPropertiesLabels.labels[1]->setText(d->description());
        channelPropertiesLabels.labels[2]->setText(d->dataDescription().get("sensorName").toString());
        channelPropertiesLabels.labels[3]->setText(d->dataDescription().get("sensorID").toString());
        QDateTime dt = d->dataDescription().get("dateTime").toDateTime();
        if (dt.isValid() && !d->dataDescription().get("dateTime").toString().isEmpty())
            s = dt.toString("d MMMM yyyy, hh:mm:ss");
        else
            s.clear();
        channelPropertiesLabels.labels[4]->setText(s);
        channelPropertiesLabels.labels[5]->setText(d->dataDescription().get("xname").toString());
        channelPropertiesLabels.labels[6]->setText(d->dataDescription().get("yname").toString());
        channelPropertiesLabels.labels[7]->setText(d->dataDescription().get("ynameold").toString());
        channelPropertiesLabels.labels[8]->setText(d->dataDescription().get("zname").toString());

        dataPropertiesLabels.labels[0]->setText(QString::number(d->data()->samplesCount()));
        dataPropertiesLabels.labels[1]->setText(QString::number(d->data()->blocksCount()));
        dataPropertiesLabels.labels[2]->setText(QString::number(d->dataDescription().get("samplerate").toDouble()));
        dataPropertiesLabels.labels[3]->setText(QString::number(d->dataDescription().get("bandwidth").toDouble()));
        dataPropertiesLabels.labels[4]->setText(d->dataDescription().get("function.format").toString());
        dataPropertiesLabels.labels[5]->setText(d->dataDescription().get("function.precision").toString());

        auto data = d->dataDescription().filter("function");
        data.remove("format");
        data.remove("precision");
        descriptionsTable->clearContents();
        descriptionsTable->setRowCount(data.size());
        int i=0;
        for (auto [key, val]: asKeyValueRange(data)) {
            if (auto item = descriptionsTable->verticalHeaderItem(i))
                item->setText(key);
            else
                descriptionsTable->setVerticalHeaderItem(i, new QTableWidgetItem(key));
            if (auto item = descriptionsTable->item(i,0))
                item->setText(val.toString());
            else
                descriptionsTable->setItem(i,0, new QTableWidgetItem(val.toString()));
            i++;
        }
    }
    else {
        current = -1;
        for (auto l: channelPropertiesLabels.labels) l->clear();
        for (auto l: dataPropertiesLabels.labels) l->clear();

        descriptionsTable->clearContents();
    }
}
