#ifndef CURSOR_H
#define CURSOR_H

#include <QObject>
#include <QColor>
#include <QPointF>
#include <QFlags>

class Plot;
class Selectable;
class TrackingCursor;
class QMenu;

class Cursor : public QObject
{
    Q_OBJECT
public:
    enum class Type
    {
        Single,
        Double,
        DoubleReject,
        Harmonic,
        Peak
    };
    enum class Style
    {
        Vertical,
        Horizontal,
        Cross
    };
    enum class Format
    {
        Fixed,
        Scientific
    };
    enum InfoOption
    {
        NoInfo = 0,
        RMS = 1,
        Energy = 2,
        Reject = 4
    };
    Q_DECLARE_FLAGS(Info, InfoOption)

    Cursor(Type type, Style style, Plot *plot = nullptr);
    virtual ~Cursor() {}

    virtual void setColor(const QColor &color) {this->m_color = color;}
    virtual void moveTo(const QPointF &pos1, const QPointF &pos2) = 0;
    virtual void moveTo(const QPointF &pos1) = 0;
    virtual void moveTo(Qt::Key key, int count, TrackingCursor *source) = 0;
    virtual void moveTo(const QPointF &pos, TrackingCursor *source) = 0;
    virtual void updatePos() = 0;
    virtual void attach() = 0;
    virtual void detach() = 0;
    virtual bool contains(Selectable*) const = 0;
    virtual void update() = 0;

    virtual int dataCount(bool allData) const = 0;
    virtual QStringList dataHeader(bool allData) const = 0;
    virtual QList<double> data(int curve, bool allData) const = 0;

    void copyValues() const;
    QPointF correctedPos(QPointF oldPos, int deltaX=0, int deltaY=0) const;

    inline bool snapToValues() const {return m_snapToValues;}
    void setSnapToValues(bool snap);

    inline bool showValues() const {return m_showValues;}
    void setShowValues(bool show);

    int digits() const {return m_digits;}
    void setDigits(int digits);

    int harmonics() const {return m_harmonics;}
    void setHarmonics(int harmonics);

    Format format() const {return m_format;}
    void setFormat(Format format);

    Type type() const {return m_type;}

    Info info() const {return m_info;}
    void setInfo(Info info);

    void addRejectCursor(Cursor *c);
    void removeRejectCursor(Cursor *c);

signals:
    void cursorPositionChanged();
    void dataChanged();
protected:
//    virtual QStringList getValues() const = 0;
    Type m_type;
    Style m_style;
    QColor m_color = QColor(Qt::black);
    Plot *m_plot = nullptr;
    bool m_snapToValues = true;
    bool m_showValues = false;
    int m_digits = 2;
    int m_harmonics = 0;
    Format m_format = Format::Fixed;
    Info m_info = NoInfo;

    QList<Cursor*> rejectCursors;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Cursor::Info);

#endif // CURSOR_H
