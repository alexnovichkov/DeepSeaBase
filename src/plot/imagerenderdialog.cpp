#include "imagerenderdialog.h"
#include "settings.h"

#include <QtWidgets>
#include "fancylineedit.h"
#include "logging.h"

ImageRenderDialog::ImageRenderDialog(bool askForPath, bool askForGraphOnly, QWidget *parent) : QDialog(parent),
    askForPath(askForPath)
{DD;
    setWindowTitle("Установка параметров рисунка");

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));


    _path = Settings::getSetting("lastPicture", "plot.bmp").toString();
    _width = Settings::getSetting("lastPictureWidth", defaultSize().width()).toInt();
    _height = Settings::getSetting("lastPictureHeight", defaultSize().height()).toInt();
    _resolution = Settings::getSetting("lastPictureResolution", defaultResolution()).toInt();

    if (askForPath) {
        pathEdit = new FancyLineEdit(this);
        pathEdit->setText(_path);

        QPixmap pixmap(16, 16);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.drawText(0,14,"...");

        pathEdit->setButtonPixmap(FancyLineEdit::Right, pixmap);
        pathEdit->setButtonVisible(FancyLineEdit::Right, true);
        pathEdit->setButtonToolTip(FancyLineEdit::Right, tr("Выбрать файл"));
        pathEdit->setAutoHideButton(FancyLineEdit::Right, true);
        connect(pathEdit, &FancyLineEdit::rightButtonClicked, [=](){
            QString last = QFileDialog::getSaveFileName(this, QString("Сохранение графика"), _path,
                                                        "Растровые изображения (*.bmp);;Файлы JPEG (*.jpg);;Файлы pdf (*.pdf);;Файлы svg (*.svg)");
            if (!last.isEmpty()) {
                pathEdit->setText(last);
                _path = last;
            }
        });
        connect(pathEdit, &QLineEdit::textChanged, [=](const QString &text){
            _path = text;
        });
    }

    if (askForGraphOnly) {
        graphOnlyCheckBox = new QCheckBox("Убрать вспомогательные графики", this);
        graphOnlyCheckBox->setChecked(Settings::getSetting("lastPictureGraphOnly", false).toBool());
    }

    widthEdit = new QLineEdit(QString::number(_width), this);
    connect(widthEdit, &QLineEdit::textChanged, [=](const QString &text){
        bool ok = true;
        int val = text.toInt(&ok);
        if (ok) _width = val;
    });

    heightEdit = new QLineEdit(QString::number(_height), this);
    connect(heightEdit, &QLineEdit::textChanged, [=](const QString &text){
        bool ok = true;
        int val = text.toInt(&ok);
        if (ok) _height = val;
    });

    resolutionCombo = new QComboBox(this);
    resolutionCombo->addItems(QStringList()<<"экранное"<<"96 dpi"<<"150 dpi"<<"300 dpi");
    resolutionCombo->setCurrentIndex(Settings::getSetting("imageResolution", 2).toInt());

    auto *mainLayout = new QFormLayout;
    if (askForPath)
        mainLayout->addRow(new QLabel("Куда сохраняем", this), pathEdit);
    if (askForGraphOnly)
        mainLayout->addRow(graphOnlyCheckBox);
    mainLayout->addRow(new QLabel("Ширина рисунка, мм", this), widthEdit);
    mainLayout->addRow(new QLabel("Высота рисунка, мм", this), heightEdit);
    mainLayout->addRow(new QLabel("Разрешение", this), resolutionCombo);
    mainLayout->addRow(buttonBox);

    setLayout(mainLayout);
    resize(400,180);
}

ImageRenderDialog::~ImageRenderDialog()
{
    Settings::setSetting("imageResolution", resolutionCombo->currentIndex());
    Settings::setSetting("lastPicture", _path);
    Settings::setSetting("lastPictureWidth", _width);
    Settings::setSetting("lastPictureHeight", _height);
    Settings::setSetting("lastPictureResolution", _resolution);
    if (graphOnlyCheckBox) Settings::setSetting("lastPictureGraphOnly", graphOnlyCheckBox->isChecked());
}

int ImageRenderDialog::getResolution() const
{DD;
    return getResolution(resolutionCombo->currentIndex());
}

bool ImageRenderDialog::graphOnly() const
{
    return graphOnlyCheckBox && graphOnlyCheckBox->isChecked();
}

int ImageRenderDialog::getResolution(int index) const
{
    switch (index) {
        case 0: return qApp->desktop()->logicalDpiX(); break;
        case 1: return 96; break;
        case 2: return 150; break;
        case 3: return 300; break;
        default: break;
    }
    return defaultResolution();
}
