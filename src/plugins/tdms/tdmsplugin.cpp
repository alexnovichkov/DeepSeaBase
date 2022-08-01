#include "tdmsplugin.h"

#include <QtWidgets>
#include <QJsonDocument>
#include <QApplication>

#include "tdmsconverterdialog.h"

QStringList TdmsPlugin::getConvertedFiles()
{
    TDMSConverterDialog dialog;
    if (dialog.exec()) {
        m_addFiles = dialog.addFiles();
        return dialog.getConvertedFiles();
    }
    return QStringList();
}
