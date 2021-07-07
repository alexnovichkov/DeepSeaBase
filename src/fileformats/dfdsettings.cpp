#include "dfdsettings.h"
#include <QtCore>

DfdSettings::DfdSettings(const QString &fileName) : m_fileName(fileName)
{

}

void DfdSettings::read()
{
    content.clear();
    m_childGroups.clear();

    QFile file(m_fileName);
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        char buf[2048];
        qint64 lineLength;
        QString group;
        QTextCodec *codec = QTextCodec::codecForName("Windows-1251");

        while ((lineLength = file.readLine(buf, sizeof(buf))) != -1) {
            // the line is available in buf
            QByteArray b = QByteArray(buf, lineLength);
            if (!b.isEmpty()) {
                QString s = codec->toUnicode(b);
                s = s.trimmed();
                if (s.startsWith('[')) {
                    s.chop(1);
                    s.remove(0,1);
                    group = s;
                    if (!m_childGroups.contains(group))
                        m_childGroups << group;
                }
                else {
                    int ind = s.indexOf('=');
                    if (ind>0) {
                        QString key = group+"/"+s.mid(0,ind);
                        QString val = s.mid(ind+1);
                        content.append({key, val});
                    }
                }
            }
        }
    }
}

QString DfdSettings::value(const QString &key) const
{
    for (const DescriptionEntry &entry: content) {
        if (entry.first == key) return entry.second;
    }
    return QString();
}

DescriptionList DfdSettings::values(const QString &group) const
{
    DescriptionList result;
    if (group.isEmpty()) return result;

    for (const DescriptionEntry &entry: content) {
        if (entry.first.startsWith(group+"/")) {
            QString key = entry.first;
            result.append({key.remove(0, group.length()+1), entry.second});
        }
    }
    return result;
}
