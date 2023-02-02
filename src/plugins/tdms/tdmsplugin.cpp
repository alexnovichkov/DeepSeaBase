#include "tdmsplugin.h"

#include "tdmsconverterdialog.h"
#include "fileformats/abstractformatfactory.h"

#include "easylogging++.h"

INITIALIZE_EASYLOGGINGPP

QStringList TdmsPlugin::getConvertedFiles(AbstractFormatFactory *factory)
{
    TDMSConverterDialog dialog(factory);

    dialog.exec();
    m_addFiles = dialog.addFiles();
    return dialog.getConvertedFiles();
}
