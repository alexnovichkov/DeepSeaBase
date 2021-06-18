#ifndef APP_H
#define APP_H

#include <QApplication>
#include <QHash>
#include <QSharedPointer>
#include <QVector>
#include <memory>

class ColorSelector;
class QSettings;
class FileDescriptor;

using F = std::shared_ptr<FileDescriptor>;

#define App (dynamic_cast<Application *>(QCoreApplication::instance()))

class Application : public QApplication
{
    Q_OBJECT
public:
    Application(int &argc, char **argv);
    ~Application();
    QVariant getSetting(const QString &key, const QVariant &defValue=QVariant());
    void setSetting(const QString &key, const QVariant &value);
    bool hasFile(const QString &name) const {return files.contains(name);}
    F find(const QString &name) const;
    F addFile(const QString &name, bool *isNew = nullptr);
    F addFile(const FileDescriptor &source,
              const QString &name,
              const QVector<int> &indexes = QVector<int>(),
              bool *isNew = nullptr);
    void maybeDelFile(const QString& name);

    ColorSelector *colors() {return m_colors;}
private:
    void createSettings();
    QHash<QString, F> files;
    QSettings *settings = nullptr;
    ColorSelector *m_colors = nullptr;
};

#endif // APP_H
