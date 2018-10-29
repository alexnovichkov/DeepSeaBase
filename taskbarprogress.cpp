#include "taskbarprogress.h"

#include <QWinTaskbarProgress>
#include <QWinTaskbarButton>
#include <QtWidgets>


TaskBarProgress::TaskBarProgress(QWidget *window, QObject *parent) : QObject(parent),
    window(window)
{
    QWinTaskbarButton *button = new QWinTaskbarButton(window);
    button->setWindow(window->windowHandle());

    winProgress = button->progress();
    winProgress->setVisible(true);
}


void TaskBarProgress::setRange(int min, int max)
{
    range = qMakePair(min, max);
    winProgress->setRange(min, max);
}

void TaskBarProgress::setValue(int value)
{
    this->value = value;
    winProgress->setValue(value);
}

void TaskBarProgress::reset()
{
    winProgress->reset();
    winProgress->setVisible(false);
    this->deleteLater();
}

void TaskBarProgress::finalize()
{
    //QTimer::singleShot(500, this, SLOT(reset()));
    reset();
}


