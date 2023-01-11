#ifndef PLOTTEDMODEL_H
#define PLOTTEDMODEL_H

#include <QMultiMap>

class Channel;
class Curve;
class FileDescriptor;

class PlottedModel
{
    Q_DISABLE_COPY(PlottedModel)
public:
    static PlottedModel &instance();
    void add(Channel *channel, Curve *curve);
    void remove(Channel *channel);
    void remove(Channel *channel, Curve *curve);
    void remove(Curve *curve);
    bool plotted(Channel *channel) const;
    bool plotted(FileDescriptor *d) const;
    int count(Channel *channel) const;
    Curve *curve(Channel *channel) const;
    QList<Curve*> curves(Channel *channel) const;
private:
    PlottedModel();
    QMap<Channel*,Curve*> m_map;
};

#endif // PLOTTEDMODEL_H
