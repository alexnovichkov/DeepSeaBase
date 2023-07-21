#include "xlsxmain.h"

#include <QtDebug>
#include <QMetaEnum>

#include "xlsxutility_p.h"

QT_BEGIN_NAMESPACE_XLSX

bool Transform2D::isValid() const
{
    bool valid = offset.has_value();
    valid |= extension.has_value();
    valid |= rotation.has_value();
    valid |= flipHorizontal.has_value();
    valid |= flipVertical.has_value();

    return valid;
}

void Transform2D::write(QXmlStreamWriter &writer, const QString& name) const
{
    if (isValid()) {
        writer.writeStartElement(name);
        if (rotation.has_value()) writer.writeAttribute(QLatin1String("rot"), rotation.value().toString());
        if (flipHorizontal.has_value()) writer.writeAttribute(QLatin1String("flipH"), flipHorizontal.value() ? "true" : "false");
        if (flipVertical.has_value()) writer.writeAttribute(QLatin1String("flipV"), flipVertical.value() ? "true" : "false");
        if (extension.has_value()) {
            writer.writeEmptyElement(QLatin1String("a:ext"));
            writer.writeAttribute(QLatin1String("cx"), QString::number(extension.value().width()));
            writer.writeAttribute(QLatin1String("cy"), QString::number(extension.value().height()));
        }
        if (offset.has_value()) {
            writer.writeEmptyElement(QLatin1String("a:off"));
            writer.writeAttribute(QLatin1String("x"), QString::number(offset.value().x()));
            writer.writeAttribute(QLatin1String("y"), QString::number(offset.value().y()));
        }
        writer.writeEndElement();
    }
    else writer.writeEmptyElement(name);
}

void Transform2D::read(QXmlStreamReader &reader)
{
    const auto &name = reader.name();
    const auto &attr = reader.attributes();
    if (attr.hasAttribute(QLatin1String("rot")))
        rotation = Angle::create(attr.value(QLatin1String("rot")));
    parseAttributeBool(attr, QLatin1String("flipH"), flipHorizontal);
    parseAttributeBool(attr, QLatin1String("flipV"), flipVertical);

    while (!reader.atEnd()) {
        auto token = reader.readNext();
        if (token == QXmlStreamReader::StartElement) {
            if (reader.name() == QLatin1String("ext")) {
                extension.emplace(QSize(reader.attributes().value(QLatin1String("cx")).toULong(),
                                        reader.attributes().value(QLatin1String("cy")).toULong()));
            }
            else if (reader.name() == QLatin1String("off")) {
                offset.emplace(QPoint(reader.attributes().value(QLatin1String("x")).toLong(),
                                        reader.attributes().value(QLatin1String("y")).toLong()));
            }
        }
        else if (token == QXmlStreamReader::EndElement && reader.name() == name)
            break;
    }
}

void PresetGeometry2D::write(QXmlStreamWriter &writer, const QString &name) const
{
    writer.writeStartElement(name);
    auto meta = QMetaEnum::fromType<ShapeType>();
    writer.writeAttribute(QLatin1String("prst"), meta.valueToKey(static_cast<int>(prst)));
    if (avLst.isEmpty()) writer.writeEmptyElement(QLatin1String("avLst"));
    else {
        writer.writeStartElement(QLatin1String("a:avLst"));
        for (const auto &gd: avLst) {
            writer.writeEmptyElement(QLatin1String("a:gd"));
            writer.writeAttribute(QLatin1String("name"), gd.name);
            writer.writeAttribute(QLatin1String("fmla"), gd.formula);
        }
        writer.writeEndElement();
    }
    writer.writeEndElement();
}

