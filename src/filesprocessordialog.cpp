#include "filesprocessordialog.h"
#include "filedescriptor.h"
#include "logging.h"
#include "taskbarprogress.h"
#include <QtTreePropertyBrowser>
#include <QJsonDocument>
#include <QtVariantPropertyManager>

#include "methods/abstractfunction.h"
#include "methods/channelfunction.h"
#include "methods/framecutterfunction.h"
#include "methods/resamplingfunction.h"
#include "methods/filteringfunction.h"
#include "methods/averagingfunction.h"
#include "methods/windowingfunction.h"
#include "methods/spectrefunction.h"

FilesProcessorDialog::FilesProcessorDialog(QList<FileDescriptor *> &dataBase, QWidget *parent)
    : QDialog(parent), dataBase(dataBase), win(parent), function(0)
{DD;
//    converter = 0;
    thread_ = 0;
    taskBarProgress = 0;

//    functions << new ChannelFunction(dataBase, 0);
//    functions << new ResamplingFunction(dataBase, 0);
//    functions << new FilteringFunction(dataBase, 0);
//    functions << new SamplingFunction(dataBase, 0);
//    functions << new AveragingFunction(dataBase, 0);
//    functions << new WindowingFunction(dataBase, 0);
    functions << new SpectreFunction(dataBase, 0);

    foreach (AbstractFunction *f, functions) {
        connect(f, SIGNAL(updateProperty(QString,QVariant,QString)),SLOT(updateProperty(QString,QVariant,QString)));
    }

    filesTree = new QTreeWidget(this);
    filesTree->setColumnCount(4);
    filesTree->setRootIsDecorated(false);
    filesTree->setAlternatingRowColors(true);
    filesTree->setHeaderLabels(QStringList()<<"№"<<"Файл"<<"Тип"<<"Каналы");
    int i=1;
    foreach (FileDescriptor *f, dataBase) {
        QStringList list;
        list << QString::number(i++) << f->fileName() << f->typeDisplay()<<QString::number(f->channelsCount());
        QTreeWidgetItem *item = new QTreeWidgetItem(list);
        filesTree->addTopLevelItem(item);
    }
    filesTree->resizeColumnToContents(0);
    filesTree->resizeColumnToContents(1);
    filesTree->resizeColumnToContents(2);
    filesTree->resizeColumnToContents(3);
    filesTree->setSizePolicy(filesTree->sizePolicy().horizontalPolicy(),
                             QSizePolicy::Expanding);

    functionsList = new QTreeWidget(this);
    functionsList->setAlternatingRowColors(true);
    functionsList->setColumnCount(2);
    functionsList->setRootIsDecorated(false);
    functionsList->setHeaderLabels(QStringList()<<"Функция"<<"Описание");
    functionsList->setSizePolicy(functionsList->sizePolicy().horizontalPolicy(),
                                 QSizePolicy::Expanding);

    foreach (AbstractFunction *f, functions) {
        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setText(0, f->displayName());
        item->setText(1, f->description());
        item->setToolTip(0, f->description());
        functionsList->addTopLevelItem(item);
    }

    functionsList->resizeColumnToContents(0);

    connect(functionsList,SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), SLOT(methodChanged(QTreeWidgetItem*)));

    propertyTree = new QtTreePropertyBrowser(this);
    propertyTree->setAlternatingRowColors(true);
    propertyTree->setHeaderVisible(true);
    propertyTree->setSizePolicy(propertyTree->sizePolicy().horizontalPolicy(),
                                 QSizePolicy::Expanding);
    propertyTree->setFactoryForManager(&m_manager, &m_factory);
    connect(&m_manager, SIGNAL(valueChanged(QtProperty*, const QVariant&)),
                this, SLOT(onValueChanged(QtProperty*, const QVariant&)));
    propertyTree->setPropertiesWithoutValueMarked(true);

