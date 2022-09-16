#include "picker.h"

#include "logging.h"
#include <QMouseEvent>
#include <QKeyEvent>
#include <QApplication>
//#include "qwt_scale_map.h"
#include <QMenu>
#include "selectable.h"

Picker::Picker(Plot *plot) : plot(plot)
{DDD;
//    mode = ModeNone;
}

Selectable * Picker::findObject(QMouseEvent *e)
{DDD;
    Selectable *selected = nullptr;

    if (e->modifiers() == Qt::NoModifier || e->modifiers() == Qt::ControlModifier) {
        pos = e->pos();

        //Ищем элемент под курсором мыши
        double minDist = qInf();
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
//        //сбрасываем выделение
//        if (currentSelected && currentSelected != selected)
//            currentSelected->setSelected(false);
//        //новое выделение
//        currentSelected = selected;
    }
    return selected;
}

void Picker::startPick(QPoint startPos, Selectable *selected)
{DDD;
    startPosition = startPos;

    if (currentSelected != selected) {
        deselect();
        currentSelected = selected;
        if (currentSelected) currentSelected->setSelected(true);
    }
}

void Picker::deselect()
{DDD;
    const auto allItems = plot->itemList();
    for (auto item: allItems) {
        if (auto selectable = dynamic_cast<Selectable*>(item)) {
            //if (selectable != currentSelected)
                selectable->setSelected(false);
        }
    }
    currentSelected = nullptr;
}

void Picker::procKeyboardEvent(int key)
{DDD;
    if (!enabled || !currentSelected) return;

    switch (key) {
        case Qt::Key_Left: {
            currentSelected->moveLeft(QApplication::keyboardModifiers() & Qt::ShiftModifier ? 10 : 1);
            break;
        }
        case Qt::Key_Right: {
            currentSelected->moveRight(QApplication::keyboardModifiers() & Qt::ShiftModifier ? 10 : 1);
            break;
        }
        case Qt::Key_Up: {
            currentSelected->moveUp(QApplication::keyboardModifiers() & Qt::ShiftModifier ? 10 : 1);
            break;
        }
        case Qt::Key_Down: {
            currentSelected->moveDown(QApplication::keyboardModifiers() & Qt::ShiftModifier ? 10 : 1);
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
{DDD;
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
{DDD;
    if (!enabled || !currentSelected) return;

    currentSelected->moveToPos(e->pos(), startPosition);
    startPosition = e->pos();

    plot->replot();
}

void Picker::endPick(QMouseEvent *e)
{DD0;
    if (!enabled) return;
//    QPoint endPos = e->pos();
//    if (endPos == pos) { //одинарный клик мышью
//        qDebug()<<"click";
        //добавляем выделение объекту
        if (pickPriority() == PickPriority::PickLast) {
            if (auto selected = findObject(e)) {
                startPick(e->pos(), selected);
            }
        }
//    }
//    else {
//        //протащили какой-то объект, надо бросить

//    }

    plot->replot();
}