void PresetGeometry2D::read(QXmlStreamReader &reader)
{
    const auto &name = reader.name();

    if (reader.attributes().hasAttribute(QLatin1String("prst"))) {
        auto meta = QMetaEnum::fromType<ShapeType>();
        prst = static_cast<ShapeType>(meta.keyToValue(reader.attributes().value(QLatin1String("prst")).toLatin1().data()));
    }
    while (!reader.atEnd()) {
        auto token = reader.readNext();
        if (token == QXmlStreamReader::StartElement) {
            if (reader.name() == QLatin1String("avLst")) {
                //
            }
            else if (reader.name() == QLatin1String("gd")) {
                avLst.append(GeometryGuide(reader.attributes().value(QLatin1String("name")).toString(),
                             reader.attributes().value(QLatin1String("fmla")).toString()));
            }
        }
        else if (token == QXmlStreamReader::EndElement && reader.name() == name)
            break;
    }
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const Transform2D &f)
{
    dbg.nospace() << "QXlsx::Transform2D(";
    if (f.offset.has_value()) dbg << "offset="<<f.offset.value()<<", ";
    if (f.extension.has_value()) dbg << "extension="<<f.extension.value()<<", ";
    if (f.rotation.has_value()) dbg << "rotation="<<f.rotation->toString()<<", ";
    if (f.flipHorizontal.has_value()) dbg << "flipHorizontal="<<f.flipHorizontal.value()<<", ";
    if (f.flipVertical.has_value()) dbg << "offset="<<f.flipVertical.value();
    dbg << ")";
    return dbg.space();
}

void PresetTextShape::write(QXmlStreamWriter &writer, const QString &name) const
{
    writer.writeStartElement(name);
    auto meta = QMetaEnum::fromType<TextShapeType>();
    writer.writeAttribute(QLatin1String("prst"), meta.valueToKey(static_cast<int>(prst)));
    if (avLst.isEmpty()) writer.writeEmptyElement(QLatin1String("a:avLst"));
    else {
        writer.writeStartElement(QLatin1String("a:avLst"));
        for (const auto &gd: avLst) {
            writer.writeEmptyElement(QLatin1String("a:gd"));
            writer.writeAttribute(QLatin1String("name"), gd.name);
            writer.writeAttribute(QLatin1String("fmla"), gd.formula);
        }
        writer.writeEndElement();
    }
    writer.writeEndElement();
}

void PresetTextShape::read(QXmlStreamReader &reader)
{
    const auto &name = reader.name();

    if (reader.attributes().hasAttribute(QLatin1String("prst"))) {
        auto meta = QMetaEnum::fromType<TextShapeType>();
        prst = static_cast<TextShapeType>(meta.keyToValue(reader.attributes().value(QLatin1String("prst")).toLatin1().data()));
    }
    while (!reader.atEnd()) {
        auto token = reader.readNext();
        if (token == QXmlStreamReader::StartElement) {
            if (reader.name() == QLatin1String("avLst")) {
                //
            }
            else if (reader.name() == QLatin1String("gd")) {
                avLst.append(GeometryGuide(reader.attributes().value(QLatin1String("name")).toString(),
                             reader.attributes().value(QLatin1String("fmla")).toString()));
            }
        }
        else if (token == QXmlStreamReader::EndElement && reader.name() == name)
            break;
    }
}

void Scene3D::write(QXmlStreamWriter &writer, const QString &name) const
{
    writer.writeStartElement(name);
    writeCamera(writer, QLatin1String("a:camera"));
    writeLightRig(writer, QLatin1String("a:lightRig"));
    if (backdropPlane.has_value())
        writeBackdrop(writer, QLatin1String("a:backdrop"));

    writer.writeEndElement();
}

void Scene3D::read(QXmlStreamReader &reader)
{
    const auto &name = reader.name();

    while (!reader.atEnd()) {
        auto token = reader.readNext();
        if (token == QXmlStreamReader::StartElement) {
            if (reader.name() == QLatin1String("camera")) {
                readCamera(reader);
            }
            else if (reader.name() == QLatin1String("lightRig")) {
                readLightRig(reader);
            }
            else if (reader.name() == QLatin1String("backdrop")) {
                readBackdrop(reader);
            }
        }
        else if (token == QXmlStreamReader::EndElement && reader.name() == name)
            break;
    }
}

