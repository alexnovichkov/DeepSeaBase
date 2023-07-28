#ifndef XLSXMARKERFORMAT_H
#define XLSXMARKERFORMAT_H

#include <QFont>
#include <QColor>
#include <QByteArray>
#include <QList>
#include <QExplicitlySharedDataPointer>
#include <QVariant>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "xlsxglobal.h"
#include "xlsxshapeproperties.h"

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
    enum class MarkerType {
        None,
        Square,
        Diamond,
        Triangle,
        Cross,
        Star,
        Dot,
        Dash,
        Circle,
        Plus,
        Picture,
        X,
        Auto
    };

    MarkerFormat();
    MarkerFormat(MarkerType type);
    MarkerFormat(const MarkerFormat &other);
    MarkerFormat &operator=(const MarkerFormat &rhs);
    ~MarkerFormat();

    std::optional<MarkerType> type() const;
    void setType(MarkerType type);

    std::optional<int> size() const;
    void setSize(int size);

    void write(QXmlStreamWriter &writer);
    void read(QXmlStreamReader &reader);

    Shape shape() const;
    void setShape(Shape shape);

    bool isValid() const;

private:
    friend QDebug operator<<(QDebug, const MarkerFormat &f);

    QSharedDataPointer<MarkerFormatPrivate> d;
};

#ifndef QT_NO_DEBUG_STREAM
  QDebug operator<<(QDebug dbg, const MarkerFormat &f);
#endif

QT_END_NAMESPACE_XLSX

#endif // XLSXMARKERFORMAT_H
