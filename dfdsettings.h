#ifndef DFDSETTINGS_H
#define DFDSETTINGS_H

#include <QStringList>
#include <QVariant>

#include "filedescriptor.h"

class DfdSettings
{
public:
    DfdSettings(const QString &fileName);
    QStringList childGroups() const {return m_childGroups;}
    void read();

    /**
     * @brief value - first found value of the @param key
     * @param key string "group/key" or "key"
     * @return QString value
     */
    QString value(const QString &key) const;
    DescriptionList values(const QString &group) const;
private:
    QString m_fileName;
    QStringList m_childGroups;
    DescriptionList content;
    //QStringList m_childGroups;
};

#endif // DFDSETTINGS_H
