#include "legendlabel.h"
#include <qevent.h>

LegendLabel::LegendLabel(QWidget *parent) :
    QwtLegendLabel(parent)
{
    setToolTip("Удалить - правый клик мыши");
}

void LegendLabel::mousePressEvent(QMouseEvent *e)
{
    if ( e->button() == Qt::LeftButton )
    {
        setDown( true );
        updateState(true, QwtLegendData::Clickable);
        return;
    }
    if ( e->button() == Qt::RightButton )
    {
        bool down = !isDown();
        setDown( down );
        updateState(down, QwtLegendData::Checkable);
        if (down) Q_EMIT(markedForDelete());
        return;
    }
    QwtTextLabel::mousePressEvent( e );
}

void LegendLabel::mouseReleaseEvent(QMouseEvent *e)
{
    if ( e->button() == Qt::LeftButton )
    {
        setDown( false );
        updateState(false, QwtLegendData::Clickable);
        return;
    }
    if ( e->button() == Qt::RightButton )
    {
        return; // do nothing, but accept
    }
    QwtTextLabel::mouseReleaseEvent( e );
}

void LegendLabel::keyPressEvent(QKeyEvent *e)
{
    if ( e->key() == Qt::Key_Delete && !e->isAutoRepeat() && isDown()) {
        Q_EMIT(markedForDelete());
        return;
    }

    QwtLegendLabel::keyPressEvent( e );
}

void LegendLabel::updateState(bool state, QwtLegendData::Mode mode)
{
    if ( mode == QwtLegendData::Clickable )
    {
        if ( state )
            Q_EMIT pressed();
        else
        {
            Q_EMIT released();
            Q_EMIT clicked();
        }
    }

    if ( mode == QwtLegendData::Checkable )
        Q_EMIT checked( state );
}
