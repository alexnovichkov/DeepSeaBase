#include "picker.h"

#include "logging.h"
#include <QMouseEvent>
#include <QKeyEvent>
#include <QApplication>
//#include "qwt_scale_map.h"
#include <QMenu>
#include "selectable.h"

Picker::Picker(Plot *plot) : plot(plot)
{DD;
//    mode = ModeNone;
}

bool Picker::findObject(QMouseEvent *e)
{DD;
    if (e->modifiers() == Qt::NoModifier || e->modifiers() == Qt::ControlModifier) {
        pos = e->pos();

        //Ищем элемент под курсором мыши
        double minDist = qInf();
        Selectable *selected = nullptr;

        const auto allItems = plot->itemList();
        for (auto item: allItems) {
            if (auto selectable = dynamic_cast<Selectable*>(item)) {
                double distx = 0.0;
                double disty = 0.0;
                if (selectable->underMouse(pos, &distx, &disty)) {
                    double dist = 0.0;
                    if (distx == qInf()) dist = disty;
                    else if (disty == qInf()) dist = distx;
                    else dist = sqrt(distx*distx+disty*disty);
                    if (!selected || dist < minDist) {
                        selected = selectable;
                        minDist = dist;
                    }
                }
            }
        }
        if (currentSelected && currentSelected != selected)
            currentSelected->setSelected(false);
        currentSelected = selected;
    }

    return currentSelected != nullptr;
}

void Picker::startPick(QPoint startPos)
{
    if (currentSelected) {
        currentSelected->setSelected(true);
        startPosition = startPos;
    }
}

void Picker::deselect()
{
    const auto allItems = plot->itemList();
    for (auto item: allItems) {
        if (auto selectable = dynamic_cast<Selectable*>(item)) {
            //if (selectable != currentSelected)
                selectable->setSelected(false);
        }
    }
}

void Picker::procKeyboardEvent(int key)
{DD;
    if (!enabled || !currentSelected) return;

    switch (key) {
        case Qt::Key_Left: {
            currentSelected->moveLeft(QApplication::keyboardModifiers() & Qt::ControlModifier ? 10 : 1);
            break;
        }
        case Qt::Key_Right: {
            currentSelected->moveRight(QApplication::keyboardModifiers() & Qt::ControlModifier ? 10 : 1);
            break;
        }
        case Qt::Key_Up: {
            currentSelected->moveUp(QApplication::keyboardModifiers() & Qt::ControlModifier ? 10 : 1);
            break;
        }
        case Qt::Key_Down: {
            currentSelected->moveDown(QApplication::keyboardModifiers() & Qt::ControlModifier ? 10 : 1);
            break;
        }
        case Qt::Key_Space: {
            currentSelected->fix();
            break;
        }
        case Qt::Key_C: {
            currentSelected->cycle();
            break;
        }
        case Qt::Key_Delete: {
            //1. nothing additional for PointLabel
            //3. Curve deletes itself
            currentSelected->remove();
            emit removeNeeded(currentSelected);
            currentSelected = nullptr;
            break;
        }
    }
}

void Picker::showContextMenu(QMouseEvent *e)
{
    if (!currentSelected) return;

    QMenu *m = new QMenu();
    m->addAction("Удалить",[=](){
        currentSelected->remove();
        emit removeNeeded(currentSelected);
        currentSelected = nullptr;
    });
    m->addActions(currentSelected->actions());

    m->exec(e->globalPos());
    m->deleteLater();
}

void Picker::proceedPick(QMouseEvent *e)
{DD;
    if (!enabled || !currentSelected) return;

    currentSelected->moveToPos(e->pos(), startPosition);
    startPosition = e->pos();

    plot->replot();
}

void Picker::endPick(QMouseEvent *e)
{DD;
    if (!enabled) return;
    QPoint endPos = e->pos();
    if (endPos == pos) { //одинарный клик мышью


    }
    else {
        //протащили какой-то объект, надо бросить

    }

    plot->replot();
}

