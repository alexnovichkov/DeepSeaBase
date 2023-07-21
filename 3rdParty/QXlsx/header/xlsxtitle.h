#ifndef XLSXTITLE_H
#define XLSXTITLE_H

#include <QFont>
#include <QColor>
#include <QByteArray>
#include <QList>
#include <QVariant>
#include <QXmlStreamWriter>
#include <QSharedData>

#include "xlsxglobal.h"
#include "xlsxcolor.h"
#include "xlsxfillproperties.h"

QT_BEGIN_NAMESPACE_XLSX

class TitlePrivate;

class QXLSX_EXPORT Title
{
public:
    enum class LayoutTarget
    {
        Inner, //inner
        Outer //outer
    };

    enum class LayoutMode
    {
        Edge, //edge
        Factor //factor
    };

    struct Layout
    {
        std::optional<LayoutTarget> layoutTarget;
        std::optional<LayoutMode> xMode;
        std::optional<LayoutMode> yMode;
        std::optional<LayoutMode> wMode;
        std::optional<LayoutMode> hMode;

        std::optional<double> x;
        std::optional<double> y;
        std::optional<double> w;
        std::optional<double> h;

        void read(QXmlStreamReader &reader);
        void write(QXmlStreamWriter &writer);
    };


    bool isValid() const;

    void write(QXmlStreamWriter &writer) const;
    void read(QXmlStreamReader &reader);

private:
    QSharedDataPointer<TitlePrivate> d;
};

QT_END_NAMESPACE_XLSX

#endif // XLSXTITLE_H
