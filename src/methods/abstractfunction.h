#ifndef ABSTRACTFUNCTION_H
#define ABSTRACTFUNCTION_H

#include <QObject>
#include <QDateTime>

class FileDescriptor;

class AbstractFunction : public QObject
{
    Q_OBJECT
public:
    explicit AbstractFunction(QList<FileDescriptor *> &dataBase, QObject *parent = nullptr);
    virtual ~AbstractFunction() {}
    virtual QString name() const = 0;
    virtual QString displayName() const = 0;
    virtual QString description() const = 0;
    virtual QString propertiesDescription() const;
    virtual QStringList properties() const = 0;
    virtual QString propertyDescription(const QString &property) const = 0;
    virtual bool propertyShowsFor(const QString &property) const;

    virtual QVariant getProperty(const QString &property) const = 0;
    virtual void setProperty(const QString &property, const QVariant &val) = 0;

    virtual QVector<double> getData(const QString &id) = 0;

    void setInput(AbstractFunction *input) {m_input = input;}


    QList<FileDescriptor *> dataBase() const {return m_dataBase;}

    QList<AbstractFunction *> functions() const {return m_functions;}

    QStringList getNewFiles() const {return newFiles;}

    // по умолчанию не делает ничего
    virtual bool compute(/*FileDescriptor *file, const QString &tempFolderName*/);
    virtual bool compute(FileDescriptor *file, const QString &tempFolderName);

    // по умолчанию возвращает пустой QVector<double>
    virtual QVector<double> get(FileDescriptor *file, const QVector<double>  &data);

    // очищает внутреннее состояние функции, но не меняет параметры, заданные ранее
    virtual void reset();
signals:
    void propertyChanged(const QString &property, const QVariant &val);
    void updateProperty(const QString &property, const QVariant &val, const QString &attribute);

    void tick();
    void tick(const QString &path);
    void finished();
    void message(const QString &s);
public slots:
    virtual void start();
protected:
    void finalize();

    QList<FileDescriptor *> m_dataBase;
    QList<AbstractFunction *> m_functions;
    AbstractFunction *m_input;

    QStringList newFiles;
    QDateTime dt;
    QString tempFolderName;

    // QObject interface
public:
    virtual bool event(QEvent *event) override;
};

#endif // ABSTRACTFUNCTION_H