void Scene3D::readCamera(QXmlStreamReader &reader)
{
    const auto &name = reader.name();
    const auto &a = reader.attributes();
    if (a.hasAttribute(QLatin1String("prst"))) {
        auto meta = QMetaEnum::fromType<CameraType>();
        camera.type = static_cast<CameraType>(meta.keyToValue(reader.attributes().value(QLatin1String("prst")).toLatin1().data()));
    }
    if (a.hasAttribute(QLatin1String("fov"))) {
        camera.fovAngle = Angle::create(reader.attributes().value(QLatin1String("fov")));
    }
    parseAttributePercent(a, QLatin1String("zoom"), camera.zoom);

    while (!reader.atEnd()) {
        auto token = reader.readNext();
        if (token == QXmlStreamReader::StartElement) {
            if (reader.name() == QLatin1String("rot")) {
                camera.rotation->resize(3);
                camera.rotation.value()[0] = Angle::create(reader.attributes().value(QLatin1String("lat")));
                camera.rotation.value()[1] = Angle::create(reader.attributes().value(QLatin1String("lon")));
                camera.rotation.value()[2] = Angle::create(reader.attributes().value(QLatin1String("rev")));
            }
        }
        else if (token == QXmlStreamReader::EndElement && reader.name() == name)
            break;
    }
}

void Scene3D::writeCamera(QXmlStreamWriter &writer, const QString &name) const
{
    writer.writeStartElement(name);
    auto meta = QMetaEnum::fromType<CameraType>();
    writer.writeAttribute(QLatin1String("prst"), meta.valueToKey(static_cast<int>(camera.type)));
    if (camera.fovAngle.has_value()) {
        writer.writeAttribute(QLatin1String("fov"), camera.fovAngle->toString());
    }
    if (camera.zoom.has_value()) {
        writer.writeAttribute(QLatin1String("zoom"), toST_Percent(camera.zoom.value()));
    }
    if (camera.rotation->size()==3) {
        writer.writeEmptyElement(QLatin1String("a:rot"));
        writer.writeAttribute(QLatin1String("lat"), camera.rotation.value()[0].toString());
        writer.writeAttribute(QLatin1String("lon"), camera.rotation.value()[1].toString());
        writer.writeAttribute(QLatin1String("rev"), camera.rotation.value()[2].toString());
    }
    writer.writeEndElement();
}

void Scene3D::readLightRig(QXmlStreamReader &reader)
{
    const auto &name = reader.name();
    const auto &a = reader.attributes();
    if (a.hasAttribute(QLatin1String("rig"))) {
        auto meta = QMetaEnum::fromType<LightRigType>();
        lightRig.type = static_cast<LightRigType>(meta.keyToValue(a.value(QLatin1String("rig")).toLatin1().data()));
    }
    if (a.hasAttribute(QLatin1String("dir"))) {
        const auto &val = a.value(QLatin1String("dir"));
        if (val == QLatin1String("tl")) lightRig.lightDirection = LightRigDirection::TopLeft;
        if (val == QLatin1String("t")) lightRig.lightDirection = LightRigDirection::Top;
        if (val == QLatin1String("tr")) lightRig.lightDirection = LightRigDirection::TopRight;
        if (val == QLatin1String("l")) lightRig.lightDirection = LightRigDirection::Left;
        if (val == QLatin1String("r")) lightRig.lightDirection = LightRigDirection::Right;
        if (val == QLatin1String("bl")) lightRig.lightDirection = LightRigDirection::BottomLeft;
        if (val == QLatin1String("b")) lightRig.lightDirection = LightRigDirection::Bottom;
        if (val == QLatin1String("br")) lightRig.lightDirection = LightRigDirection::BottomRight;
    }

    while (!reader.atEnd()) {
        auto token = reader.readNext();
        if (token == QXmlStreamReader::StartElement) {
            if (reader.name() == QLatin1String("rot")) {
                lightRig.rotation->resize(3);
                lightRig.rotation.value()[0] = Angle::create(reader.attributes().value(QLatin1String("lat")));
                lightRig.rotation.value()[1] = Angle::create(reader.attributes().value(QLatin1String("lon")));
                lightRig.rotation.value()[2] = Angle::create(reader.attributes().value(QLatin1String("rev")));
            }
        }
        else if (token == QXmlStreamReader::EndElement && reader.name() == name)
            break;
    }
}

