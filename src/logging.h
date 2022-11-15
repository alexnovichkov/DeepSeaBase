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
    Trace(int level, const QString &msg);
    ~Trace();
    static int level;
    bool cleanup = false;
private:
    QString _msg;
    QString file;
    std::chrono::time_point<std::chrono::steady_clock> start;
};

#define DD    Trace trace(1, Q_FUNC_INFO);
#define DDD   Trace trace(2, Q_FUNC_INFO);
//always trace
#define DD0  Trace trace(Q_FUNC_INFO);

#endif // LOGGING_H
