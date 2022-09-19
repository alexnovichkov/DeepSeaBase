#include "picker.h"

#include "logging.h"
#include <QMouseEvent>
#include <QKeyEvent>
#include <QApplication>
//#include "qwt_scale_map.h"
#include <QMenu>

Picker::Picker(Plot *plot) : plot(plot)
{DDD;
//    mode = ModeNone;
}

bool isCurve(QwtPlotItem *i)
{
    auto rtti = i->rtti();
    return (rtti == QwtPlotItem::Rtti_PlotCurve || rtti == QwtPlotItem::Rtti_PlotSpectrogram
            || rtti == QwtPlotItem::Rtti_PlotHistogram);
}

Selected Picker::findObject(QMouseEvent *e)
{DDD;
    Selected selected {nullptr, SelectedPoint()};

    if (e->modifiers() == Qt::NoModifier || e->modifiers() == Qt::ControlModifier) {
        pos = e->pos();

        //Ищем элемент под курсором мыши

        //сначала ищем метки, курсоры и т.д., то есть не кривые
        {
            double minDist = qInf();
            const auto allItems = plot->itemList();
            for (auto item: allItems) {
                if (auto selectable = dynamic_cast<Selectable*>(item)) {
                    if (isCurve(item)) continue;
                    double distx = 0.0;
                    double disty = 0.0;
                    SelectedPoint point;
                    if (selectable->underMouse(pos, &distx, &disty, &point)) {
                        double dist = 0.0;
                        if (distx == qInf()) dist = disty;
                        else if (disty == qInf()) dist = distx;
                        else dist = sqrt(distx*distx+disty*disty);
                        if (!selected.object || dist < minDist) {
                            selected.object = selectable;
                            selected.point = point;
                            minDist = dist;
                        }
                    }
                }
            }
        }
        if (!selected.object) {
            double minDist = qInf();
            const auto allItems = plot->itemList();
            for (auto item: allItems) {
                if (auto selectable = dynamic_cast<Selectable*>(item)) {
                    if (!isCurve(item)) continue;
                    double distx = 0.0;
                    double disty = 0.0;
                    SelectedPoint point;
                    if (selectable->underMouse(pos, &distx, &disty, &point)) {
                        double dist = 0.0;
                        if (distx == qInf()) dist = disty;
                        else if (disty == qInf()) dist = distx;
                        else dist = sqrt(distx*distx+disty*disty);
                        if (!selected.object || dist < minDist) {
                            selected.object = selectable;
                            selected.point = point;
                            minDist = dist;
                        }
                    }
                }
            }
        }
    }
    return selected;
}

void Picker::startPick(QPoint startPos, Selected selected)
{DDD;
    startPosition = startPos;

    if (!(selected == currentSelected)) deselect();
    currentSelected = selected;
    if (currentSelected.object) currentSelected.object->setSelected(true, currentSelected.point);

//    if (selected && !selected->selectedAs(currentSelected)) {
//        deselect();
//        currentSelected = selected;
//        if (currentSelected) currentSelected->setSelected(true);
//    }
}

void Picker::deselect()
{DDD;
    const auto allItems = plot->itemList();
    for (auto item: allItems) {
        if (auto selectable = dynamic_cast<Selectable*>(item)) {
            //if (selectable != currentSelected)
                selectable->setSelected(false, SelectedPoint());
        }
    }
    currentSelected.clear();
}

void Picker::procKeyboardEvent(int key)
{DDD;
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
{DDD;
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
{DDD;
    if (!enabled || !currentSelected.object) return;

    currentSelected.object->moveToPos(e->pos(), startPosition);
    startPosition = e->pos();

    plot->replot();
}

void Picker::endPick(QMouseEvent *e)
{DDD;
    if (!enabled) return;
//    QPoint endPos = e->pos();
//    if (endPos == pos) { //одинарный клик мышью
//        qDebug()<<"click";
        //добавляем выделение объекту
        if (pickPriority() == PickPriority::PickLast) {
            if (auto selected = findObject(e); selected.object) {
                startPick(e->pos(), selected);
            }
            else deselect();
        }
//    }
//    else {
//        //протащили какой-то объект, надо бросить

//    }

    plot->replot();
}

