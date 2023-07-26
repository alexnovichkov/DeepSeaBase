#ifndef CURSORBOX_H
#define CURSORBOX_H

#include <QTreeWidget>

class QCPPlot;
class Cursors;

class CursorBox : public QTreeWidget
{
    Q_OBJECT
public:
    explicit CursorBox(Cursors *cursors, QCPPlot *parent);
    void update();
public slots:
    void updateContents();
    void updateLayout();
    void changeFont(const QString &key, const QVariant &val);
signals:
    void closeRequested();
private:
    void copy();
    Cursors *cursors;
    QCPPlot *plot;
    bool wasHidden = true;
    QAction *copyAct;

    // QWidget interface
protected:
    virtual void closeEvent(QCloseEvent *event) override;
};

#endif // CURSORBOX_H