//    QToolBar *toolBar = new QToolBar(this);
//    toolBar->addAction(qApp->style()->standardIcon(QStyle::SP_LineEditClearButton), "");

    infoLabel = new QPlainTextEdit(this);
    infoLabel->setReadOnly(true);

    progress = new QProgressBar(this);
    int progressMax = 0;
    foreach(FileDescriptor *dfd, dataBase) {
        progressMax += dfd->channelsCount();
    }
    progress->setRange(0, progressMax);


    shutdown = new QCheckBox("Выключить компьютер после завершения обработки", this);

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(start()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(stop()));

    QSplitter *splitter2 = new QSplitter(Qt::Horizontal, this);
    splitter2->setContentsMargins(0,0,0,0);
    splitter2->addWidget(functionsList);
    splitter2->addWidget(propertyTree);
    splitter2->addWidget(infoLabel);
    splitter2->setCollapsible(0, false);
    splitter2->setCollapsible(1, false);
    splitter2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QSplitter *splitter1 = new QSplitter(Qt::Vertical, this);
    splitter1->setContentsMargins(0,0,0,0);
    splitter1->addWidget(filesTree);
    splitter1->addWidget(splitter2);
    splitter1->setCollapsible(0, false);
    splitter1->setCollapsible(1, false);
    splitter1->setStretchFactor(0,1);
    splitter1->setStretchFactor(1,2);

    QGridLayout *l = new QGridLayout;
    l->addWidget(splitter1, 0,0,1,2);
    l->addWidget(progress,1,0,1,2);
    l->addWidget(shutdown, 2,0);
    l->addWidget(buttonBox, 2,1,1,1,Qt::AlignRight);

    setLayout(l);

    resize(1000, 600);
}

FilesProcessorDialog::~FilesProcessorDialog()
{DD;
    foreach (AbstractFunction *f, functions) {
        if (f) f->deleteLater();
    }
    if (thread_) {
        thread_->quit();
        thread_->wait();
        thread_->deleteLater();
    }

}

void FilesProcessorDialog::methodChanged(QTreeWidgetItem *item)
{DD;
    //propertyTree->setUpdatesEnabled(false);
    propertyTree->clear();

    //m_manager.blockSignals(true);
    m_manager.clear();
    map.clear();

    if (!item) return;

    int index = functionsList->indexOfTopLevelItem(item);
    if (index < 0 || index >= functions.size()) return;
    function = functions[index];

    // Parsing properties descriptions
    foreach (AbstractFunction *f, function->functions()) {
        addProperties(f);
    }
    addProperties(function);

    updateVisibleProperties();
}

void FilesProcessorDialog::updateVisibleProperties()
{
    if (!function) return;

    foreach (QtProperty *property, map.keys()) {
        foreach (auto item, propertyTree->items(property)) {
            propertyTree->setItemVisible(item, function->propertyShowsFor(map.value(dynamic_cast<QtVariantProperty*>(property))));
        }
    }
}

void FilesProcessorDialog::onValueChanged(QtProperty *property, const QVariant &val)
{
//    qDebug()<<property->propertyName()<<val;

    if (function) function->setProperty(map.value(dynamic_cast<QtVariantProperty*>(property)), val);

    updateVisibleProperties();
}

void FilesProcessorDialog::updateProperty(const QString &property, const QVariant &val, const QString &attribute)
{
    if (QtVariantProperty *p = map.key(property)) {
        p->setAttribute(attribute, val);

        p->setValue(function->getProperty(property));
//        updateVisibleProperties();
    }
}

