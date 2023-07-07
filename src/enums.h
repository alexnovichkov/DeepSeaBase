#ifndef ENUMS_H
#define ENUMS_H

#include "QtMath"
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
        atLeft = 1,
        atRight = 2,
        atTop = 4,
        atBottom = 8,
        atColor = 16,
        atInvalid = 0
    };
    Q_ENUM_NS(AxisType)

    enum class AxisScale {
        Linear,
        Logarithmic,
        ThirdOctave
    };
    Q_ENUM_NS(AxisScale)

    enum class PlotType
    {
        Time,
        General,
        Octave,
        Spectrogram
    };
    Q_ENUM_NS(PlotType)

    enum class InteractionMode {
        ScalingInteraction,
        DataInteraction
    };
    enum LegendData {
        ldColor = 0,
        ldTitle,
        ldFileNumber,
        ldSelected,
        ldFixed
    };
}

struct Range {
    inline void clear() {min = INFINITY; max = -INFINITY;}
    inline double dist() const {return qAbs(max-min);}
    double min;
    double max;
};

#include <QColor>

struct LegendData
{
    QString text;
    QColor color;
    int fileNumber = 0;
    bool checked = true;
    bool selected = false;
    bool fixed = false;
};

struct PointBlock
{
    double minX = 0;
    double maxX = 0;
    double minY = 0;
    double maxY = 0;
    int from = 0;
    int to = 0;
};

struct AxisParameters
{
    double tickStep = 0;
    double subTickStep = 0;
    bool tickStepAutomatic = true;
    bool subTickStepAutomatic = true;
    Enums::AxisScale scale = Enums::AxisScale::Linear;
};

#endif // ENUMS_H
