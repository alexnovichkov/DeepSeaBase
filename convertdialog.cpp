#include "convertdialog.h"

#include <QtWidgets>
#include <QThread>

#include "converters.h"
#include "dfdfiledescriptor.h"
#include "methods/spectremethod.h"
#include "methods/timemethod.h"
#include "methods/xresponch1.h"
#include "methods/octavemethod.h"
#include "logging.h"

#include "converter.h"

CalculateSpectreDialog::CalculateSpectreDialog(QList<FileDescriptor *> *dataBase, QWidget *parent) :
    QDialog(parent)
{DD;
    converter = 0;
    thread = 0;

    foreach (FileDescriptor *d, *dataBase) {
        DfdFileDescriptor *dd = static_cast<DfdFileDescriptor *>(d);
        if (dd)
            this->dataBase << dd;
    }

    methodCombo = new QComboBox(this);
    methodCombo->setEditable(false);
    for (int i=0; i<26; ++i) {
        methodCombo->addItem(QString(methods[i].methodDescription));
    }
    connect(methodCombo,SIGNAL(currentIndexChanged(int)), SLOT(methodChanged(int)));

    static double bands[12] = {12800.0, 6400.0, 3200.0, 1600.0, 800.0, 400.0,
                                200.0,   100.0,  50.0,   25.0,   12.5,  6.25};

    activeStripCombo = new QComboBox(this);
    activeStripCombo->setEditable(false);

    //infoLabel = new QLabel(this);
    //infoLabel->setWordWrap(true);
    infoLabel = new QPlainTextEdit(this);

    useDeepsea = new QCheckBox("Использовать DeepSea для расчета спектров", this);
    connect(useDeepsea, &QCheckBox::clicked, [=](){
        if (useDeepsea->isChecked()) {
            QString s = QStandardPaths::findExecutable("DeepSea");
            if (s.isEmpty()) infoLabel->appendPlainText("Не могу найти DeepSea.exe в стандартных путях.\n"
                                                "Добавьте путь к DeepSea.exe в переменную PATH");
        }
        else infoLabel->clear();
    });

    progress = new QProgressBar(this);
    int progressMax = 0;
    foreach(DfdFileDescriptor *dfd, this->dataBase) {
        progressMax += dfd->channelsCount();
    }

    progress->setRange(0, progressMax);
    progress->hide();

    RawChannel *raw = dynamic_cast<RawChannel *>(dataBase->first()->channel(0));
    if (raw)
        bandWidth = raw->BandWidth;
    else
        bandWidth = qRound(1.0 / dataBase->first()->channel(0)->xStep() / 2.56);

    for (int i=0; i<12; ++i) {
        if (bands[i]-bandWidth <= 0.01) {
            bandWidth = bands[i];
            break;
        }
    }

    double bw = bandWidth;
    for (int i=0; i<12; ++i) {
        activeStripCombo->addItem(QString::number(bw));
        bw /= 2.0;
    }
    activeStripCombo->setCurrentIndex(2);

    activeChannelSpin = new QSpinBox(this);
    activeChannelSpin->setRange(1, 256);
    activeChannelSpin->setValue(1);
    baseChannelSpin = new QSpinBox(this);
    baseChannelSpin->setRange(1, 256);
    baseChannelSpin->setValue(2);
    overlap = new QSpinBox(this);
    overlap->setRange(0,75);
    overlap->setValue(0);
    overlap->setSingleStep(5);

    methodsStack = new QStackedWidget(this);
    for (int i=0; i<26; ++i) {
        switch (i) {
            case 0: methodsStack->addWidget(new TimeMethod(this)); break;
            case 1: methodsStack->addWidget(new SpectreMethod(this)); break;
            case 9: methodsStack->addWidget(new XresponcH1Method(this)); break;
            case 18: methodsStack->addWidget(new OctaveMethod(this)); break;

            default: methodsStack->addWidget(new SpectreMethod(this));
        }
    }

    shutdown = new QCheckBox("Выключить компьютер после завершения обработки", this);

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                                       QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(start()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(stop()));

    //connect(this,SLOT(reject()), this, SLOT(stop()));

    QGridLayout *l = new QGridLayout;
    l->addWidget(new QLabel("Метод обработки", this), 0,0);
    l->addWidget(methodCombo,0,1);
    l->addWidget(new QLabel("Активный канал", this), 1,0);
    l->addWidget(activeChannelSpin, 1,1);
    l->addWidget(new QLabel("Опорный канал", this), 2,0);
    l->addWidget(baseChannelSpin, 2,1);
    l->addWidget(new QLabel("Частотный диапазон", this), 3,0);
    l->addWidget(activeStripCombo, 3,1);
    l->addWidget(new QLabel("Перекрытие, %", this), 4,0);
    l->addWidget(overlap, 4,1);
    l->addWidget(infoLabel, 5,0,1,2);
    l->addWidget(progress,6,0,1,2);

    l->addWidget(methodsStack,0,2,7,1);
    l->addWidget(useDeepsea,7,0,1,3);
    l->addWidget(shutdown,8,0,1,3);
    l->addWidget(buttonBox, 9,0,1,3);


    setLayout(l);
    methodCombo->setCurrentIndex(1);

}

