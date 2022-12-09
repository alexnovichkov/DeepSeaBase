#ifndef CURSORS_H
#define CURSORS_H

#include <QObject>
class Plot;
class Selectable;

#include "cursor.h"

class Cursors : public QObject
{
    Q_OBJECT
public:
    explicit Cursors(Plot *parent = nullptr);
    Cursor *addSingleCursor(const QPoint &pos, Cursor::Style style);
    Cursor *addDoubleCursor(const QPoint &pos, Cursor::Style style);
    Cursor *addRejectCursor(const QPoint &pos, Cursor::Style style);
    Cursor *addHarmonicCursor(const QPoint &pos);

    inline bool isEmpty() const {return m_cursors.isEmpty();}
    QList<Cursor*> cursors() const {return m_cursors;}
    int dataCount() const;
    QStringList dataHeader() const;
    QStringList data(int curveIndex) const;
    Cursor *cursorFor(Selectable *selected) const;
public slots:
    void removeCursor(Cursor *cursor);
    void update();
signals:
    void cursorsChanged();
    void cursorPositionChanged();
private:
    Cursor *addDoubleCursor(const QPoint &pos, Cursor::Style style, bool reject);
    Plot *plot = nullptr;
    QList<Cursor*> m_cursors;
};

#endif // CURSORS_H
