#ifndef XLSXMARKERFORMAT_P_H
#define XLSXMARKERFORMAT_P_H

#include <QtGlobal>
#include <QSharedData>
#include <QMap>
#include <QSet>
#include <optional>
#include "xlsxmarkerformat.h"

QT_BEGIN_NAMESPACE_XLSX

class MarkerFormatPrivate : public QSharedData
{
public:
    std::optional<int> size; // [2..72]
    std::optional<MarkerFormat::MarkerType> markerType;
    ShapeProperties shape;

    MarkerFormatPrivate();
    MarkerFormatPrivate(const MarkerFormatPrivate &other);
    ~MarkerFormatPrivate();
};


QT_END_NAMESPACE_XLSX

#endif // XLSXMARKERFORMAT_P_H
