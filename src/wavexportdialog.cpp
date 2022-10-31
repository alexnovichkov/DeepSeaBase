#include "wavexportdialog.h"

#include <QtWidgets>

#include "fileformats/filedescriptor.h"
#include "wavexporter.h"
#include "taskbarprogress.h"
#include "logging.h"

WavExportDialog::WavExportDialog(FileDescriptor * file, const QVector<int> &indexes, QWidget *parent)
    : QDialog(parent), file{file}, indexes{indexes}
{DDD;
    setWindowTitle("Сохранение в WAV");

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                     QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(start()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(stop()));

    channelsCount = new QSpinBox(this);
    channelsCount->setRange(1, indexes.size());

    formatComboBox = new QComboBox(this);
    formatComboBox->addItems({"PCM, 16 bit", "Extended PCM, 16 bit", "IEEE-float, 32 bit"});

    connect(channelsCount, qOverload<int>(&QSpinBox::valueChanged), [=](int val) {
       QString text =  "<font color=darkblue>Формат имени файлов: %1 - %2.wav</font>";
       if (val==1) text = text.arg(QFileInfo(file->fileName()).fileName()).arg(file->channel(indexes.first())->name());
       else {
           text =  "<font color=darkblue>Формат имени файлов: %1 - %2-%3.wav</font>";
           text = text.arg(QFileInfo(file->fileName()).fileName())
                  .arg(indexes.first())
                  .arg(indexes.at(val-1));
       }
       hintLabel->setText(text);
    });

    bar = new QProgressBar(this);
    bar->setTextVisible(false);
    bar->setFixedHeight(10);
    bar->setRange(0, indexes.size());

    hintLabel = new QLabel(this);
    hintLabel->setIndent(10);


    channelsCount->setValue(indexes.size());
    bar->setValue(0);

    taskBarProgress = new TaskBarProgress(this, this);
    taskBarProgress->setRange(bar->minimum(), bar->maximum());

    QFormLayout *l = new QFormLayout;
    l->addRow("Файл", new QLabel(file->fileName(),this));
    l->addRow("Каналов на файл", channelsCount);
    l->addRow("Формат файла", formatComboBox);
    l->addRow(hintLabel);
    l->addRow(bar);
    l->addRow(buttonBox);
    setLayout(l);
}

WavExportDialog::~WavExportDialog()
{DDD;
    if (exporter) {
        exporter->deleteLater();
    }
    if (thread) {
        thread->quit();
        thread->wait();
        thread->deleteLater();
    }
}

void WavExportDialog::start()
{DDD;
    buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
    if (!thread) thread = new QThread;
    exporter = new WavExporter(file, indexes, channelsCount->value());
    exporter->setFormat(static_cast<WavFormat>(formatComboBox->currentIndex()));
    exporter->moveToThread(thread);

    connect(thread, SIGNAL(started()), exporter, SLOT(start()));
    connect(exporter, SIGNAL(chunksCountChanged(int)), this, SLOT(updateMaxProgress(int)));
    connect(exporter, SIGNAL(finished()), thread, SLOT(quit()));
    connect(exporter, SIGNAL(finished()), this, SLOT(accept()));
    connect(exporter, SIGNAL(finished()), taskBarProgress, SLOT(finalize()));
    connect(exporter, SIGNAL(tick()), this, SLOT(updateProgressIndicator()));

    thread->start();
}

void WavExportDialog::updateProgressIndicator()
{DDD;
    bar->setValue(bar->value()+1);
    taskBarProgress->setValue(bar->value());
}

void WavExportDialog::updateMaxProgress(int val)
{DDD;
    bar->setMaximum(val);
    if (taskBarProgress) taskBarProgress->setRange(bar->minimum(), bar->maximum());
}

void WavExportDialog::stop()
{DDD;
    if (thread)
        thread->requestInterruption();
    QDialog::accept();
}

void WavExportDialog::accept()
{DDD;
    //if (taskBarProgress) taskBarProgress->finalize();

    QDialog::accept();
}

void WavExportDialog::reject()
{DDD;
    stop();
    QDialog::reject();
    if (taskBarProgress) taskBarProgress->finalize();
}
