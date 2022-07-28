#ifndef IConvertPlugin_H
#define IConvertPlugin_H

#include <QStringList>

class IConvertPlugin
{
public:
    virtual ~IConvertPlugin() {}

    /*does all editing*/
    //virtual QList<Tag> getNewTags(const QList<Tag> &oldTags) = 0;

    /*returns true if plugin can work with no files selected
     * (f.e. adding files */
    //virtual bool canWorkWithNoFilesSelected() = 0;
};

Q_DECLARE_INTERFACE(IConvertPlugin, "deepsea.IConvertPlugin/1.0")

#endif // IConvertPlugin
