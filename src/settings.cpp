#include "settings.h"

#include <QSettings>
#include <QFile>

QSettings createSettings()
{
    if (QFile::exists("portable")) {
        return QSettings("deepseabase.ini", QSettings::IniFormat);
    }
    else {
        return QSettings("Alex Novichkov","DeepSea Database");
    }
}

QVariant Settings::getSetting(const QString &key, const QVariant &defValue)
{
    auto s = createSettings();
    return s.value(key, defValue);
}

void Settings::setSetting(const QString &key, const QVariant &value)
{
    auto s = createSettings();
    s.setValue(key, value);
}

