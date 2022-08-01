#include "app.h"
#include <QFile>
#include <QSettings>
#include "fileformats/filedescriptor.h"
#include "fileformats/formatfactory.h"
#include "colorselector.h"
#include "logging.h"
#include "settings.h"

Application::Application(int &argc, char **argv) : QApplication(argc, argv)
{DD;
    QVariantList list = Settings::getSetting("colors").toList();
    m_colors = new ColorSelector(list);
}

Application::~Application()
{DD;
    for (auto f: qAsConst(files)) f.reset();

    Settings::setSetting("colors", m_colors->getColors());
    delete m_colors;
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

    F f(FormatFactory::createDescriptor(name));
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

    F f(FormatFactory::createDescriptor(source, name, indexes));
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

    F f(FormatFactory::createDescriptor(source, name));
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
