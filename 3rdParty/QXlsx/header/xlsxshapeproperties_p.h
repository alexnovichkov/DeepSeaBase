#ifndef XLSXSHAPEPROPERTIES_P_H
#define XLSXSHAPEPROPERTIES_P_H

#include <QtGlobal>
#include <QSharedData>
#include <QMap>
#include <QSet>

#include "xlsxmain.h"
#include "xlsxlineformat.h"
#include "xlsxshapeproperties.h"
#include "xlsxfillproperties.h"

QT_BEGIN_NAMESPACE_XLSX

//class ShapePropertiesPrivate : public QSharedData
//{
//public:
//    std::optional<ShapeProperties::BlackWhiteMode> blackWhiteMode; //attribute, optional
//    std::optional<Transform2D> xfrm; //element, optional
//    std::optional<PresetGeometry2D> presetGeometry;
//    //TODO: CustomGeometry2D

//    FillProperties fill; // element group, optional
//    LineFormat line; //element, optional
//    //EG_EffectProperties; // ?
//    //Scene3D scene3D; // element, optional
//    //Shape3D shape3D; // element, optional
//    // TODO: extLst

//    ShapePropertiesPrivate() {}
//    ShapePropertiesPrivate(const ShapePropertiesPrivate &other);
//    ~ShapePropertiesPrivate() {}
//};


QT_END_NAMESPACE_XLSX

#endif // XLSXSHAPEPROPERTIES_P_H
