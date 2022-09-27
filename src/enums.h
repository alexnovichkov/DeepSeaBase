#ifndef ENUMS_H
#define ENUMS_H

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

    enum class PlotType
    {
        Time,
        General,
        Octave,
        Spectrogram
    };
}


#endif // ENUMS_H
