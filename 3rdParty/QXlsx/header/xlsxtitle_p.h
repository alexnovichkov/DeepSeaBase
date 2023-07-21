#ifndef XLSXTITLE_P_H
#define XLSXTITLE_P_H

#include <QtGlobal>
#include <QSharedData>
#include <QMap>
#include <QSet>

#include "xlsxtitle.h"
#include "xlsxshapeproperties.h"
#include "xlsxtext.h"

QT_BEGIN_NAMESPACE_XLSX

//      <xsd:element name="tx" type="CT_Tx" minOccurs="0" maxOccurs="1"/>
//      <xsd:element name="layout" type="CT_Layout" minOccurs="0" maxOccurs="1"/>
//      <xsd:element name="overlay" type="CT_Boolean" minOccurs="0" maxOccurs="1"/>
//      <xsd:element name="spPr" type="a:CT_ShapeProperties" minOccurs="0" maxOccurs="1"/>
//      <xsd:element name="txPr" type="a:CT_TextBody" minOccurs="0" maxOccurs="1"/>
//      <xsd:element name="extLst" type="CT_ExtensionList" minOccurs="0" maxOccurs="1"/>

class TitlePrivate : public QSharedData
{
public:
    Text text; //optional, c:tx
    std::optional<Title::Layout> layout;
    std::optional<bool> overlay; //optional, c:overlay
    ShapeProperties shape;
    //TextBody textBody;


    TitlePrivate();
    TitlePrivate(const TitlePrivate &other);
    ~TitlePrivate();
};

QT_END_NAMESPACE_XLSX

#endif // XLSXTITLE_P_H
