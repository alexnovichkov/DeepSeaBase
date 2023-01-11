#ifndef PLOTTEDMODEL_H
#define PLOTTEDMODEL_H

#include <QMultiMap>

class Channel;
class Curve;

class PlottedModel
{
    Q_DISABLE_COPY(PlottedModel)
public:
    static PlottedModel &instance();
    void add(Channel *channel, Curve *curve);
    void remove(Channel *channel);
    void remove(Channel *channel, Curve *curve);
    void remove(Curve *curve);
    bool plotted(Channel *channel);
    int count(Channel *channel);
    Curve *curve(Channel *channel);
    QList<Curve*> curves(Channel *channel);
private:
    PlottedModel();
    QMap<Channel*,Curve*> m_map;
};

#endif // PLOTTEDMODEL_H
