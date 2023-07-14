#ifndef XLSXMARKERFORMAT_P_H
#define XLSXMARKERFORMAT_P_H

#include <QtGlobal>
#include <QSharedData>
#include <QMap>
#include <QSet>

#include "xlsxmarkerformat.h"

QT_BEGIN_NAMESPACE_XLSX

class MarkerFormatPrivate : public QSharedData
{
public:
    int size = 7; // [2..]
    MarkerFormat::MarkerType markerType = MarkerFormat::MT_Diamond;


    MarkerFormatPrivate();
    MarkerFormatPrivate(const MarkerFormatPrivate &other);
    ~MarkerFormatPrivate();
};


QT_END_NAMESPACE_XLSX

#endif // XLSXMARKERFORMAT_P_H
