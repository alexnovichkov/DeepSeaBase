#ifndef ABSTRACTMETHOD_H
#define ABSTRACTMETHOD_H

#include <QStringList>

class DfdFileDescriptor;

class AbstractMethod
{
public:
    virtual int id() = 0;
    virtual QStringList methodSettings(DfdFileDescriptor *dfd, int activeChannel, int strip) = 0;
    virtual QString methodDll() = 0;
    virtual int panelType() = 0;
    virtual QString methodName() = 0;
    virtual int dataType() = 0;
    QList<DfdFileDescriptor *> *dataBase;
    int strip;
};

#endif // ABSTRACTMETHOD_H
