#include "legend.h"
#include <QtDebug>
#include "legendlabel.h"

Legend::Legend(QWidget *parent) :
    QwtLegend(parent)
{
}

void Legend::itemMarkedForDelete()
{
    QWidget *w = qobject_cast<QWidget *>( sender() );
    if ( w )
    {
        const QVariant info = itemInfo( w );
        if ( info.isValid() )
        {
            const QList<QWidget *> widgetList =
                legendWidgets( info );

            const int index = widgetList.indexOf( w );
            if ( index >= 0 ) {
//                qDebug()<<index <<info;
                Q_EMIT markedForDelete(info, index );
            }
        }
    }
}

QWidget *Legend::createWidget(const QwtLegendData &data) const
{
    Q_UNUSED( data );

    LegendLabel *label = new LegendLabel();
    label->setItemMode( defaultItemMode() );

    connect( label, SIGNAL( clicked() ), SLOT( itemClicked() ) );
    connect( label, SIGNAL( checked( bool ) ), SLOT( itemChecked( bool ) ) );
    connect( label, SIGNAL(markedForDelete()), SLOT(itemMarkedForDelete()));

    return label;
}

void Legend::updateWidget(QWidget *widget, const QwtLegendData &data)
{
    LegendLabel *label = qobject_cast<LegendLabel *>( widget );
    if ( label )
    {
        label->setData( data );
        if ( !data.value( QwtLegendData::ModeRole ).isValid() )
        {
            // use the default mode, when there is no specific
            // hint from the legend data

            label->setItemMode( defaultItemMode() );
        }
    }
}
