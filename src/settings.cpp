#include "settings.h"

#include <QSettings>
#include <QFile>
#include "logging.h"

QSettings createSettings()
{DD;
    if (QFile::exists("portable")) {
        return QSettings("deepseabase.ini", QSettings::IniFormat);
    }
    else {
        return QSettings("Alex Novichkov","DeepSea Database");
    }
}

QVariant Settings::getSetting(const QString &key, const QVariant &defValue) const
{DD;
    auto s = createSettings();
    return s.value(key, defValue);
}

void Settings::setSetting(const QString &key, const QVariant &value)
{DD;
    auto s = createSettings();
    auto oldValue = s.value(key);
    if (oldValue != value) {
        s.setValue(key, value);
        emit settingChanged(key, value);
    }
}


QList<QVariant> Settings::toList(const QList<int> &list)
{DD;
    QList<QVariant> result;
    for (int val: list) result << QVariant(val);
    return result;
}

QList<int> Settings::fromList(const QList<QVariant> &list)
{DD;
    QList<int> result;
    for (auto val: list) result << val.toInt();
    return result;
}

bool Settings::hasSetting(const QString &key) const
{
    static QStringList allSettings {
        "font",
        "cursorDialogFont",
        "plotOctaveAsHistogram",
        "thirdOctaveInitialFilter",
        "canvasDoubleClick",
        "canvasDoubleClickCursor",
        "spectrogramTimeScale", //0 - время, 1 - дистанция
        "legendMiddleButton",
        "defaultFileFormat",
        "defaultYAxisPresentation"
    };

    if (key.isEmpty()) return false;

    return allSettings.contains(key);
}

Settings *Settings::instance()
{
    static Settings *inst = nullptr;
    if (!inst) {
        inst = new Settings();
    }
    return inst;
}
