#include "calculatespectredialog.h"

#include <QtWidgets>
#include <QThread>

#include "fileformats/dfdfiledescriptor.h"
#include "methods/dfdmethods/spectremethod.h"
#include "methods/dfdmethods/timemethod.h"
#include "methods/dfdmethods/xresponch1.h"
#include "methods/dfdmethods/octavemethod.h"
#include "logging.h"

#include "converters/converter.h"
#include "taskbarprogress.h"

CalculateSpectreDialog::CalculateSpectreDialog(QList<FileDescriptor *> &dataBase, QWidget *parent) :
    QDialog(parent), dataBase(dataBase), win(parent)
{DD;
    methodCombo = new QComboBox(this);
    methodCombo->setEditable(false);
    for (const auto &method: methods)
        methodCombo->addItem(QString(method.methodDescription));

    connect(methodCombo,SIGNAL(currentIndexChanged(int)), SLOT(methodChanged(int)));

    infoLabel = new QPlainTextEdit(this);
    channelsFilter = new QLineEdit(this);
    channelsFilter->setPlaceholderText("1, 3-6, 10-");

//    useDeepsea = new QCheckBox("Использовать DeepSea для расчета спектров", this);
//    connect(useDeepsea, &QCheckBox::clicked, [=](){
//        if (useDeepsea->isChecked()) {
//            QString s = QStandardPaths::findExecutable("DeepSea");
//            if (s.isEmpty()) infoLabel->appendPlainText("Не могу найти DeepSea.exe в стандартных путях.\n"
//                                                "Добавьте путь к DeepSea.exe в переменную PATH");
//        }
//        else infoLabel->clear();
//    });

    progress = new QProgressBar(this);
    int progressMax = 0;
    for (FileDescriptor *dfd: dataBase) {
        progressMax += dfd->channelsCount();
    }
    progress->setRange(0, progressMax);

    methodsStack = new QStackedWidget(this);
    for (int i=0; i<26; ++i) {
        switch (i) {
            case 0: methodsStack->addWidget(new TimeMethod(dataBase, this)); break;
            case 1: methodsStack->addWidget(new SpectreMethod(dataBase, this)); break;
            case 9: methodsStack->addWidget(new FRFMethod(dataBase, this)); break;
            case 18: methodsStack->addWidget(new OctaveMethod(dataBase, this)); break;

            default: methodsStack->addWidget(new SpectreMethod(dataBase, this));
        }
    }

    shutdown = new QCheckBox("Выключить компьютер после завершения обработки", this);

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(start()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(stop()));

    QGridLayout *l = new QGridLayout;
    l->addWidget(infoLabel, 0,0,3,1);

    l->addWidget(new QLabel("Каналы:", this), 0,1);
    l->addWidget(channelsFilter, 0,2);

    l->addWidget(new QLabel("Метод обработки", this), 1,1);
    l->addWidget(methodCombo,1,2);


    l->addWidget(methodsStack,2,1,1,2);
    l->addWidget(progress,3,0,1,3);
//    l->addWidget(useDeepsea,4,0,1,3);
    l->addWidget(shutdown,5,0,1,3);
    l->addWidget(buttonBox, 6,0,1,3);

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
//    QString s = QStandardPaths::findExecutable("DeepSea");
//    if (s.isEmpty()) {
//        infoLabel->appendPlainText("Не могу найти DeepSea.exe в стандартных путях.\n"
//                                   "Добавьте путь к DeepSea.exe в переменную PATH");
//        return;
//    }

    newFiles.clear();

    buttonBox->buttons().constFirst()->setDisabled(true);

    Parameters p = currentMethod->parameters();
    p.method = currentMethod;
//    p.useDeepSea = useDeepsea->isChecked();
    p.useDeepSea = true;
    p.channelFilter = channelsFilter->text();

    if (!thread) thread = new QThread;
    converter = new Converter(dataBase, p);
    converter->moveToThread(thread);

    taskBarProgress = new TaskBarProgress(win, this);
    taskBarProgress->setRange(progress->minimum(), progress->maximum());

    connect(thread, SIGNAL(started()), converter, SLOT(start()));
    connect(converter, SIGNAL(finished()), thread, SLOT(quit()));
    connect(converter, SIGNAL(finished()), this, SLOT(accept()));
    connect(converter, SIGNAL(finished()), taskBarProgress, SLOT(finalize()));

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
    if (taskBarProgress) taskBarProgress->finalize();

    if (shutdown->isChecked()) {
        QProcess::execute("shutdown",QStringList()<<"/s"<<"/f"<<"/t"<<"1");
    }

    else QDialog::accept();
}

void CalculateSpectreDialog::reject()
{DD;
    stop();
    QDialog::reject();
    if (taskBarProgress) taskBarProgress->finalize();
}

void CalculateSpectreDialog::updateProgressIndicator(const QString &path)
{DD;
    progress->setValue(int(QDir(path).count()-2));
}

void CalculateSpectreDialog::updateProgressIndicator()
{DD;
    progress->setValue(progress->value()+1);
    taskBarProgress->setValue(progress->value());
}

