#ifndef LOGGING_H
#define LOGGING_H

#include <QString>
#include <QtDebug>

#define DO_TRACE

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

#ifdef DO_TRACE


#define DD  Trace trace(Q_FUNC_INFO);
#else
#define DD
#endif

#define DDD  Trace trace(Q_FUNC_INFO);

#endif // LOGGING_H
