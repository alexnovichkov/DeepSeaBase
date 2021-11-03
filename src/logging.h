#ifndef LOGGING_H
#define LOGGING_H

#include <QString>
#include <QtDebug>

/// maximum trace level, DDDD
//#define TRACE_MAX
/// medium trace level, DDD
//#define TRACE_MED
/// minimum trace level, DD
//#define TRACE_MIN


#define DebugPrint(s) qDebug()<<#s<<s;

class QElapsedTimer;
class Trace {
public:
    Trace(const QString &msg);
    ~Trace();
private:
    QString _msg;
    QElapsedTimer *time;
};

#ifdef TRACE_MIN
#define DD  Trace trace(Q_FUNC_INFO);
#else
#define DD
#endif
#ifdef TRACE_MED
#define DDD Trace trace(Q_FUNC_INFO);
#else
#define DDD
#endif
#ifdef TRACE_MAX
#define DDDD  Trace trace(Q_FUNC_INFO);
#else
#define DDDD
#endif

//always trace
#define DD0  Trace trace(Q_FUNC_INFO);

#endif // LOGGING_H
