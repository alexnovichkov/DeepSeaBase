#include "app.h"
#include <QFile>
#include <QSettings>
#include <QStandardPaths>
#include "fileformats/filedescriptor.h"
#include "fileformats/formatfactory.h"
#include "logging.h"
#include "settings.h"
#include "qeasysettings.hpp"

Application::Application(int &argc, char **argv) : QApplication(argc, argv)
{DD;
    Trace::level = el::Loggers::verboseLevel();
    formatFactory = std::make_unique<FormatFactory>();

//#ifdef APP_PORTABLE
//    QEasySettings::init(QEasySettings::Format::iniFormat, "DeepSea Database");
//#else
//    QEasySettings::init(QEasySettings::Format::regFormat, "DeepSea Database");
//#endif

//    //reading and applying style
//    QFile cssFile("qtstyle.css");
//    if (cssFile.open(QFile::Text | QFile::ReadOnly)) {
//        QString style = cssFile.readAll();
//        setStyleSheet(style);
//    }

//    QEasySettings::writeStyle(QEasySettings::Style::autoFusion);
//    auto currentStyle = QEasySettings::readStyle();
//    QEasySettings::setStyle(currentStyle);

//    QString themePrefix;
//    //follow system theme
//    if (currentStyle == QEasySettings::Style::autoFusion) {
//        QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
//                           QSettings::NativeFormat);
//        if(settings.value("AppsUseLightTheme")==0)
//            themePrefix = "[dark]";
//        else
//            themePrefix = "[light]";
//    }
//    else if (currentStyle == QEasySettings::Style::darkFusion)
//        themePrefix = "[dark]";
//    else {
//        themePrefix = "[light]";
//    }
//    if (themePrefix == "[light]") alternateTextColor = "#505050";
//    if (themePrefix == "[dark]") alternateTextColor = "#b0b0b0";

//    //some icon themes may have no dark mode
//    iconTheme = se->value(QSL("iconTheme"),QSL("maia")).toString();
//    if (QFileInfo::exists("icons/"+iconTheme+themePrefix)) {
//        QIcon::setThemeName(iconTheme+themePrefix);
//        isDarkTheme = themePrefix == "[dark]";
//    }
//    else
//        QIcon::setThemeName(iconTheme);

    //setStyle(new MyProxyStyle(style()));
}

Application::~Application()
{DD;
    for (auto f: qAsConst(files)) f.reset();
}

F Application::find(const QString &name) const
{DD;
    return files.value(name);
}

F Application::addFile(const QString &name, bool *isNew)
{DD;
    if (files.contains(name)) {
        if (isNew) *isNew = false;
        return files.value(name);
    }

    F f(formatFactory->createDescriptor(name));
    if (f)
        files.insert(name, f);
    if (isNew) *isNew = true;

    return f;
}

F Application::addFile(const FileDescriptor &source, const QString &name, const QVector<int> &indexes, bool *isNew)
{DD;
    if (files.contains(name)) {
        if (isNew) *isNew = false;
        return files.value(name);
    }

    F f(formatFactory->createDescriptor(source, name, indexes));
    if (f) {
        files.insert(name, f);
        if (isNew) *isNew = true;
    }
    return f;
}

F Application::addFile(const QVector<Channel*> &source, const QString &name, bool *isNew)
{DD;
    if (files.contains(name)) {
        if (isNew) *isNew = false;
        return files.value(name);
    }

    F f(formatFactory->createDescriptor(source, name));
    if (f) {
        files.insert(name, f);
        if (isNew) *isNew = true;
    }
    return f;
}

void Application::maybeDelFile(const QString &name)
{DD;
    F& f = files[name];
    if (f.use_count()<2) {
        f.reset();
        files.remove(name);
    }
}

void Application::loadPlugins()
{DD;
    QDir pluginsDir = QDir(qApp->applicationDirPath()+"/plugins");
    const QFileInfoList potentialPlugins = pluginsDir.entryInfoList(QDir::Files);
    for (const QFileInfo &fileName: potentialPlugins) {
        QString path = fileName.canonicalFilePath();
        QPluginLoader loader(path);
        QJsonObject metaData = loader.metaData().value("MetaData").toObject();

        if (metaData.isEmpty()) continue;
        metaData.insert("path",path);
        QString pluginInterface = metaData.value("interface").toString();
        if (pluginInterface == "IConvertPlugin")
            convertPlugins << metaData;
//        if (pluginInterface=="IQoobarPlugin")
//            plugins << metaData;
    }
}

QDebug operator<<(QDebug debug, const F &f)
{
    debug << f->fileName();
    return debug;
}
