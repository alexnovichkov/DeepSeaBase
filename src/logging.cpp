#include "logging.h"

#include <QElapsedTimer>

static int indent = 0;

Trace::Trace(const QString &msg) : _msg(msg)
{
    start = std::chrono::high_resolution_clock::now();
    qDebug().noquote() << QString(indent, ' ') << "Entering "<< _msg;
    indent+=2;
}
Trace::~Trace()
{
    indent-=2;
    auto diff = std::chrono::high_resolution_clock::now() - start;
    qDebug().noquote() << QString(indent, ' ')<<"Leaving  "<<_msg << "time" <<
                          std::chrono::duration<double, std::milli>(diff).count() << "ms";


}

