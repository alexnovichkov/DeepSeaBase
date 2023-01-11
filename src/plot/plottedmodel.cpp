#include "plottedmodel.h"
#include "fileformats/filedescriptor.h"

PlottedModel &PlottedModel::instance()
{
    static PlottedModel m;
    return m;
}

void PlottedModel::add(Channel *channel, Curve *curve)
{
    m_map.insertMulti(channel, curve);
}

void PlottedModel::remove(Channel *channel)
{
    m_map.remove(channel);
}

void PlottedModel::remove(Channel *channel, Curve *curve)
{
    auto it = m_map.find(channel);
    while (it != m_map.end() && it.key()==channel) {
        if (it.value()==curve) {
            m_map.erase(it);
            return;
        }
        ++it;
    }
}

void PlottedModel::remove(Curve *curve)
{
    QMutableMapIterator<Channel*, Curve*> i(m_map);
    while (i.hasNext()) {
        i.next();
        if (i.value() == curve)
            i.remove();
    }
}

bool PlottedModel::plotted(Channel *channel) const
{
    return m_map.contains(channel);
}

bool PlottedModel::plotted(FileDescriptor *d) const
{
    QMapIterator<Channel*, Curve*> i(m_map);
    while (i.hasNext()) {
        i.next();
        if (i.key()->descriptor() == d)
            return true;
    }
    return false;
}

int PlottedModel::count(Channel *channel) const
{
    return m_map.count(channel);
}

Curve *PlottedModel::curve(Channel *channel) const
{
    return m_map.value(channel, nullptr);
}

QList<Curve *> PlottedModel::curves(Channel *channel) const
{
    return m_map.values(channel);
}

PlottedModel::PlottedModel()
{

}
