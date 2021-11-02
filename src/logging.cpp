#include "logging.h"

#include <QElapsedTimer>

static int indent = 0;

Trace::Trace(const QString &msg) : _msg(msg)
{
    time = new QElapsedTimer();
    time->start();
    qDebug().noquote() << QString(indent, ' ') << "Entering "<< _msg;
    indent+=2;
}
Trace::~Trace()
{
    indent-=2;
    qDebug().noquote() << QString(indent, ' ')<<"Leaving  "<<_msg << "time" << time->elapsed();

    delete time;
}

