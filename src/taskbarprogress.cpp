#include "taskbarprogress.h"

#include <QWinTaskbarProgress>
#include <QWinTaskbarButton>
#include <QtWidgets>
#include "logging.h"

TaskBarProgress::TaskBarProgress(QWidget *window, QObject *parent) : QObject(parent),
    window(window)
{DDD;
    QWinTaskbarButton *button = new QWinTaskbarButton(window);
    button->setWindow(window->windowHandle());

    winProgress = button->progress();
    winProgress->setVisible(true);
}


void TaskBarProgress::setRange(int min, int max)
{DDD;
    range = qMakePair(min, max);
    winProgress->setRange(min, max);
}

void TaskBarProgress::setValue(int value)
{DDD;
    this->value = value;
    winProgress->setValue(value);
}

void TaskBarProgress::reset()
{DDD;
    winProgress->reset();
    winProgress->setVisible(false);
    this->deleteLater();
}

void TaskBarProgress::finalize()
{DDD;
    //QTimer::singleShot(500, this, SLOT(reset()));
    reset();
}


