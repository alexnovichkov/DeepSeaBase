#include "logging.h"

#include <QElapsedTimer>

static int indent = 0;
#include "app.h"

int Trace::level = 0;

Trace::Trace(const QString &msg) : _msg(msg)
{
    start = std::chrono::steady_clock::now();
    LOG(TRACE) << QString(indent, ' ') << "Entering "<< _msg;
    indent+=2;
    cleanup = true;
}

Trace::Trace(int level, const QString &msg) : _msg(msg)
{
    if (level <= this->level) {
        start = std::chrono::steady_clock::now();
        LOG(TRACE) << QString(indent, ' ') << "Entering "<< _msg;
        indent+=2;
        cleanup = true;
    }
}

Trace::~Trace()
{
    if (cleanup) {
        indent-=2;
        auto diff = std::chrono::steady_clock::now() - start;
        LOG(TRACE) << QString(indent, ' ')<<"Leaving  "<<_msg << "time" <<
                      std::chrono::duration<double, std::micro>(diff).count() << "mcs";
    }
}