void Scene3D::writeLightRig(QXmlStreamWriter &writer, const QString &name) const
{
    writer.writeStartElement(name);
    auto meta = QMetaEnum::fromType<LightRigType>();
    writer.writeAttribute(QLatin1String("rig"), meta.valueToKey(static_cast<int>(lightRig.type)));
    switch (lightRig.lightDirection) {
        case LightRigDirection::Top: writer.writeAttribute(QLatin1String("dir"), QLatin1String("t")); break;
        case LightRigDirection::Left: writer.writeAttribute(QLatin1String("dir"), QLatin1String("l")); break;
        case LightRigDirection::Right: writer.writeAttribute(QLatin1String("dir"), QLatin1String("r")); break;
        case LightRigDirection::Bottom: writer.writeAttribute(QLatin1String("dir"), QLatin1String("b")); break;
        case LightRigDirection::TopLeft: writer.writeAttribute(QLatin1String("dir"), QLatin1String("tl")); break;
        case LightRigDirection::TopRight: writer.writeAttribute(QLatin1String("dir"), QLatin1String("tr")); break;
        case LightRigDirection::BottomLeft: writer.writeAttribute(QLatin1String("dir"), QLatin1String("bl")); break;
        case LightRigDirection::BottomRight: writer.writeAttribute(QLatin1String("dir"), QLatin1String("br")); break;
    }

    if (lightRig.rotation->size()==3) {
        writer.writeEmptyElement(QLatin1String("a:rot"));
        writer.writeAttribute(QLatin1String("lat"), lightRig.rotation.value()[0].toString());
        writer.writeAttribute(QLatin1String("lon"), lightRig.rotation.value()[1].toString());
        writer.writeAttribute(QLatin1String("rev"), lightRig.rotation.value()[2].toString());
    }
    writer.writeEndElement();
}

void Scene3D::readBackdrop(QXmlStreamReader &reader)
{
    const auto &name = reader.name();

    while (!reader.atEnd()) {
        auto token = reader.readNext();
        if (token == QXmlStreamReader::StartElement) {
            if (reader.name() == QLatin1String("anchor")) {
                const auto &a = reader.attributes();
                backdropPlane->anchor.resize(3);
                backdropPlane->anchor[0] = Coordinate::create(a.value(QLatin1String("x")).toString());
                backdropPlane->anchor[1] = Coordinate::create(a.value(QLatin1String("y")).toString());
                backdropPlane->anchor[2] = Coordinate::create(a.value(QLatin1String("z")).toString());
            }
            if (reader.name() == QLatin1String("norm")) {
                const auto &a = reader.attributes();
                backdropPlane->norm.resize(3);
                backdropPlane->norm[0] = Coordinate::create(a.value(QLatin1String("dx")).toString());
                backdropPlane->norm[1] = Coordinate::create(a.value(QLatin1String("dy")).toString());
                backdropPlane->norm[2] = Coordinate::create(a.value(QLatin1String("dz")).toString());
            }
            if (reader.name() == QLatin1String("up")) {
                const auto &a = reader.attributes();
                backdropPlane->up.resize(3);
                backdropPlane->up[0] = Coordinate::create(a.value(QLatin1String("dx")).toString());
                backdropPlane->up[1] = Coordinate::create(a.value(QLatin1String("dy")).toString());
                backdropPlane->up[2] = Coordinate::create(a.value(QLatin1String("dz")).toString());
            }
        }
        else if (token == QXmlStreamReader::EndElement && reader.name() == name)
            break;
    }
}

