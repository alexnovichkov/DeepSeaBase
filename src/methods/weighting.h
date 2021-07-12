#ifndef WEIGHTING_H
#define WEIGHTING_H

#include <QString>

namespace Weighting
{
    enum class Type
    {
        No = 0,
        A = 1,
        B = 2,
        C = 3,
        D = 4
    };
    Type fromString(QString s);
    QString toString(Type type);
}

#endif // WEIGHTING_H
