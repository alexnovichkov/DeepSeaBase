#ifndef APP_H
#define APP_H

#include <QApplication>
#include <QHash>
#include <QSharedPointer>
#include <QVector>
#include <memory>
#include <QtDebug>

class ColorSelector;
class FileDescriptor;
class Channel;

using F = std::shared_ptr<FileDescriptor>;

QDebug operator<<(QDebug debug, const F &f);

#define App (dynamic_cast<Application *>(QCoreApplication::instance()))

class AbstractFormatFactory;

class Application : public QApplication
{
    Q_OBJECT
public:
    Application(int &argc, char **argv);
    ~Application();
    bool hasFile(const QString &name) const {return files.contains(name);}
    F find(const QString &name) const;
    F addFile(const QString &name, bool *isNew = nullptr);
    F addFile(const FileDescriptor &source,
              const QString &name,
              const QVector<int> &indexes = QVector<int>(),
              bool *isNew = nullptr);
    F addFile(const QVector<Channel*> &source, const QString &name, bool *isNew = nullptr);
    void maybeDelFile(const QString& name);
    void loadPlugins();

    QList<QJsonObject> convertPlugins;

    ColorSelector *colors() {return m_colors.get();}
    std::unique_ptr<AbstractFormatFactory> formatFactory;
//    QTextStream logStream;
private:
    QHash<QString, F> files;
    std::unique_ptr<ColorSelector> m_colors;
};

#endif // APP_H
