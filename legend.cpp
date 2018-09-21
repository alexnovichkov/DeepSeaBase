#include "legend.h"
#include <QtDebug>
#include "legendlabel.h"
#include "qwt_plot_item.h"

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


void Legend::updateLegend(const QVariant &itemInfo, const QList<QwtLegendData> &data)
{
//    qDebug()<<Q_FUNC_INFO;
//    qDebug()<<"data size"<<data.size();
//    if (!data.isEmpty()) {
//        foreach(QwtLegendData i, data) {
//            qDebug()<<i.title().text();
//        }
//    }
//    QList<QWidget *> widgetList = legendWidgets( itemInfo );
//    qDebug()<<"widget list size"<<widgetList.size();

//    QwtPlotItem *item = 0;
//    if ( itemInfo.canConvert<QwtPlotItem *>() )
//            item = qvariant_cast<QwtPlotItem *>( itemInfo );
//    if (item) {

//    }

    QwtLegend::updateLegend(itemInfo, data);

    //// updateLegend from QwtLegend
//    QList<QWidget *> widgetList = legendWidgets( itemInfo );

//    if ( widgetList.size() != data.size() )
//    {
//        QLayout *contentsLayout = d_data->view->contentsWidget->layout();

//        while ( widgetList.size() > data.size() )
//        {
//            QWidget *w = widgetList.takeLast();

//            contentsLayout->removeWidget( w );

//            // updates might be triggered by signals from the legend widget
//            // itself. So we better don't delete it here.

//            w->hide();
//            w->deleteLater();
//        }

//        for ( int i = widgetList.size(); i < data.size(); i++ )
//        {
//            QWidget *widget = createWidget( data[i] );

//            if ( contentsLayout )
//                contentsLayout->addWidget( widget );

//            if ( isVisible() )
//            {
//                // QLayout does a delayed show, with the effect, that
//                // the size hint will be wrong, when applications
//                // call replot() right after changing the list
//                // of plot items. So we better do the show now.

//                widget->setVisible( true );
//            }

//            widgetList += widget;
//        }

//        if ( widgetList.isEmpty() )
//        {
//            d_data->itemMap.remove( itemInfo );
//        }
//        else
//        {
//            d_data->itemMap.insert( itemInfo, widgetList );
//        }

//        updateTabOrder();
//    }

//    for ( int i = 0; i < data.size(); i++ )
//        updateWidget( widgetList[i], data[i] );
}
