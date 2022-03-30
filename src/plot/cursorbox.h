#ifndef CURSORBOX_H
#define CURSORBOX_H

#include <QTreeWidget>

class Plot;
class Cursors;

class CursorBox : public QTreeWidget
{
    Q_OBJECT
public:
    explicit CursorBox(Cursors *cursors, Plot *parent);
    void update();
public slots:
    void updateContents();
    void updateLayout();
signals:
    void closeRequested();
private:
    void copy();
    Cursors *cursors;
    Plot *plot;
    bool wasHidden = true;
    QAction *copyAct;

    // QWidget interface
protected:
    virtual void closeEvent(QCloseEvent *event) override;
};

#endif // CURSORBOX_H
