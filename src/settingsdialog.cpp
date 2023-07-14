#include "settingsdialog.h"

#include <QtWidgets>
#include "app.h"
#include "enums.h"
#include "logging.h"
#include "settings.h"
#include "methods/octavefilterbank.h"

#include <QtTreePropertyBrowser>
#include <QtVariantPropertyManager>

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent)
{DD;
    setWindowTitle(tr("Настройки"));

    m_manager = new QtVariantPropertyManager(this);
    m_factory = new QtVariantEditorFactory(this);

    propertyTree = new QtTreePropertyBrowser(this);
    propertyTree->setAlternatingRowColors(true);
    propertyTree->setHeaderVisible(true);
    propertyTree->setSizePolicy(propertyTree->sizePolicy().horizontalPolicy(),
                                 QSizePolicy::Expanding);
    propertyTree->setFactoryForManager(m_manager, m_factory);
    propertyTree->setPropertiesWithoutValueMarked(true);
    propertyTree->setResizeMode(QtTreePropertyBrowser::ResizeToContents);

    addSettings();


    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    auto *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(propertyTree);
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);

    resize({qApp->primaryScreen()->availableSize().width()/3,
            qApp->primaryScreen()->availableSize().height()/3});
}

void SettingsDialog::accept()
{DD;

    QDialog::accept();

}

void SettingsDialog::propertyChanged(QtProperty *p, const QVariant &v)
{
    if (auto key = m_displayNames.value(p->propertyName()); se->hasSetting(key)) {
        se->setSetting(key, v);

        if (key == "font") QMessageBox::information(this, "DeepSea Database",
                                                    "Изменение шрифта вступит в силу\nпосле перезапуска программы");
    }
}

void SettingsDialog::addSettings()
{
    /*   Общие   */
    auto global = m_manager->addProperty(QVariant::String, "Общие");
    global->setValue(QVariant());
    auto item = propertyTree->addProperty(global);

    auto _font = m_manager->addProperty(QVariant::Font, "Шрифт приложения");
    m_displayNames.insert("Шрифт приложения", "font");
    _font->setValue(se->getSetting("font", font()));
    global->addSubProperty(_font);
    propertyTree->setExpanded(item, false);

    /*   Графики    */
    auto graphs = m_manager->addProperty(QVariant::String, "Графики");
    graphs->setValue(QVariant());
    item = propertyTree->addProperty(graphs);
    //propertyTree->setExpanded(item, true);

    auto legendMiddleButton = m_manager->addProperty(QtVariantPropertyManager::enumTypeId(),
                                                     "Средняя кнопка мыши по легенде");
    legendMiddleButton->setAttribute("enumNames", QStringList({"Удаляет кривую",
                                                               "Перемещает кривую на левую/правую ось"}));
    m_displayNames.insert("Средняя кнопка мыши по легенде", "legendMiddleButton");
    legendMiddleButton->setValue(se->getSetting("legendMiddleButton", 0).toInt());
    graphs->addSubProperty(legendMiddleButton);

    auto cursorDialogFont = m_manager->addProperty(QVariant::Font, "Шрифт окна курсоров");
    m_displayNames.insert("Шрифт окна курсоров", "cursorDialogFont");
    auto f = font();
    f.setPointSize(f.pointSize()-1);
    cursorDialogFont->setValue(se->getSetting("cursorDialogFont", f));
    graphs->addSubProperty(cursorDialogFont);

    auto plotOctaveAsHistogram = m_manager->addProperty(QtVariantPropertyManager::enumTypeId(), "График 1/3октавы");
    plotOctaveAsHistogram->setAttribute("enumNames", QStringList({"Ломаная", "Гистограмма"}));
    m_displayNames.insert("График 1/3октавы", "plotOctaveAsHistogram");
    plotOctaveAsHistogram->setValue(se->getSetting("plotOctaveAsHistogram", 0).toInt());
    graphs->addSubProperty(plotOctaveAsHistogram);

    auto canvasDoubleClick = m_manager->addProperty(QtVariantPropertyManager::enumTypeId(), "Двойной щелчок по канве");
    canvasDoubleClick->setAttribute("enumNames",
                                    QStringList({"Создаёт новый курсор в графиках любого типа",
                                                 "Создает новый курсор, но только в спектрах"}));
    m_displayNames.insert("Двойной щелчок по канве", "canvasDoubleClick");
    canvasDoubleClick->setValue(se->getSetting("canvasDoubleClick", 1).toInt());
    graphs->addSubProperty(canvasDoubleClick);

    auto canvasDoubleClickCursor = m_manager->addProperty(QtVariantPropertyManager::enumTypeId(), "Тип курсора");
    canvasDoubleClickCursor->setAttribute("enumNames",
                                    QStringList({"Вертикальный",
                                                 "Перекрестье"}));
    m_displayNames.insert("Тип курсора", "canvasDoubleClickCursor");
    canvasDoubleClickCursor->setValue(se->getSetting("canvasDoubleClickCursor", 0).toInt());
    canvasDoubleClick->addSubProperty(canvasDoubleClickCursor);

    /* DFD */
    auto dfd = m_manager->addProperty(QVariant::String, "Файлы DFD");
    dfd->setValue(QVariant());
    item = propertyTree->addProperty(dfd);
    //propertyTree->setExpanded(item, true);

    auto thirdOctaveInitialFilter = m_manager->addProperty(QtVariantPropertyManager::enumTypeId(),
                                                           "Начальный фильтр третьоктавы");
    m_displayNames.insert("Начальный фильтр третьоктавы", "thirdOctaveInitialFilter");
    thirdOctaveInitialFilter->setToolTip("Если в файле отсутствует канал для оси X, \n"
                                         "первый фильтр будет равен этому числу");
    auto vals = OctaveFilterBank::octaveStrips(3, 40, 10, 0);
    QVariantList vals1;
    for (double v: vals) vals1 << v;
    thirdOctaveInitialFilter->setAttribute("enumNames", vals1);
    thirdOctaveInitialFilter->setValue(se->getSetting("thirdOctaveInitialFilter", 1).toInt());
    dfd->addSubProperty(thirdOctaveInitialFilter);

    connect(m_manager, SIGNAL(valueChanged(QtProperty*, const QVariant&)),
                this, SLOT(propertyChanged(QtProperty*, const QVariant&)));
}
