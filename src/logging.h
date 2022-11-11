#ifndef LOGGING_H
#define LOGGING_H

#include <QString>
#include <QtDebug>
#include <QFile>

#include "easylogging++.h"

#include <chrono>

#define DebugPrint(s) LOG(DEBUG)<<#s<<" "<<s;

class QElapsedTimer;
class Trace {
public:
    Trace(const QString &msg);
    ~Trace();
private:
    QString _msg;
    QString file;
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
};

#define DD    if (VLOG_IS_ON(1)) Trace trace(Q_FUNC_INFO);
#define DDD   if (VLOG_IS_ON(2)) Trace trace(Q_FUNC_INFO);
//always trace
#define DD0  Trace trace(Q_FUNC_INFO);

#endif // LOGGING_H
