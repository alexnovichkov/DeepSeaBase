#include "calculatespectredialog.h"

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

    methodsStack = new QStackedWidget(this);
    for (int i=0; i<26; ++i) {
        switch (i) {
            case 0:
                methodsStack->addWidget(new TimeMethod(this->dataBase, this)); break;
            case 1: methodsStack->addWidget(new SpectreMethod(this->dataBase, this)); break;
            case 9: methodsStack->addWidget(new FRFMethod(this->dataBase, this)); break;
            case 18: methodsStack->addWidget(new OctaveMethod(this->dataBase, this)); break;

            default: methodsStack->addWidget(new SpectreMethod(this->dataBase, this));
        }
    }

    shutdown = new QCheckBox("Выключить компьютер после завершения обработки", this);

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                                       QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(start()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(stop()));

    //connect(this,SLOT(reject()), this, SLOT(stop()));

    QGridLayout *l = new QGridLayout;
    l->addWidget(infoLabel, 0,0,2,1);
    l->addWidget(new QLabel("Метод обработки", this), 0,1);
    l->addWidget(methodCombo,0,2);
    l->addWidget(methodsStack,1,1,1,2);
    l->addWidget(progress,2,0,1,3);
    l->addWidget(useDeepsea,3,0,1,3);
    l->addWidget(shutdown,4,0,1,3);
    l->addWidget(buttonBox, 5,0,1,3);

//    l->addWidget(new QLabel("Активный канал", this), 1,0);
//    l->addWidget(activeChannelSpin, 1,1);
//    l->addWidget(new QLabel("Опорный канал", this), 2,0);
//    l->addWidget(baseChannelSpin, 2,1);

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

