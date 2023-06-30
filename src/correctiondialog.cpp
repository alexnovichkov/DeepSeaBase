#include "correctiondialog.h"

#include <QtWidgets>
#include "settings.h"
#include "headerview.h"
#include "plot/plot.h"
#include "plot/plotmodel.h"
#include "plot/curve.h"
#include "fileformats/dfdfiledescriptor.h"
#include "logging.h"

CorrectionDialog::CorrectionDialog(Plot *plot, QWidget *parent) : QDialog(parent), plot(plot)
{DD;
    setWindowFlags(Qt::Tool /*| Qt::WindowTitleHint*/);
    setWindowTitle("Поправки");

    table = new QTableView(this);
    //tableHeader = new HeaderView(Qt::Horizontal, table);
    //table->setHorizontalHeader(tableHeader);
    table->setModel(plot->model());
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setStretchLastSection(true);

    allFilesCheckBox = new QCheckBox("Применить поправку ко всем выделенным файлам", this);

    edit = new QLineEdit(this);
    edit->setText("0.0");

    correctButton = new QToolButton(this);
    correctButton->setText("Скорректировать");
    connect(correctButton, SIGNAL(clicked(bool)), SLOT(correct()));
    correctButton->setSizePolicy(correctButton->sizePolicy().horizontalPolicy(), QSizePolicy::Expanding);

    correctionType = new QComboBox(this);
    correctionType->addItems(QStringList()<<"Слагаемое"<<"Множитель");
    correctionType->setCurrentIndex(0);


    QDialogButtonBox *buttonBox = new QDialogButtonBox(/*Qt::Vertical, */this);
    auto b = buttonBox->addButton("Закрыть без сохранения", QDialogButtonBox::RejectRole);
    b->setDefault(true);
    b->setAutoDefault(true);
    b = buttonBox->addButton("Закрыть и сохранить поправку в файл данных", QDialogButtonBox::AcceptRole);
    b->setDefault(false);
    b->setAutoDefault(false);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));


    QGridLayout *l = new QGridLayout;
    l->addWidget(new QLabel("Выделите каналы, введите величину поправки и нажмите \"Скорректировать\"\n\n"
                            "Если величина поправки не устроила, введите другую величину поправки.\n"
                            "Поправки не накапливаются.", this),
                 0,0,1,3);
    l->addWidget(table,1,0,1,3);
    l->addWidget(new QLabel("Величина поправки",this),2,0);
    l->addWidget(edit,2,1);

    l->addWidget(new QLabel("Тип поправки",this),3,0);
    l->addWidget(correctionType,3,1);

    l->addWidget(correctButton,2,2,2,1);

    l->addWidget(allFilesCheckBox,4,0,1,3);

    l->addWidget(buttonBox,5,0,1,3);
    l->setRowStretch(5,l->rowStretch(5)+30);
    setLayout(l);

    auto size = Settings::getSetting("correctionDialogSize").toSize();
    if (!size.isEmpty()) resize(size);
    else resize(qApp->primaryScreen()->availableSize().width()/3,
                qApp->primaryScreen()->availableSize().height()/3);
}

CorrectionDialog::~CorrectionDialog()
{DD;
    Settings::setSetting("correctionDialogSize", size());
}

//void CorrectionDialog::closeEvent(QCloseEvent *event)
//{DD;

//    QWidget::closeEvent(event);
//}

void CorrectionDialog::setFiles(const QList<FileDescriptor *> &descriptors)
{DD;
    files = descriptors;
    // удаляем из списка файлов файлы, графики которых построены на экране
    // чтобы эти файлы не мешались
    const auto channels = plot->model()->plottedChannels();
    for (const auto c: channels)
        files.removeAll(c->descriptor());

    allFilesCheckBox->setEnabled(files.size()>1);
}

