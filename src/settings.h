#ifndef SETTINGS_H
#define SETTINGS_H

#include <QVariant>

class QSettings;

namespace Settings {
    QVariant getSetting(const QString &key, const QVariant &defValue=QVariant());
    void setSetting(const QString &key, const QVariant &value);
    QList<QVariant> toList(const QList<int> &list);
    QList<int> fromList(const QList<QVariant> &list);
}

#endif // SETTINGS_H
