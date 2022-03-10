#ifndef CURSORS_H
#define CURSORS_H

#include <QObject>
class Plot;
class Cursor;

class Cursors : public QObject
{
    Q_OBJECT
public:
    explicit Cursors(Plot *parent = nullptr);
    void addSingleCursor(const QPoint &pos);
signals:

private:
    Plot *plot = nullptr;
    QList<Cursor*> cursors;
};

#endif // CURSORS_H
