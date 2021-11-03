#include "taskbarprogress.h"

#include <QWinTaskbarProgress>
#include <QWinTaskbarButton>
#include <QtWidgets>
#include "logging.h"

TaskBarProgress::TaskBarProgress(QWidget *window, QObject *parent) : QObject(parent),
    window(window)
{DD;
    QWinTaskbarButton *button = new QWinTaskbarButton(window);
    button->setWindow(window->windowHandle());

    winProgress = button->progress();
    winProgress->setVisible(true);
}


void TaskBarProgress::setRange(int min, int max)
{DD;
    range = qMakePair(min, max);
    winProgress->setRange(min, max);
}

void TaskBarProgress::setValue(int value)
{DD;
    this->value = value;
    winProgress->setValue(value);
}

void TaskBarProgress::reset()
{DD;
    winProgress->reset();
    winProgress->setVisible(false);
    this->deleteLater();
}

void TaskBarProgress::finalize()
{DD;
    //QTimer::singleShot(500, this, SLOT(reset()));
    reset();
}


