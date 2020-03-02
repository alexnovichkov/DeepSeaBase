#include "legendlabel.h"
#include <qevent.h>

LegendLabel::LegendLabel(QWidget *parent) :
    QwtLegendLabel(parent)
{
//    setToolTip("Удалить - правый клик мыши\nПереместить на правую ось Y - средняя кнопка мыши");
    setToolTip("Настроить - левая кнопка мыши\n"
               "Удалить - правая кнопка мыши");
}

void LegendLabel::mousePressEvent(QMouseEvent *e)
{
    switch (e->button()) {
        case Qt::LeftButton: {
            bool down = true;
            setDown( down );
            updateState(down, QwtLegendData::Clickable);
            break;
        }
        case Qt::RightButton: {
            bool down = !isDown();
            setDown( down );
            updateState(down, QwtLegendData::Checkable);
            if (down) Q_EMIT(markedForDelete());
            break;
        }
        case Qt::MiddleButton: {
            bool down = !isDown();
            setDown( down );
            updateState(down, QwtLegendData::Checkable);
            if (down) Q_EMIT(markedToMoveToRight());
            break;
        }
        default:
            break;
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


void LegendLabel::setText(const QwtText &text)
{
    QwtText s = text;

    if (data().hasRole(QwtLegendData::UserRole+1)) {// role for file number
        s.setText(s.text()+QString(" [%1]").arg(data().value(QwtLegendData::UserRole+1).toInt()));
    }
    QwtLegendLabel::setText(s);
}
