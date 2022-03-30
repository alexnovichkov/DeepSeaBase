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
    void addSingleCursor(const QPoint &pos, Cursor::Style style);
    void addDoubleCursor(const QPoint &pos, Cursor::Style style);
    void addRejectCursor(const QPoint &pos, Cursor::Style style);
    void addHarmonicCursor(const QPoint &pos);

    inline bool isEmpty() const {return m_cursors.isEmpty();}
    QList<Cursor*> cursors() const {return m_cursors;}
    int dataCount() const;
    QStringList dataHeader() const;
    QStringList data(int curveIndex) const;
public slots:
    void removeCursor(Selectable *selected);
    void update();
signals:
    void cursorsChanged();
    void cursorPositionChanged();
private:
    void addDoubleCursor(const QPoint &pos, Cursor::Style style, bool reject);
    Plot *plot = nullptr;
    QList<Cursor*> m_cursors;
};

#endif // CURSORS_H
