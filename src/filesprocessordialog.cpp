#include "filesprocessordialog.h"
#include "fileformats/filedescriptor.h"
#include "logging.h"
#include "taskbarprogress.h"
#include <QtTreePropertyBrowser>
#include <QJsonDocument>
#include <QtVariantPropertyManager>

//#include "methods/abstractfunction.h"

#include "methods/spectrealgorithm.h"
#include "methods/resamplingalgorithm.h"
#include "methods/windowingalgorithm.h"
#include "methods/frfalgorithm.h"
#include "methods/psalgorithm.h"
#include "methods/psdalgorithm.h"
#include "methods/filteringalgorithm.h"
#include "methods/octavealgorithm.h"

FilesProcessorDialog::FilesProcessorDialog(QList<FileDescriptor *> &dataBase, QWidget *parent)
    : QDialog(parent), dataBase(dataBase), win(parent), currentAlgorithm(0)
{DD;
    thread_ = 0;
    taskBarProgress = 0;


    algorithms << new ResamplingAlgorithm(dataBase);
    algorithms << new WindowingAlgorithm(dataBase);
    algorithms << new FilteringAlgorithm(dataBase);
    algorithms << new SpectreAlgorithm(dataBase);
    algorithms << new PsAlgorithm(dataBase);
    algorithms << new PsdAlgorithm(dataBase);
    algorithms << new FRFAlgorithm(dataBase);
    algorithms << new OctaveAlgorithm(dataBase);

    filesTree = new QTreeWidget(this);
    filesTree->setColumnCount(4);
    filesTree->setRootIsDecorated(false);
    filesTree->setAlternatingRowColors(true);
    filesTree->setHeaderLabels(QStringList()<<"№"<<"Файл"<<"Тип"<<"Каналы");
    int i=1;
    for (FileDescriptor *f: qAsConst(dataBase))
        filesTree->addTopLevelItem(new QTreeWidgetItem({QString::number(i++),
                                                        f->fileName(),
                                                        f->typeDisplay(),
                                                        QString::number(f->channelsCount())}));

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
    functionsList->setHeaderLabels({"Функция","Описание"});
    functionsList->setSizePolicy(functionsList->sizePolicy().horizontalPolicy(),
                                 QSizePolicy::Expanding);

    for (AbstractAlgorithm *f: qAsConst(algorithms)) {
        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setText(0, f->displayName());
        item->setText(1, f->description());
        item->setToolTip(0, f->description());
        functionsList->addTopLevelItem(item);
    }

    functionsList->resizeColumnToContents(0);

    connect(functionsList,SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), SLOT(methodChanged(QTreeWidgetItem*)));

    m_manager = new QtVariantPropertyManager(this);
    m_factory = new QtVariantEditorFactory(this);

    propertyTree = new QtTreePropertyBrowser(this);
    propertyTree->setAlternatingRowColors(true);
    propertyTree->setHeaderVisible(true);
    propertyTree->setSizePolicy(propertyTree->sizePolicy().horizontalPolicy(),
                                 QSizePolicy::Expanding);
    propertyTree->setFactoryForManager(m_manager, m_factory);
    connect(m_manager, SIGNAL(valueChanged(QtProperty*, const QVariant&)),
                this, SLOT(onValueChanged(QtProperty*, const QVariant&)));
    propertyTree->setPropertiesWithoutValueMarked(true);
    propertyTree->setResizeMode(QtTreePropertyBrowser::ResizeToContents);

//    QToolBar *toolBar = new QToolBar(this);
//    toolBar->addAction(qApp->style()->standardIcon(QStyle::SP_LineEditClearButton), "");

    infoLabel = new QPlainTextEdit(this);
    infoLabel->setReadOnly(true);

    progress = new QProgressBar(this);
    int progressMax = 0;
    for (FileDescriptor *dfd: qAsConst(dataBase)) {
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

    resize(qApp->primaryScreen()->availableSize().width()/2,
           qApp->primaryScreen()->availableSize().height()/2);
}