void CorrectionDialog::correct()
{DD;
    bool ok;
    QString s = edit->text();
    double correctionValue = s.toDouble(&ok);
    if (!ok) {
        s.replace(',','.');
        correctionValue = s.toDouble(&ok);
    }
    if (!ok) {
        QMessageBox::critical(this, "Поправка","Введенное значение поправки не является числом.");
        return;
    }

    auto selected = table->selectionModel()->selectedIndexes().size();

    if (selected==0) {
        QMessageBox::critical(this, "Поправка","Ни одного канала не выделено.\n"
                                               "Куда мне вносить поправку?");
        return;
    }

    if (correctionValue < 0.0 && correctionType->currentIndex()==1) {
        QMessageBox::critical(this, "Поправка","Отрицательный множитель коррекции!\n"
                                               "Лучше так не делать");
        return;
    }

    for (int i=0; i<plot->model()->size(); ++i) {
        if (table->selectionModel()->rowIntersectsSelection(i, QModelIndex())) {
            Channel *ch =  plot->model()->curve(i)->channel;
            int channelNumber = ch->index();
            if (channelNumber == -1) continue;

            plot->model()->setTemporaryCorrection(ch, correctionValue, correctionType->currentIndex());

            if (allFilesCheckBox->isChecked()) {
                foreach (FileDescriptor *file, files) {
                    if (Channel *ch1 = file->channel(channelNumber)) {
                        if (!ch1->populated()) ch1->populate();
                        ch1->data()->setTemporaryCorrection(correctionValue, correctionType->currentIndex());
                    }
                }
            }
        }
    }

    plot->recalculateScale(Enums::AxisType::atLeft);
    plot->recalculateScale(Enums::AxisType::atRight);
    plot->update();
}

void CorrectionDialog::makeCorrectionConstant(Channel *channel)
{DD;
    if (!channel) return;

    channel->data()->makeCorrectionConstant();
    //Обработка предыдущей коррекции.
    QString previousCorrection = channel->correction();

    if (previousCorrection.isEmpty()) {
        // ранее коррекция не проводилась
        channel->setCorrection(channel->data()->correctionString());
    }
    else {
        // ранее коррекция проводилась, мы должны учесть старое и новое значения
        if (previousCorrection.startsWith("[")) previousCorrection.remove(0,1);
        if (previousCorrection.endsWith("]")) previousCorrection.chop(1);
        int previousType = previousCorrection.startsWith("*")?1:0;
        if (previousType == 1) previousCorrection.remove(0,1);
        bool ok;
        double previousCorrectionValue = previousCorrection.toDouble(&ok);
        if (!ok) previousCorrectionValue = previousType == 0 ? 0.0 : 1.0;

        if (previousType == correctionType->currentIndex()) {
            // типы коррекций совпадают, можно работать
            double newValue = previousType == 0?previousCorrectionValue + channel->data()->correction():
                                                previousCorrectionValue * channel->data()->correction();
            channel->setCorrection(DataHolder::correctionString(newValue, previousType));
        }
        else
            channel->setCorrection(channel->data()->correctionString());
    }
    //подчищаем за собой
    channel->data()->removeCorrection();
}

void CorrectionDialog::accept()
{DD;
    QList<FileDescriptor*> list;
    // сперва графики
    for (int i=0; i<plot->curvesCount(); ++i) {
        Channel *ch = plot->model()->curve(i)->channel;
        if (!ch->data()->hasCorrection()) continue;

        makeCorrectionConstant(ch);
        if (!list.contains(ch->descriptor())) list << ch->descriptor();
        ch->setChanged(true);
        ch->setDataChanged(true);

        if (allFilesCheckBox->isChecked()) {
            foreach (FileDescriptor *file, files) {
                const int index = ch->index();
                if (index == -1) continue;
                if (Channel *ch1 = file->channel(index)) {
                    makeCorrectionConstant(ch1);
                    if (!list.contains(ch1->descriptor())) list << ch1->descriptor();
                    ch1->setChanged(true);
                    ch1->setDataChanged(true);
                }
            }
        }
    }
    foreach (FileDescriptor *f, list) {
        f->setChanged(true);
        f->setDataChanged(true);
        f->write();
    }

    plot->recalculateScale(Enums::AxisType::atLeft);
    plot->recalculateScale(Enums::AxisType::atRight);
    plot->update();
    QDialog::accept();
}

void CorrectionDialog::reject()
{DD;
    for (int i=0; i<plot->curvesCount(); ++i) {
        plot->model()->curve(i)->channel->data()->removeCorrection();

        if (allFilesCheckBox->isChecked()) {
            for (FileDescriptor *file: files) {
                const int index = plot->model()->curve(i)->channel->index();
                if (index == -1) continue;
                if (Channel *ch1 = file->channel(index)) {
                    ch1->data()->removeCorrection();
                }
            }
        }
    }

    plot->recalculateScale(Enums::AxisType::atLeft);
    plot->recalculateScale(Enums::AxisType::atRight);
    plot->update();
    QDialog::reject();
}
