#ifndef CHANNELSMIMEDATA_H
#define CHANNELSMIMEDATA_H

#include <QMimeData>
#include <QVector>
class Channel;

class ChannelsMimeData : public QMimeData
{
    Q_OBJECT
public:
    ChannelsMimeData(const QVector<Channel*> &channels)
        : channels(channels) {}

    QVector<Channel*> channels;
    bool plotOnLeft = true;
};

#endif // CHANNELSMIMEDATA_H
