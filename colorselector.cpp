#include "colorselector.h"

#include <QtCore>

#include "mainwindow.h"

ColorSelector* ColorSelector::m_Instance = 0;

static QColor defaultColor = QColor(QRgb(0x00808080));

static QList<QColor> usedColors;

static uint colorsTable[16]={
    0x00b40000,
    0x00000080,
    0x00008080,
    0x00803f00,
    0x00ff8000,
    0x000000ff,
    0x00808000,
    0x0000ffff,
    0x00f0f0c0, //240, 240, 192
    0x00800080,
    0x00ff00ff,
    0x00007800, //0, 120, 0
    0x00000000,
    0x00ff8080,
    0x008080ff,
    0x00a0a0a4
};

ColorSelector *ColorSelector::instance()
{
    static QMutex mutex;
    if (!m_Instance) {
        mutex.lock();

        if (!m_Instance)
            m_Instance = new ColorSelector;

        mutex.unlock();
    }

    return m_Instance;
}

void ColorSelector::drop()
{
    static QMutex mutex;
    mutex.lock();
    QVariantList list;
    QVector<QColor> vec = colors;
    foreach(const QColor &color, vec) {
        QVariant v;
        v.setValue<QColor>(color);
        list << v;
    }
    MainWindow::setSetting("colors", list);

    delete m_Instance;
    m_Instance = 0;
    mutex.unlock();
}

void ColorSelector::addColor(const QColor &color)
{
    static QMutex mutex;
    mutex.lock();
    colors.append(color);
    mutex.unlock();
}

void ColorSelector::setColor(const QColor &color, int index)
{
    static QMutex mutex;
    mutex.lock();
    if (index>=0 && index<colors.size())
        colors[index] = color;
    mutex.unlock();
}

void ColorSelector::removeColor(int index)
{
    static QMutex mutex;
    mutex.lock();
    if (index>=0 && index<colors.size())
        colors.remove(index);
    mutex.unlock();
}

QColor ColorSelector::getColor()
{
    for (int i=0; i<colors.size(); ++i) {
        QColor c = colors.at(i);
        if (!usedColors.contains(c)) {
            usedColors.append(c);
            return c;
        }
    }

    return QColor(QRgb(0x00808080));
}

void ColorSelector::resetState()
{
    usedColors.clear();
}

void ColorSelector::freeColor(const QColor &color)
{
    usedColors.removeAll(color);
}

ColorSelector::ColorSelector()
{
    QVariantList list = MainWindow::getSetting("colors").toList();
    if (list.isEmpty()) {
        colors.resize(COLORS_COUNT);
        for (int i=0; i<colors.size() && i<COLORS_COUNT; ++i)
            colors[i] = QColor(QRgb(colorsTable[i]));
    }
    else {
        colors.resize(list.size());
        for (int i=0; i<list.size(); ++i) {
            QVariant v = list.at(i);
            colors[i] = v.value<QColor>();
        }
    }
}
