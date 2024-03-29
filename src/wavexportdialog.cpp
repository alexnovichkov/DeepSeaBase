#include "wavexportdialog.h"

#include <QtWidgets>

#include "fileformats/filedescriptor.h"
#include "wavexporter.h"
#include "taskbarprogress.h"
#include "logging.h"
#include "settings.h"

WavExportDialog::WavExportDialog(FileDescriptor * file, const QVector<int> &indexes, QWidget *parent)
    : QDialog(parent), file{file}, indexes{indexes}
{DD;
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
                  .arg(indexes.first()+1)
                  .arg(indexes.at(val-1)+1);
       }
       hintLabel->setText(text);
       se->setSetting("wavChannelsCount", val);
    });

    bar = new QProgressBar(this);
    bar->setTextVisible(false);
    bar->setFixedHeight(10);
    bar->setRange(0, indexes.size());

    hintLabel = new QLabel(this);
    hintLabel->setIndent(10);

    auto wavChannelsCount = se->getSetting("wavChannelsCount", indexes.size()).toInt();
    channelsCount->setValue(qMin(wavChannelsCount, indexes.size()));
    bar->setValue(0);

    taskBarProgress = new TaskBarProgress(this, this);
    taskBarProgress->setRange(bar->minimum(), bar->maximum());

    auto l = new QFormLayout;
    l->addRow("Файл", new QLabel(file->fileName(),this));
    l->addRow("Каналов на файл", channelsCount);
    l->addRow("Формат файла", formatComboBox);
    l->addRow(hintLabel);
    l->addRow(bar);
    l->addRow(buttonBox);
    setLayout(l);
}

WavExportDialog::~WavExportDialog()
{DD;
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
{DD;
    buttonBox->button(QDialogButtonBox::Ok)->setDisabled(true);
    if (!thread) thread = new QThread;
    exporter = new WavExporter(file, indexes, channelsCount->value());
    exporter->setFormat(static_cast<WavFormat>(formatComboBox->currentIndex()));
    exporter->moveToThread(thread);

    connect(thread, SIGNAL(started()), exporter, SLOT(start()));
//    connect(exporter, SIGNAL(chunksCountChanged(int)), this, SLOT(updateMaxProgress(int)));
    connect(exporter, SIGNAL(finished()), thread, SLOT(quit()));
    connect(exporter, SIGNAL(finished()), this, SLOT(accept()));
    connect(exporter, SIGNAL(finished()), taskBarProgress, SLOT(finalize()));
    connect(exporter, SIGNAL(tick()), this, SLOT(updateProgressIndicator()));

    thread->start();
}

void WavExportDialog::updateProgressIndicator()
{DD;
    bar->setValue(bar->value()+1);
    taskBarProgress->setValue(bar->value());
}

void WavExportDialog::updateMaxProgress(int val)
{DD;
    bar->setMaximum(val);
    if (taskBarProgress) taskBarProgress->setRange(bar->minimum(), bar->maximum());
}

void WavExportDialog::stop()
{DD;
    if (thread)
        thread->requestInterruption();
    QDialog::accept();
}

void WavExportDialog::accept()
{DD;
    //if (taskBarProgress) taskBarProgress->finalize();

    QDialog::accept();
}

void WavExportDialog::reject()
{DD;
    stop();
    QDialog::reject();
    if (taskBarProgress) taskBarProgress->finalize();
}
