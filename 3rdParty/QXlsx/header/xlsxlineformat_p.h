// xlsxlineformat_p.h
#ifndef XLSXLINEFORMAT_P_H
#define XLSXLINEFORMAT_P_H

#include <QtGlobal>
#include <QSharedData>
#include <QMap>
#include <QSet>

#include "xlsxlineformat.h"
#include "xlsxcolor.h"
#include "xlsxfillproperties.h"
#include "xlsxmain.h"

QT_BEGIN_NAMESPACE_XLSX

//<xsd:complexType name="CT_LineProperties">
//    <xsd:sequence>
//      <xsd:group ref="EG_LineFillProperties" minOccurs="0" maxOccurs="1"/>
//      <xsd:group ref="EG_LineDashProperties" minOccurs="0" maxOccurs="1"/>
//      <xsd:group ref="EG_LineJoinProperties" minOccurs="0" maxOccurs="1"/>
//      <xsd:element name="headEnd" type="CT_LineEndProperties" minOccurs="0" maxOccurs="1"/>
//      <xsd:element name="tailEnd" type="CT_LineEndProperties" minOccurs="0" maxOccurs="1"/>
//      <xsd:element name="extLst" type="CT_OfficeArtExtensionList" minOccurs="0" maxOccurs="1"/>
//    </xsd:sequence>
//    <xsd:attribute name="w" type="ST_LineWidth" use="optional"/>
//    <xsd:attribute name="cap" type="ST_LineCap" use="optional"/>
//    <xsd:attribute name="cmpd" type="ST_CompoundLine" use="optional"/>
//    <xsd:attribute name="algn" type="ST_PenAlignment" use="optional"/>
//  </xsd:complexType>

class LineFormatPrivate : public QSharedData
{
public:
    FillProperties fill;
    Coordinate width; //pt or EMU
    std::optional<LineFormat::CompoundLineType> compoundLineType;
    std::optional<LineFormat::StrokeType> strokeType;
    std::optional<LineFormat::LineCap> lineCap;
    std::optional<LineFormat::PenAlignment> penAlignment;

    std::optional<LineFormat::LineJoin> lineJoin;

    std::optional<LineFormat::LineEndType> tailType;
    std::optional<LineFormat::LineEndType> headType;
    std::optional<LineFormat::LineEndSize> tailLength;
    std::optional<LineFormat::LineEndSize> headLength;
    std::optional<LineFormat::LineEndSize> tailWidth;
    std::optional<LineFormat::LineEndSize> headWidth;

    LineFormatPrivate();
    LineFormatPrivate(const LineFormatPrivate &other);
    ~LineFormatPrivate();

    bool operator == (const LineFormatPrivate &format) const;
};


QT_END_NAMESPACE_XLSX

#endif
