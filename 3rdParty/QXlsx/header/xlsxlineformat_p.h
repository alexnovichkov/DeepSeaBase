// xlsxlineformat_p.h
#ifndef XLSXLINEFORMAT_P_H
#define XLSXLINEFORMAT_P_H

#include <QtGlobal>
#include <QSharedData>
#include <QMap>
#include <QSet>

#include "xlsxlineformat.h"

QT_BEGIN_NAMESPACE_XLSX

class LineFormatPrivate : public QSharedData
{
public:
    LineFormat::LineType lineType = LineFormat::LT_SolidLine;
    QColor color;
    double alpha = 0; // [0..1]
    double width = 2.25; //px
    bool smooth = false;
    LineFormat::CompoundLineType compoundLineType = LineFormat::CLT_Single;
    LineFormat::StrokeType strokeType = LineFormat::ST_Solid;
    LineFormat::PointType pointType = LineFormat::PT_Square;

    LineFormatPrivate();
    LineFormatPrivate(const LineFormatPrivate &other);
    ~LineFormatPrivate();
};


QT_END_NAMESPACE_XLSX

#endif