void Scene3D::writeBackdrop(QXmlStreamWriter &writer, const QString &name) const
{
    writer.writeStartElement(name);
    if (backdropPlane->anchor.size()==3) {
        writer.writeEmptyElement(QLatin1String("a:anchor"));
        writer.writeAttribute(QLatin1String("x"), backdropPlane->anchor[0].toString());
        writer.writeAttribute(QLatin1String("y"), backdropPlane->anchor[1].toString());
        writer.writeAttribute(QLatin1String("z"), backdropPlane->anchor[2].toString());
    }
    if (backdropPlane->norm.size()==3) {
        writer.writeEmptyElement(QLatin1String("a:norm"));
        writer.writeAttribute(QLatin1String("dx"), backdropPlane->norm[0].toString());
        writer.writeAttribute(QLatin1String("dy"), backdropPlane->norm[1].toString());
        writer.writeAttribute(QLatin1String("dz"), backdropPlane->norm[2].toString());
    }
    if (backdropPlane->up.size()==3) {
        writer.writeEmptyElement(QLatin1String("a:up"));
        writer.writeAttribute(QLatin1String("dx"), backdropPlane->up[0].toString());
        writer.writeAttribute(QLatin1String("dy"), backdropPlane->up[1].toString());
        writer.writeAttribute(QLatin1String("dz"), backdropPlane->up[2].toString());
    }
    writer.writeEndElement();
}

Coordinate::Coordinate(qint64 emu)
{
    this->val = QVariant::fromValue<qint64>(emu);
}

Coordinate::Coordinate(const QString &val)
{
    this->val = val;
}

Coordinate::Coordinate(double points)
{
    this->val = QVariant::fromValue<double>(points);
}

qint64 Coordinate::toEMU() const
{
    if (val.userType() == QMetaType::LongLong)
        return val.value<qint64>();
    if (val.userType() == QMetaType::Double)
        return qint64(val.value<double>() * 12700); //TODO: Check other methods of rounding

    return {};
}

QString Coordinate::toString() const
{
    if (val.userType() == QMetaType::LongLong)
        return val.toString();
    if (val.userType() == QMetaType::Double) {
        qint64 v = toEMU();
        return QString::number(v);
    }
    return val.toString();
}

double Coordinate::toPoints() const
{
    qint64 v = toEMU();
    return double(v) / 12700.0;
}

void Coordinate::setEMU(qint64 val)
{
    this->val = QVariant::fromValue<qint64>(val);
}

void Coordinate::setString(const QString &val)
{
    this->val = val;
}

void Coordinate::setPoints(double points)
{
    this->val = QVariant::fromValue<double>(points);
}

Coordinate Coordinate::create(const QString &val)
{
    bool ok;
    long long n = val.toLongLong(&ok);
    if (ok) return Coordinate(n);

    return Coordinate(val);
}

Coordinate Coordinate::create(const QStringRef &val)
{
    bool ok;
    long long n = val.toLongLong(&ok);
    if (ok) return Coordinate(n);

    return Coordinate(val.toString());
}

void Bevel::write(QXmlStreamWriter &writer, const QString &name) const
{
    writer.writeEmptyElement(name);
    if (width.has_value()) writer.writeAttribute(QLatin1String("w"), width.value().toString());
    if (height.has_value()) writer.writeAttribute(QLatin1String("h"), height.value().toString());
    if (type.has_value()) {
        auto meta = QMetaEnum::fromType<BevelType>();
        writer.writeAttribute(QLatin1String("prst"), meta.valueToKey(static_cast<int>(type.value())));
    }
}

void Bevel::read(QXmlStreamReader &reader)
{
    const auto &a = reader.attributes();
    if (a.hasAttribute(QLatin1String("w"))) width = Coordinate::create(a.value(QLatin1String("w")).toString());
    if (a.hasAttribute(QLatin1String("h"))) height = Coordinate::create(a.value(QLatin1String("w")).toString());
    if (a.hasAttribute(QLatin1String("prst"))) {
        auto meta = QMetaEnum::fromType<BevelType>();
        type = static_cast<BevelType>(meta.keyToValue(a.value(QLatin1String("prst")).toLatin1().data()));
    }
}

