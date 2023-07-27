#ifndef XLSXLABEL_H
#define XLSXLABEL_H

#include "xlsxglobal.h"

#include <QSharedData>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <optional>

#include "xlsxutility_p.h"

QT_BEGIN_NAMESPACE_XLSX

class LabelPrivate;

class QXLSX_EXPORT Label
{
public:
    enum class Position
    {
        BestFit,
        Left,
        Right,
        Top,
        Bottom,
        Center,
        InBase,
        InEnd,
        OutEnd
    };

    enum ShowParameter
    {
        ShowLegendKey = 1,
        ShowValue = 2,
        ShowCategory = 4,
        ShowSeries = 8,
        ShowPercent = 16,
        ShowBubbleSize = 32
    };
    Q_DECLARE_FLAGS(ShowParameters, ShowParameter)

    Label();
    Label(int index, ShowParameters show, Position position);
    Label(const Label &other);
    Label &operator=(const Label &other);
    ~Label();

    void setIndex(int index);
    void setShowParameters(ShowParameters show);
    void setPosition(Position position);

    void read(QXmlStreamReader &reader);
    void write(QXmlStreamWriter &writer) const;
private:
    SERIALIZE_ENUM(Position, {
        {Position::BestFit, "bestFit"},
        {Position::Left, "l"},
        {Position::Right, "r"},
        {Position::Top, "t"},
        {Position::Bottom, "b"},
        {Position::Center, "ctr"},
        {Position::InBase, "inBase"},
        {Position::InEnd, "inEnd"},
        {Position::OutEnd, "outEnd"},
    });

    QSharedDataPointer<LabelPrivate> d;
};

class SharedLabelProperties
{
public:

};

class Labels
{
public:
    QList<Label> labels;
    std::optional<bool> visible;
    std::optional<bool> showLeaderLines;
    // TODO: <xsd:element name="leaderLines" type="CT_ChartLines" minOccurs="0" maxOccurs="1"/>
    SharedLabelProperties properties;

    void read(QXmlStreamReader &reader);
    void write(QXmlStreamWriter &writer) const;
    bool isEmpty() const;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Label::ShowParameters)

QT_END_NAMESPACE_XLSX



#endif // XLSXLABEL_H
