#include <QtGlobal>
#include <QDataStream>
#include <QDebug>

#include "xlsxtitle.h"
#include "xlsxtitle_p.h"

QT_BEGIN_NAMESPACE_XLSX

bool Title::isValid() const
{
    if (d) return true;
    return false;
}

void Title::write(QXmlStreamWriter &writer) const
{

}

void Title::read(QXmlStreamReader &reader)
{

}

TitlePrivate::TitlePrivate()
{

}

TitlePrivate::TitlePrivate(const TitlePrivate &other) : QSharedData(other)
  //TODO: all other members
{

}

TitlePrivate::~TitlePrivate()
{

}

void Title::Layout::read(QXmlStreamReader &reader)
{
    const auto &name = reader.name();

    while (!reader.atEnd()) {
        auto token = reader.readNext();
        if (token == QXmlStreamReader::StartElement) {
            const auto &val = reader.attributes().value("val");
            if (reader.name() == QLatin1String("layoutTarget")) {
                if (val == "inner") layoutTarget = LayoutTarget::Inner;
                if (val == "outer") layoutTarget = LayoutTarget::Outer;
            }
            if (reader.name() == QLatin1String("xMode")) {
                if (val == "edge") xMode = LayoutMode::Edge;
                if (val == "factor") xMode = LayoutMode::Factor;
            }
            if (reader.name() == QLatin1String("yMode")) {
                if (val == "edge") yMode = LayoutMode::Edge;
                if (val == "factor") yMode = LayoutMode::Factor;
            }
            if (reader.name() == QLatin1String("wMode")) {
                if (val == "edge") wMode = LayoutMode::Edge;
                if (val == "factor") wMode = LayoutMode::Factor;
            }
            if (reader.name() == QLatin1String("hMode")) {
                if (val == "edge") hMode = LayoutMode::Edge;
                if (val == "factor") hMode = LayoutMode::Factor;
            }
            if (reader.name() == QLatin1String("x")) x = val.toDouble();
            if (reader.name() == QLatin1String("y")) y = val.toDouble();
            if (reader.name() == QLatin1String("w")) w = val.toDouble();
            if (reader.name() == QLatin1String("h")) h = val.toDouble();
        }
        else if (token == QXmlStreamReader::EndElement && reader.name() == name)
            break;
    }
}

void Title::Layout::write(QXmlStreamWriter &writer)
{
    writer.writeStartElement("c:layout");
    writer.writeStartElement("c:manualLayout");

    if (layoutTarget.has_value()) {
        writer.writeEmptyElement("c:layoutTarget");
        switch (layoutTarget.value()) {
            case LayoutTarget::Inner: writer.writeAttribute("val", "inner"); break;
            case LayoutTarget::Outer: writer.writeAttribute("val", "outer"); break;
        }
    }
    if (xMode.has_value()) {
        writer.writeEmptyElement("c:xMode");
        switch (xMode.value()) {
            case LayoutMode::Edge: writer.writeAttribute("val", "edge"); break;
            case LayoutMode::Factor: writer.writeAttribute("val", "factor"); break;
        }
    }
    if (yMode.has_value()) {
        writer.writeEmptyElement("c:yMode");
        switch (yMode.value()) {
            case LayoutMode::Edge: writer.writeAttribute("val", "edge"); break;
            case LayoutMode::Factor: writer.writeAttribute("val", "factor"); break;
        }
    }
    if (wMode.has_value()) {
        writer.writeEmptyElement("c:wMode");
        switch (wMode.value()) {
            case LayoutMode::Edge: writer.writeAttribute("val", "edge"); break;
            case LayoutMode::Factor: writer.writeAttribute("val", "factor"); break;
        }
    }
    if (hMode.has_value()) {
        writer.writeEmptyElement("c:hMode");
        switch (hMode.value()) {
            case LayoutMode::Edge: writer.writeAttribute("val", "edge"); break;
            case LayoutMode::Factor: writer.writeAttribute("val", "factor"); break;
        }
    }

    writer.writeEndElement();
    writer.writeEndElement();
}

QT_END_NAMESPACE_XLSX