void Shape3D::write(QXmlStreamWriter &writer, const QString &name) const
{
    writer.writeStartElement(name);
    if (z.has_value()) writer.writeAttribute(QLatin1String("z"), z.value().toString());
    if (extrusionHeight.has_value()) writer.writeAttribute(QLatin1String("extrusionH"), extrusionHeight.value().toString());
    if (contourWidth.has_value()) writer.writeAttribute(QLatin1String("contourW"), contourWidth.value().toString());

    if (material.has_value()) {
        auto meta = QMetaEnum::fromType<MaterialType>();
        writer.writeAttribute(QLatin1String("prstMaterial"), meta.valueToKey(static_cast<int>(material.value())));
    }

    if (bevelTop.has_value()) bevelTop.value().write(writer, QLatin1String("a:bevelT"));
    if (bevelBottom.has_value()) bevelBottom.value().write(writer, QLatin1String("a:bevelB"));
    if (extrusionColor.has_value()) {
        writer.writeStartElement(QLatin1String("a:extrusionClr"));
        extrusionColor->write(writer);
        writer.writeEndElement();
    }
    if (contourColor.has_value()) {
        writer.writeStartElement(QLatin1String("a:contourClr"));
        contourColor->write(writer);
        writer.writeEndElement();
    }

    writer.writeEndElement();
}

void Shape3D::read(QXmlStreamReader &reader)
{
    const auto &name = reader.name();
    const auto &a = reader.attributes();

    if (a.hasAttribute(QLatin1String("prstMaterial"))) {
        auto meta = QMetaEnum::fromType<MaterialType>();
        material = static_cast<MaterialType>(meta.keyToValue(a.value(QLatin1String("prstMaterial")).toLatin1().data()));
    }
    if (a.hasAttribute(QLatin1String("z"))) {
        z = Coordinate::create(a.value(QLatin1String("z")).toString());
    }
    if (a.hasAttribute(QLatin1String("extrusionH"))) {
        extrusionHeight = Coordinate::create(a.value(QLatin1String("extrusionH")).toString());
    }
    if (a.hasAttribute(QLatin1String("contourW"))) {
        contourWidth = Coordinate::create(a.value(QLatin1String("contourW")).toString());
    }

    while (!reader.atEnd()) {
        auto token = reader.readNext();
        if (token == QXmlStreamReader::StartElement) {
            if (reader.name() == QLatin1String("bevelT")) {
                bevelTop->read(reader);
            }
            if (reader.name() == QLatin1String("bevelB")) {
                bevelBottom->read(reader);
            }
            if (reader.name() == QLatin1String("extrusionClr")) {
                reader.readNextStartElement();
                extrusionColor->read(reader);
            }
            if (reader.name() == QLatin1String("contourClr")) {
                reader.readNextStartElement();
                contourColor->read(reader);
            }
        }
        else if (token == QXmlStreamReader::EndElement && reader.name() == name)
            break;
    }
}

Angle::Angle(qint64 angleInFrac) : val(angleInFrac)
{
}

Angle::Angle(double angleInDegrees) : val(qint64(angleInDegrees * 60000))
{
}

qint64 Angle::toFrac() const
{
    return val;
}

double Angle::toDegrees() const
{
    return double(val) / 60000.0;
}

QString Angle::toString() const
{
    return QString::number(val);
}

void Angle::setFrac(qint64 frac)
{
    val = frac;
}

void Angle::setDegrees(double degrees)
{
    val = qint64(degrees * 60000);
}

Angle Angle::create(const QStringRef &s)
{
    bool ok;
    qint64 val = s.toLongLong(&ok);
    if (ok) return Angle(val);
    return Angle();
}

void parseAttribute(const QXmlStreamAttributes &a, const QLatin1String &name, std::optional<Angle> &target)
{
    if (a.hasAttribute(name)) target = Angle::create(a.value(name));
}

void parseAttribute(const QXmlStreamAttributes &a, const QLatin1String &name, std::optional<Coordinate> &target)
{
    if (a.hasAttribute(name)) target = Coordinate::create(a.value(name));
}

#endif

QT_END_NAMESPACE_XLSX



