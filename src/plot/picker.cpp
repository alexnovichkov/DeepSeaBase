#include "picker.h"

#include "logging.h"
#include <QMouseEvent>
#include <QApplication>
#include <QMenu>
#include "qcpplot.h"

QDebug operator<<(QDebug deb, const Selected &o)
{
    deb << o.point.x << o.point.z << o.object;
    return deb;
}

Picker::Picker(Plot *plot) : plot(plot)
{DD;
//    mode = ModeNone;
}

Selected Picker::findObject(QMouseEvent *e)
{DD;
    Selected selected {nullptr, SelectedPoint()};

    if (e->modifiers() == Qt::NoModifier || e->modifiers() == Qt::ControlModifier) {
        pos = e->pos();
        selected = plot->impl()->findObject(pos);
    }
    return selected;
}

void Picker::startPick(QPoint startPos, Selected selected)
{DD;
    startPosition = startPos;

    if (!(selected == currentSelected)) deselect();
    currentSelected = selected;
    if (currentSelected.object) currentSelected.object->setSelected(true, currentSelected.point);
}

void Picker::deselect()
{DD;
    plot->impl()->deselect();
    currentSelected.clear();
}

void Picker::procKeyboardEvent(int key)
{DD;
    if (!enabled || !currentSelected.object) return;

    switch (key) {
        case Qt::Key_Left: {
            currentSelected.object->moveLeft(QApplication::keyboardModifiers() & Qt::ShiftModifier ? 10 : 1);
            break;
        }
        case Qt::Key_Right: {
            currentSelected.object->moveRight(QApplication::keyboardModifiers() & Qt::ShiftModifier ? 10 : 1);
            break;
        }
        case Qt::Key_Up: {
            currentSelected.object->moveUp(QApplication::keyboardModifiers() & Qt::ShiftModifier ? 10 : 1);
            break;
        }
        case Qt::Key_Down: {
            currentSelected.object->moveDown(QApplication::keyboardModifiers() & Qt::ShiftModifier ? 10 : 1);
            break;
        }
        case Qt::Key_Space: {
            currentSelected.object->fix();
            break;
        }
        case Qt::Key_C: {
            currentSelected.object->cycle();
            break;
        }
        case Qt::Key_Delete: {
            //1. nothing additional for PointLabel
            //3. Curve deletes itself
            currentSelected.object->remove();
            emit removeNeeded(currentSelected.object);
            currentSelected.clear();
            break;
        }
    }
}

void Picker::showContextMenu(QMouseEvent *e)
{DD;
    if (!currentSelected.object) return;

    QMenu *m = new QMenu();
    m->addAction("Удалить",[=](){
        currentSelected.object->remove();
        emit removeNeeded(currentSelected.object);
        currentSelected.clear();
    });
    m->addActions(currentSelected.object->actions());

    m->exec(e->globalPos());
    m->deleteLater();
}

bool Picker::alreadySelected(Selected selected)
{
//    return selected && selected->selectedAs(currentSelected);
    return selected == currentSelected;
}

void Picker::proceedPick(QMouseEvent *e)
{DD;
    if (!enabled || !currentSelected.object) return;

    currentSelected.object->moveToPos(e->pos(), startPosition);
    startPosition = e->pos();

    plot->impl()->replot();
}

void Picker::endPick(QMouseEvent *e)
{DD;
    if (!enabled) return;

    if (auto selected = findObject(e); selected.object) {
        startPick(e->pos(), selected);
    }
    else deselect();

    plot->impl()->replot();
}

