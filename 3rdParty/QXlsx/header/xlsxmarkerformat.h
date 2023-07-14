#ifndef XLSXMARKERFORMAT_H
#define XLSXMARKERFORMAT_H

#include <QFont>
#include <QColor>
#include <QByteArray>
#include <QList>
#include <QExplicitlySharedDataPointer>
#include <QVariant>

#include "xlsxglobal.h"

QT_BEGIN_NAMESPACE_XLSX

class Styles;
class Worksheet;
class WorksheetPrivate;
class RichStringPrivate;
class SharedStrings;

class MarkerFormatPrivate;

class QXLSX_EXPORT MarkerFormat
{
public:
    enum MarkerType {
        MT_NoMarker,
        MT_Square,
        MT_Diamond,
        MT_Triangle,
        MT_Cross,
        MT_Star,
        MT_Dot,
        MT_Dash,
        MT_Circle,
        MT_Plus
    };

    MarkerFormat();
    MarkerFormat(const MarkerFormat &other);
    MarkerFormat &operator=(const MarkerFormat &rhs);
    ~MarkerFormat();

    MarkerType markerType() const;
    void setMarkerType(MarkerType type);

    int size() const;
    void setSize(int size);

    bool isValid() const;
    QByteArray formatKey() const;

    bool operator == (const MarkerFormat &format) const;
    bool operator != (const MarkerFormat &format) const;

private:
    friend   QDebug operator<<(QDebug, const MarkerFormat &f);

    QExplicitlySharedDataPointer<MarkerFormatPrivate> d;
};

#ifndef QT_NO_DEBUG_STREAM
  QDebug operator<<(QDebug dbg, const MarkerFormat &f);
#endif

QT_END_NAMESPACE_XLSX

#endif // XLSXMARKERFORMAT_H
