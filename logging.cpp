#include "logging.h"

//#ifdef DO_TRACE

#include <QTime>
#include <QtDebug>


Trace::Trace(const QString &msg) : _msg(msg)
{
    time = new QTime();
    time->start();
    qDebug()<<"Entering "<<_msg;
}
Trace::~Trace()
{
    qDebug()<<"Leaving  "<<_msg << "time" << time->elapsed();
    delete time;
}
//#endif