CalculateSpectreDialog::~CalculateSpectreDialog()
{DD;
    if (converter) {
        converter->deleteLater();
    }
    if (thread) {
        thread->quit();
        thread->wait();
        thread->deleteLater();
    }
}

void CalculateSpectreDialog::methodChanged(int method)
{DD;
    methodsStack->setCurrentIndex(method);
    currentMethod = dynamic_cast<AbstractMethod *>(methodsStack->currentWidget());
}

void CalculateSpectreDialog::start()
{DD;
    newFiles.clear();

    buttonBox->buttons().first()->setDisabled(true);

    Parameters p = currentMethod->parameters();
    p.method = currentMethod;
    p.useDeepSea = useDeepsea->isChecked();

   // p.activeChannel = -1;//activeChannelSpin->value();
   // if (currentMethod->id() == 9 /* Добавлять сюда методы, которым требуется активный канал*/) {
        p.activeChannel = activeChannelSpin->value();
  //  }

   // p.baseChannel = -1;
  //  if (currentMethod->id() == 9 /* Добавлять сюда кросс методы, которым требуется опорный канал*/) {
        p.baseChannel = baseChannelSpin->value();
  //  }

    p.overlap = 1.0 * overlap->value() / 100;

    p.bandWidth = bandWidth;
    p.initialBandStrip = activeStripCombo->currentIndex();

    if (!thread) thread = new QThread;
    converter = new Converter(dataBase, p);
    converter->moveToThread(thread);



    connect(thread, SIGNAL(started()), converter, SLOT(start()));
    connect(converter, SIGNAL(finished()), thread, SLOT(quit()));
    connect(converter, SIGNAL(finished()), this, SLOT(accept()));
    connect(converter, SIGNAL(tick()), SLOT(updateProgressIndicator()));
    connect(converter, SIGNAL(tick(QString)), SLOT(updateProgressIndicator(QString)));
    connect(converter, SIGNAL(message(QString)), infoLabel, SLOT(appendPlainText(QString)));

    progress->show();
    progress->setValue(0);

    thread->start();
}

void CalculateSpectreDialog::stop()
{DD;
    if (thread)
    thread->requestInterruption();
    QDialog::accept();
}

void CalculateSpectreDialog::accept()
{DD;
    newFiles = converter->getNewFiles();

    if (shutdown->isChecked()) {
        QProcess::execute("shutdown",QStringList()<<"/s"<<"/f"<<"/t"<<"1");
    }

    else QDialog::accept();
}

void CalculateSpectreDialog::reject()
{DD;
    stop();
    QDialog::reject();
}

void CalculateSpectreDialog::updateProgressIndicator(const QString &path)
{DD;
    progress->setValue(QDir(path).count()-2);
}

void CalculateSpectreDialog::updateProgressIndicator()
{
    progress->setValue(progress->value()+1);
}

