#ifndef LEGEND_H
#define LEGEND_H

#include "qwt_legend.h"
#include <QSet>

class Legend : public QwtLegend
{
    Q_OBJECT
public:
    explicit Legend(QWidget *parent = 0);
signals:
    void markedForDelete( const QVariant &itemInfo, int index );

private slots:
    void itemMarkedForDelete();
protected:
    virtual QWidget *createWidget( const QwtLegendData & ) const;
    virtual void updateWidget( QWidget *widget, const QwtLegendData &data );


    // QwtAbstractLegend interface
public slots:
    virtual void updateLegend(const QVariant &itemInfo, const QList<QwtLegendData> &data);
};

#endif // LEGEND_H
