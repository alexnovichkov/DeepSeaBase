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
    QColor color;
    double width = 2.25; //px

    LineFormatPrivate();
    LineFormatPrivate(const LineFormatPrivate &other);
    ~LineFormatPrivate();
};


QT_END_NAMESPACE_XLSX

#endif
