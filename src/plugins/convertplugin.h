#ifndef IConvertPlugin_H
#define IConvertPlugin_H

#include <QStringList>
#include <QtPlugin>
class AbstractFormatFactory;

class IConvertPlugin
{
public:
    virtual ~IConvertPlugin() {}

    virtual bool addFiles() = 0;
    virtual QStringList getConvertedFiles(AbstractFormatFactory *factory) = 0;
};

Q_DECLARE_INTERFACE(IConvertPlugin, "deepsea.IConvertPlugin/1.0")

#endif // IConvertPlugin