void FilesProcessorDialog::addProperties(AbstractFunction *f)
{
    if (f->properties().size()<1) return;
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(f->propertiesDescription().toUtf8(), &error);
    if (error.error != QJsonParseError::NoError) {
        qDebug()<<error.errorString() << error.offset;
        qDebug()<<f->propertiesDescription();
        return;
    }

    QtVariantProperty *group = 0;

    QJsonArray propertiesArray = doc.array();
    for (int i=0; i< propertiesArray.size(); ++i) {
        QJsonObject val = propertiesArray.at(i).toObject();
        QString type = val.value("type").toString();
        QString name = val.value("name").toString();
        QString displayName = val.value("displayName").toString();
        if (i==0) //first property makes the header
            displayName = f->displayName();
        QString tooltip = val.value("toolTip").toString();

        int typeId = QVariant::String;
        if (type=="string") typeId = QVariant::String;
        else if (type=="int") typeId = QVariant::Int;
        else if (type=="double") typeId = QVariant::Double;
        else if (type=="bool") typeId = QVariant::Bool;
        else if (type=="enum") typeId = QtVariantPropertyManager::enumTypeId();


        QtVariantProperty *p = m_manager.addProperty(typeId, displayName);
        if (i==0) {
            group = p;
            auto item = propertyTree->addProperty(group);
            propertyTree->setExpanded(item, true);
        }
        else group->addSubProperty(p);
        p->setToolTip(tooltip);

        if (typeId == QVariant::Int || typeId == QVariant::Double) {
            if (val.contains("minimum")) p->setAttribute("minimum", val.value("minimum"));
            if (val.contains("maximum")) p->setAttribute("maximum", val.value("maximum"));
        }
        else if (typeId == QVariant::Bool) {
            p->setAttribute("textVisible", true);
        }
        else if (typeId == QtVariantPropertyManager::enumTypeId()) {
            QStringList names;
            QJsonArray namesArray = val.value("values").toArray();
            for (int name = 0; name < namesArray.size(); ++name)
                names << namesArray.at(name).toString();
            p->setAttribute("enumNames", names);
        }

        map.insert(p, f->name()+"/"+name);
    }

    foreach (const QString &property, map.values()) {
        map.key(property)->setValue(f->getProperty(property));
    }
}

void FilesProcessorDialog::accept()
{DD;
    newFiles = function->getNewFiles();
    if (taskBarProgress) taskBarProgress->finalize();

    /*if (shutdown->isChecked()) {
        QProcess::execute("shutdown",QStringList()<<"/s"<<"/f"<<"/t"<<"1");
    }

    else */QDialog::accept();
}

void FilesProcessorDialog::reject()
{DD;
    stop();
    QDialog::reject();
    if (taskBarProgress) taskBarProgress->finalize();
}

void FilesProcessorDialog::updateProgressIndicator(const QString &path)
{DD;
    progress->setValue(QDir(path).count()-2);
}

void FilesProcessorDialog::updateProgressIndicator()
{
    progress->setValue(progress->value()+1);
    taskBarProgress->setValue(progress->value());
}

void FilesProcessorDialog::start()
{
    if (!function) return;

    newFiles.clear();
    buttonBox->buttons().first()->setDisabled(true);

//    Parameters p = currentMethod->parameters();
//    p.method = currentMethod;
//    p.useDeepSea = useDeepsea->isChecked();
//    p.channelFilter = channelsFilter->text();

    if (!thread_) thread_ = new QThread;
    function->moveToThread(thread_);
//    foreach (AbstractFunction *f, function->functions()) {
//        qDebug()<<f->name()<<"is currently in"<<f->thread();
//    }

    taskBarProgress = new TaskBarProgress(win, this);
    taskBarProgress->setRange(progress->minimum(), progress->maximum());

    connect(thread_, SIGNAL(started()), function, SLOT(start()));
    connect(function, SIGNAL(finished()), thread_, SLOT(quit()));
    connect(function, SIGNAL(finished()), this, SLOT(accept()));
    connect(function, SIGNAL(finished()), taskBarProgress, SLOT(finalize()));

    connect(function, SIGNAL(tick()), SLOT(updateProgressIndicator()));
    connect(function, SIGNAL(tick(QString)), SLOT(updateProgressIndicator(QString)));
    connect(function, SIGNAL(message(QString)), infoLabel, SLOT(appendPlainText(QString)));

    progress->show();
    progress->setValue(0);

    thread_->start();
}

void FilesProcessorDialog::stop()
{
    if (thread_)
    thread_->requestInterruption();
    QDialog::accept();
}


