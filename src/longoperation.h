#ifndef LONGOPERATION_H
#define LONGOPERATION_H

#include <QApplication>

class LongOperation
{
public:
    LongOperation() {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    }
    ~LongOperation() {
        QApplication::restoreOverrideCursor();
    }
};

#endif // LONGOPERATION_H
