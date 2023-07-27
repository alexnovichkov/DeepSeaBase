#include "xlsxlabel.h"

QT_BEGIN_NAMESPACE_XLSX

class LabelPrivate : public QSharedData
{
public:
    LabelPrivate();
    LabelPrivate(const LabelPrivate &other);
    ~LabelPrivate();

    int index;
    Label::ShowParameters showFlags;
    std::optional<Label::Position> pos;
};

LabelPrivate::LabelPrivate()
{

}

LabelPrivate::LabelPrivate(const LabelPrivate &other) : QSharedData(other)
  //, TODO: all members
  , index{other.index}, showFlags{other.showFlags}, pos{other.pos}
{

}

LabelPrivate::~LabelPrivate()
{

}

Label::Label()
{

}

Label::Label(int index, ShowParameters show, Label::Position position)
{
    d = new LabelPrivate;
    d->index = index;
    d->showFlags = show;
    d->pos = position;
}

Label::Label(const Label &other) : d(other.d)
{

}

Label &Label::operator=(const Label &other)
{
    d = other.d;
    return *this;
}

Label::~Label()
{

}

void Label::setIndex(int index)
{
    if (!d) d = new LabelPrivate;
    d->index = index;
}

void Label::setShowParameters(ShowParameters show)
{
    if (!d) d = new LabelPrivate;
    d->showFlags = show;
}

void Label::setPosition(Label::Position position)
{
    if (!d) d = new LabelPrivate;
    d->pos = position;
}

void Label::read(QXmlStreamReader &reader)
{

}

void Label::write(QXmlStreamWriter &writer) const
{
    if (!d) return;
    writer.writeStartElement("c:dLbl");
    writer.writeEmptyElement("c:idx");
    writer.writeAttribute("val", QString::number(d->index));

    writer.writeEmptyElement("c:showVal"); writer.writeAttribute("val", d->showFlags & ShowValue ? "1" : "0");
    writer.writeEmptyElement("c:showCatName"); writer.writeAttribute("val", d->showFlags & ShowCategory ? "1" : "0");
    writer.writeEmptyElement("c:showSerName"); writer.writeAttribute("val", d->showFlags & ShowSeries ? "1" : "0");
    writer.writeEmptyElement("c:showLegendKey"); writer.writeAttribute("val", d->showFlags & ShowLegendKey ? "1" : "0");
    writer.writeEmptyElement("c:showPercent"); writer.writeAttribute("val", d->showFlags & ShowPercent ? "1" : "0");
    writer.writeEmptyElement("c:showBubbleSize"); writer.writeAttribute("val", d->showFlags & ShowBubbleSize ? "1" : "0");

    if (d->pos.has_value()) {
        writer.writeEmptyElement("c:dLblPos");
        QString s; toString(d->pos.value(), s);
        writer.writeAttribute("val", s);
    }

    writer.writeEndElement();
}

void Labels::read(QXmlStreamReader &reader)
{

}

void Labels::write(QXmlStreamWriter &writer) const
{
    if (labels.isEmpty()) {
        writer.writeEmptyElement("c:dLbls");
        return;
    }

    writer.writeStartElement("c:dLbls");
    for (const auto &l: labels) l.write(writer);

    writer.writeEmptyElement("c:showVal"); writer.writeAttribute("val", "0");
    writer.writeEmptyElement("c:showCatName"); writer.writeAttribute("val", "0");
    writer.writeEmptyElement("c:showSerName"); writer.writeAttribute("val", "0");
    writer.writeEmptyElement("c:showLegendKey"); writer.writeAttribute("val", "0");
    writer.writeEmptyElement("c:showPercent"); writer.writeAttribute("val", "0");
    writer.writeEmptyElement("c:showBubbleSize"); writer.writeAttribute("val", "0");

    writer.writeEndElement();
}

bool Labels::isEmpty() const
{
    return labels.isEmpty();
}

QT_END_NAMESPACE_XLSX




