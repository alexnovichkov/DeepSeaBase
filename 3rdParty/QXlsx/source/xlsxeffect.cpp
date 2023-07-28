#include "xlsxeffect.h"


QXlsx::Effect::Effect()
{

}

QXlsx::Effect::Effect(const QXlsx::Effect &other) : d(other.d)
{

}

QXlsx::Effect::~Effect()
{

}

QXlsx::Effect &QXlsx::Effect::operator=(const QXlsx::Effect &other)
{
    d = other.d;
    return *this;
}

bool QXlsx::Effect::operator ==(const QXlsx::Effect &other) const
{
    if (d == other.d) return true;
    return *d.constData() == *other.d.constData();
}

bool QXlsx::Effect::operator !=(const QXlsx::Effect &other) const
{
    return *d.constData() != *other.d.constData();
}

void QXlsx::Effect::read(QXmlStreamReader &reader)
{
    const auto &name = reader.name();
    if (name == QLatin1String("effectLst")) {
        readEffectList(reader);
    }
    else if (name == QLatin1String("effectDag")) {

    }
}

void QXlsx::Effect::write(QXmlStreamWriter &writer) const
{

}

void QXlsx::Effect::readEffectList(QXmlStreamReader &reader)
{
    const auto &name = reader.name();
    while (!reader.atEnd()) {
        auto token = reader.readNext();

        if (token == QXmlStreamReader::StartElement) {
            const auto &a = reader.attributes();
            if (reader.name() == QLatin1String("blur")) {
                parseAttribute(a, QLatin1String("rad"), d->blurRadius);
                parseAttributeBool(a, QLatin1String("grow"), d->blurGrow);
            }
            if (reader.name() == QLatin1String("fillOverlay")) {
                FillBlendMode t;
                fromString(a.value(QLatin1String("blend")).toString(), t);
                d->fillBlendMode = t;
                reader.readNextStartElement();
                d->fillOverlay.read(reader);
            }
            if (reader.name() == QLatin1String("glow")) {
                parseAttribute(a, QLatin1String("rad"), d->glowRadius);
                reader.readNextStartElement();
                d->glowColor.read(reader);
            }
            if (reader.name() == QLatin1String("innerShdw")) {
                parseAttribute(a, QLatin1String("dir"), d->innerShadowDirection);
                parseAttribute(a, QLatin1String("dist"), d->innerShadowOffset);
                parseAttribute(a, QLatin1String("blurRad"), d->innerShadowBlurRadius);
                reader.readNextStartElement();
                d->innerShadowColor.read(reader);
            }
            if (reader.name() == QLatin1String("outerShdw")) {
                parseAttribute(a, QLatin1String("dir"), d->outerShadowDirection);
                parseAttribute(a, QLatin1String("dist"), d->outerShadowOffset);
                parseAttribute(a, QLatin1String("blurRad"), d->outerShadowBlurRadius);
                parseAttributePercent(a, QLatin1String("sx"), d->outerShadowHorizontalScalingFactor);
                parseAttributePercent(a, QLatin1String("sy"), d->outerShadowVerticalScalingFactor);
                parseAttribute(a, QLatin1String("kx"), d->outerShadowHorizontalSkewFactor);
                parseAttribute(a, QLatin1String("ky"), d->outerShadowVerticalSkewFactor);
                parseAttributeBool(a, QLatin1String("rotWithShape"), d->outerShadowRotateWithShape);
                if (a.hasAttribute(QLatin1String("algn"))) {
                    Alignment t;
                    fromString(a.value(QLatin1String("algn")).toString(), t);
                    d->outerShadowAlignment = t;
                }
                reader.readNextStartElement();
                d->innerShadowColor.read(reader);
            }
            if (reader.name() == QLatin1String("prstShdw")) {
                parseAttribute(a, QLatin1String("dir"), d->presetShadowDirection);
                parseAttribute(a, QLatin1String("dist"), d->presetShadowOffset);
                parseAttributeInt(a, QLatin1String("prst"), d->presetShadow);
                reader.readNextStartElement();
                d->presetShadowColor.read(reader);
            }
            if (reader.name() == QLatin1String("reflection")) {}
            if (reader.name() == QLatin1String("softEdge")) {}
        }
        else if (token == QXmlStreamReader::EndElement && reader.name() == name)
            break;
    }
}

void QXlsx::Effect::writeEffectList(QXmlStreamWriter &writer) const
{

}

QXlsx::EffectPrivate::EffectPrivate()
{

}

QXlsx::EffectPrivate::EffectPrivate(const QXlsx::EffectPrivate &other) : QSharedData(other)
  // TODO: all members
{

}

QXlsx::EffectPrivate::~EffectPrivate()
{

}

bool QXlsx::EffectPrivate::operator ==(const EffectPrivate &other) const
{

}

bool QXlsx::EffectPrivate::operator !=(const EffectPrivate &other) const
{

}
