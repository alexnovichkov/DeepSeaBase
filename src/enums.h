#ifndef ENUMS_H
#define ENUMS_H

#include "QtMath"

namespace Enums {
    enum Direction {
        Left,
        Right,
        Up,
        Down
    };
    enum class AxisType {
        atLeft = 1,
        atRight = 2,
        atTop = 4,
        atBottom = 8,
        atInvalid = 0
    };
    enum class AxisScale {
        Linear,
        Logarithmic
    };

    enum class PlotType
    {
        Time,
        General,
        Octave,
        Spectrogram
    };
    enum class InteractionMode {
        NoInteraction,
        ScalingInteraction,
        DataInteraction,
        LabelInteraction
    };
}

struct Range {
    inline void clear() {min = INFINITY; max = -INFINITY;}
    inline double dist() const {return qAbs(max-min);}
    double min;
    double max;
};


#endif // ENUMS_H
