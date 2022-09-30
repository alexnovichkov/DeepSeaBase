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
        atInvalid = 0
    };
    Q_ENUM_NS(AxisType)

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
    Q_ENUM_NS(PlotType)

    enum class InteractionMode {
        NoInteraction,
        ScalingInteraction,
        DataInteraction,
        LabelInteraction
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
    int fileNumber = -1;
    bool checked = true;
    bool selected = false;
    bool fixed = false;
};


#endif // ENUMS_H
