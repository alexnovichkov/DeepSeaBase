#ifndef LOGGING_H
#define LOGGING_H

#include <QString>

//#define DO_TRACE

#define DebugPrint(s) qDebug()<<#s<<s;

class QTime;
class Trace {
public:
    Trace(const QString &msg);
    ~Trace();
private:
    QString _msg;
    QTime *time;
};

#ifdef DO_TRACE


#define DD  Trace trace(Q_FUNC_INFO);
#else
#define DD
#endif

#define DDD  Trace trace(Q_FUNC_INFO);

#endif // LOGGING_H