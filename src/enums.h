#ifndef ENUMS_H
#define ENUMS_H

#include <QObject>

namespace Enums {
    Q_NAMESPACE
    enum Direction {
        Left,
        Right,
        Up,
        Down
    };
    enum class AxisType {
        atLeft,
        atRight,
        atTop,
        atBottom,
        atInvalid
    };
    Q_ENUM_NS(AxisType)
}


#endif // ENUMS_H
