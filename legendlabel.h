#ifndef LEGENDLABEL_H
#define LEGENDLABEL_H

#include "qwt_legend_label.h"

class LegendLabel : public QwtLegendLabel
{
    Q_OBJECT
public:
    explicit LegendLabel(QWidget *parent = 0);
signals:
    void markedForDelete();
    void markedToMoveToRight();
protected:
    //virtual void paintEvent( QPaintEvent * );
    virtual void mousePressEvent( QMouseEvent * );
    virtual void mouseReleaseEvent( QMouseEvent * );
    virtual void keyPressEvent( QKeyEvent * );
//    virtual void keyReleaseEvent( QKeyEvent * );
    void updateState(bool state, QwtLegendData::Mode mode);

};

#endif // LEGENDLABEL_H
