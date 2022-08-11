#include "settings.h"

#include <QSettings>
#include <QFile>
#include "logging.h"

QSettings createSettings()
{DDD;
    if (QFile::exists("portable")) {
        return QSettings("deepseabase.ini", QSettings::IniFormat);
    }
    else {
        return QSettings("Alex Novichkov","DeepSea Database");
    }
}

QVariant Settings::getSetting(const QString &key, const QVariant &defValue)
{DDD;
    auto s = createSettings();
    return s.value(key, defValue);
}

void Settings::setSetting(const QString &key, const QVariant &value)
{DDD;
    auto s = createSettings();
    s.setValue(key, value);
}

