#ifndef CHANNELSMIMEDATA_H
#define CHANNELSMIMEDATA_H

#include <QMimeData>
#include <QVector>


class ChannelsMimeData : public QMimeData
{
    Q_OBJECT
public:
    ChannelsMimeData(const QVector<int> &channels)
        : channels(channels) {}

    QVector<int> channels;
};

#endif // CHANNELSMIMEDATA_H
