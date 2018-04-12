#ifndef LOGGING_H
#define LOGGING_H

#include <QString>

//#define DO_TRACE


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

#endif // LOGGING_H
