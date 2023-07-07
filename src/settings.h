#ifndef SETTINGS_H
#define SETTINGS_H

#include <QVariant>
#include <QObject>

class QSettings;

#define se Settings::instance()

class Settings : public QObject {
    Q_OBJECT
    Settings(QObject *parent = nullptr) : QObject(parent) {}
signals:
    void settingChanged(const QString &key, const QVariant &val);
public:
    static Settings *instance();
    QVariant getSetting(const QString &key, const QVariant &defValue=QVariant()) const;
    void setSetting(const QString &key, const QVariant &value);
    static QList<QVariant> toList(const QList<int> &list);
    static QList<int> fromList(const QList<QVariant> &list);
    bool hasSetting(const QString &key) const;
};

#endif // SETTINGS_H
