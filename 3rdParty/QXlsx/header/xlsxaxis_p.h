#ifndef XLSXAXIS_P_H
#define XLSXAXIS_P_H

#include <QtGlobal>
#include <QSharedData>
#include <QMap>
#include <QSet>

#include "xlsxaxis.h"
#include "xlsxshapeproperties.h"

QT_BEGIN_NAMESPACE_XLSX

//<xsd:group name="EG_AxShared">
//  <xsd:sequence>
//    <xsd:element name="axId" type="CT_UnsignedInt" minOccurs="1" maxOccurs="1"/>
//    <xsd:element name="scaling" type="CT_Scaling" minOccurs="1" maxOccurs="1"/>
//    <xsd:element name="delete" type="CT_Boolean" minOccurs="0" maxOccurs="1"/>
//    <xsd:element name="axPos" type="CT_AxPos" minOccurs="1" maxOccurs="1"/>
//    <xsd:element name="majorGridlines" type="CT_ChartLines" minOccurs="0" maxOccurs="1"/>
//    <xsd:element name="minorGridlines" type="CT_ChartLines" minOccurs="0" maxOccurs="1"/>
//    <xsd:element name="title" type="CT_Title" minOccurs="0" maxOccurs="1"/>
//    <xsd:element name="numFmt" type="CT_NumFmt" minOccurs="0" maxOccurs="1"/>
//    <xsd:element name="majorTickMark" type="CT_TickMark" minOccurs="0" maxOccurs="1"/>
//    <xsd:element name="minorTickMark" type="CT_TickMark" minOccurs="0" maxOccurs="1"/>
//    <xsd:element name="tickLblPos" type="CT_TickLblPos" minOccurs="0" maxOccurs="1"/>
//    <xsd:element name="spPr" type="a:CT_ShapeProperties" minOccurs="0" maxOccurs="1"/>
//    <xsd:element name="txPr" type="a:CT_TextBody" minOccurs="0" maxOccurs="1"/>
//    <xsd:element name="crossAx" type="CT_UnsignedInt" minOccurs="1" maxOccurs="1"/>
//    <xsd:choice minOccurs="0" maxOccurs="1">
//      <xsd:element name="crosses" type="CT_Crosses" minOccurs="1" maxOccurs="1"/>
//      <xsd:element name="crossesAt" type="CT_Double" minOccurs="1" maxOccurs="1"/>
//    </xsd:choice>
//  </xsd:sequence>
//</xsd:group>

//class AxisPrivate : public QSharedData
//{
//public:
//    Axis::Type type;
//    Axis::Position axisPos;
//    int axisId;
//    int crossAxis;
//    Axis::Scaling scaling;

//    std::optional<QString> axisName; //TODO: write full text support
//    std::optional<bool> visible;

//    ShapeProperties majorGridlines;
//    ShapeProperties minorGridlines;

//    QString title; // temporary solution
////    Title title;
////    NumberFormat numberFormat;

////    TickMark majorTickMark;
////    TickMark minorTickMark;

//    AxisPrivate();
//    AxisPrivate(const AxisPrivate &other);
//    ~AxisPrivate() {}
//};


QT_END_NAMESPACE_XLSX

#endif // XLSXAXIS_P_H
