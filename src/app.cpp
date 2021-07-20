#include "app.h"
#include <QFile>
#include <QSettings>
#include "fileformats/filedescriptor.h"
#include "fileformats/formatfactory.h"
#include "colorselector.h"

Application::Application(int &argc, char **argv) : QApplication(argc, argv)
{
    QVariantList list = getSetting("colors").toList();
    m_colors = new ColorSelector(list);
}

Application::~Application()
{
    for (auto f: files) f.reset();

    setSetting("colors", m_colors->getColors());
    delete m_colors;
}

QVariant Application::getSetting(const QString &key, const QVariant &defValue)
{
    if (!settings) createSettings();
    return settings->value(key, defValue);
}

void Application::setSetting(const QString &key, const QVariant &value)
{
    if (!settings) createSettings();
    settings->setValue(key, value);
}

F Application::find(const QString &name) const
{
    return files.value(name);
}

F Application::addFile(const QString &name, bool *isNew)
{
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
{
    if (files.contains(name)) {
        if (isNew) *isNew = false;
        return files.value(name);
    }

    F f(FormatFactory::createDescriptor(source, name, indexes));
    if (f)
        files.insert(name, f);
    if (isNew) *isNew = true;

    return f;
}

void Application::maybeDelFile(const QString &name)
{
    F& f = files[name];
    if (f.use_count()<2) {
        f.reset();
        files.remove(name);
    }
}

void Application::createSettings()
{
    if (QFile::exists("portable")) {
        settings = new QSettings("deepseabase.ini", QSettings::IniFormat);
    }
    else {
        settings = new QSettings("Alex Novichkov","DeepSea Database");
    }
}