FilesProcessorDialog::~FilesProcessorDialog()
{DD;
    for (AbstractAlgorithm *f: qAsConst(algorithms)) {
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
    propertyTree->clear();
    m_manager->clear();
    map.clear();
    if (currentAlgorithm) {
        for (AbstractFunction *f: currentAlgorithm->functions())
            disconnect(f,0,0,0);
    }

    currentAlgorithm = nullptr;

    if (!item) return;

    int index = functionsList->indexOfTopLevelItem(item);
    if (index < 0 || index >= algorithms.size()) return;
    currentAlgorithm = algorithms[index];

    // Parsing properties descriptions
    for (AbstractFunction *f: currentAlgorithm->functions()) {
        if (!f->paired()) addProperties(f);
        connect(f, &AbstractFunction::attributeChanged, this, &FilesProcessorDialog::updateProperty);
    }

    // блокируем сигналы, чтобы при каждом изменении свойства не обновлять все отображения свойств
    // мы это сделаем в самом конце функцией updateVisibleProperties();
//    m_manager->blockSignals(true);
    for (auto [key, value]: asKeyValueRange(map)) {
        key->setValue(currentAlgorithm->getParameter(value.f, value.name));
    }
//    m_manager->blockSignals(false);
    updateVisibleProperties();
}

void FilesProcessorDialog::updateVisibleProperties()
{DD;
    if (!currentAlgorithm) return;

    for (QtVariantProperty *property: map.keys()) {
        foreach (auto item, propertyTree->items(property)) {
            propertyTree->setItemVisible(item, currentAlgorithm->propertyShowsFor(map.value(property).f, map.value(property).name));
        }
    }
}

void FilesProcessorDialog::onValueChanged(QtProperty *property, const QVariant &val)
{DD;
    if (!currentAlgorithm) return;

    Property p = map.value(dynamic_cast<QtVariantProperty*>(property));
    if (p.name.isEmpty()) return;

    currentAlgorithm->setParameter(p.f, p.name, val);

    updateVisibleProperties();
}

void FilesProcessorDialog::updateProperty(AbstractFunction *f, const QString &property, const QVariant &val, const QString &attribute)
{DD;
    for (auto [key,v] : asKeyValueRange(map)) {
        if (v.f == f && v.name == property) {
            if (val.isValid() && !attribute.isEmpty())
                key->setAttribute(attribute, val);

            key->setValue(currentAlgorithm->getParameter(f, property));
            updateVisibleProperties();
        }
    }
}

void FilesProcessorDialog::addProperties(AbstractFunction *f)
{DD;
    if (!f) return;

    if (f->properties().isEmpty()) return;

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(f->propertiesDescription().toUtf8(), &error);
    if (error.error != QJsonParseError::NoError) {
        qDebug()<<error.errorString() << error.offset;
        qDebug()<<f->propertiesDescription().toUtf8();
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


        QtVariantProperty *p = m_manager->addProperty(typeId, displayName);
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

        map.insert(p, {f, f->name()+"/"+name});
    }


}

void FilesProcessorDialog::accept()
{DD;
    newFiles = currentAlgorithm->getNewFiles();
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
{DD;
    progress->setValue(progress->value()+1);
    taskBarProgress->setValue(progress->value());
}

void FilesProcessorDialog::start()
{DD;
    if (!currentAlgorithm) return;

    newFiles.clear();
    buttonBox->buttons().constFirst()->setDisabled(true);

    if (!thread_) thread_ = new QThread;
    currentAlgorithm->moveToThread(thread_);

    taskBarProgress = new TaskBarProgress(win, this);
    taskBarProgress->setRange(progress->minimum(), progress->maximum());

    connect(thread_, SIGNAL(started()), currentAlgorithm, SLOT(start()));
    connect(currentAlgorithm, SIGNAL(finished()), thread_, SLOT(quit()));
    connect(currentAlgorithm, SIGNAL(finished()), this, SLOT(accept()));
    connect(currentAlgorithm, SIGNAL(finished()), taskBarProgress, SLOT(finalize()));

    connect(currentAlgorithm, SIGNAL(tick()), SLOT(updateProgressIndicator()));
    connect(currentAlgorithm, SIGNAL(tick(QString)), SLOT(updateProgressIndicator(QString)));
    connect(currentAlgorithm, SIGNAL(message(QString)), infoLabel, SLOT(appendPlainText(QString)));

    progress->show();
    progress->setValue(0);

    thread_->start();
}

void FilesProcessorDialog::stop()
{DD;
    if (thread_)
    thread_->requestInterruption();
    QDialog::accept();
}


