#ifndef TDMSPLUGIN_H
#define TDMSPLUGIN_H

#include <QObject>
#include "../convertplugin.h"
class AbstractFormatFactory;

class TdmsPlugin : public QObject, IConvertPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "qoobar.IConvertPlugin/1.0" FILE "tdms.json")
    Q_INTERFACES(IConvertPlugin)
public:
    virtual bool addFiles() {return m_addFiles;}
    virtual QStringList getConvertedFiles(AbstractFormatFactory *factory);
private:
    bool m_addFiles = false;
};

#endif
